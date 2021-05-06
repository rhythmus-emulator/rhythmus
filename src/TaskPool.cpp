#include "TaskPool.h"
#include "Error.h"
#include "common.h"

namespace rhythmus
{

unsigned __task_id = 1;

// --------------------------------- class Task

Task::Task()
  : id_(__task_id++), status_(0), current_thread_(nullptr), callback_(nullptr)
{
}

Task::Task(bool is_async_task)
  : id_(__task_id++), status_(0), current_thread_(nullptr), callback_(nullptr)
{
}

void Task::run_task()
{
  _run_state();
  run();
  _finish_state();
}

void Task::abort_task()
{
  abort();
  _finish_state();
}

void Task::wait()
{
  // use spinlock to prevent hang.
  // (e.g. set conditional_variable before wait() is called)
  // XXX: is there better way to wait without spinlock?
  if (is_finished()) return;
  while (this->status_ < 2)
    _sleep(10);
  //std::unique_lock<std::mutex> l(mutex_);
  //task_done_cond_.wait(l, [this] { return this->status_ >= 2; });
}

void Task::set_callback(ITaskCallback *callback)
{
  callback_ = callback;
}

unsigned Task::get_task_id() const
{
  return id_;
}

bool Task::is_started() const
{
  return status_ >= 1;
}

bool Task::is_finished() const
{
  return status_ >= 2;
}

void Task::_run_state()
{
  status_ = 1;
}

void Task::_finish_state()
{
  status_ = 2;
  current_thread_ = nullptr;
  task_done_cond_.notify_all();
  if (callback_) callback_->run();
}

//---------------------------- class TaskThread

TaskThread::TaskThread(TaskPool *pool)
  : is_running_(true), current_task_(nullptr), pool_(pool)
{
}

TaskThread::TaskThread(TaskThread&& t) noexcept
{
  TaskThread::operator=(std::move(t));
}

TaskThread& TaskThread::operator=(TaskThread&& t) noexcept
{
  thread_ = std::move(t.thread_);
  is_running_ = t.is_running_;
  current_task_ = std::move(t.current_task_);
  pool_ = t.pool_;
  return *this;
}

TaskThread::~TaskThread()
{
  exit();
}

void TaskThread::exit()
{
  if (is_running_) {
    is_running_ = false;
    abort();
    current_task_ = nullptr;
    if (thread_.joinable())
      thread_.join();
  }
}

void TaskThread::run()
{
  thread_ = std::thread([this] {
    for (; this->is_running_;) {
      this->current_task_ = this->pool_->DequeueTask();
      if (!this->current_task_)
        break;

      // mark as running state
      this->current_task_->current_thread_ = this;

      // run
      this->current_task_->run_task();

      // mark as finished state and delete task
      delete this->current_task_;
      this->current_task_ = nullptr;
    }
  });
}

void TaskThread::abort()
{
  if (current_task_)
    current_task_->abort_task();
}

void TaskThread::set_task(Task* task)
{
  current_task_ = task;
}

bool TaskThread::is_idle() const { return current_task_ == nullptr; }

// ----------------------------- class TaskPool

TaskPool::TaskPool(size_t size) : pool_size_(0), stop_(false)
{
  SetPoolSize(size);
}

TaskPool::~TaskPool()
{
  ClearTaskPool();
}

void TaskPool::Initialize()
{
  // TODO: load preference from setting
  TASKMAN = new TaskPool(4);
}

void TaskPool::Destroy()
{
  delete TASKMAN;
  TASKMAN = nullptr;
}

void TaskPool::SetPoolSize(size_t size)
{
  if (size == 0)
    return;

  // before re-allocating, cancel all task (if exists)
  ClearTaskPool();

  for (size_t i = 0; i < size; ++i) {
    TaskThread *tt = new TaskThread(this);
    worker_pool_.push_back(tt);
    tt->run();
  }

  pool_size_ = size;
}

size_t TaskPool::GetPoolSize() const
{
  return pool_size_;
}

unsigned TaskPool::EnqueueTask(Task* task)
{
  R_ASSERT(task != nullptr);
  // if new task is added after AbortAllTask() while Destroy(),
  // then TASKMAN hangs. To prevent it, Don't enqueue new task while halting.
  // such case may occur when running task enqueue new task during destruction.
  if (stop_) {
    task->abort();
    task->_finish_state();
    delete task;
    return 0;
  }
  std::lock_guard<std::mutex> lock(task_pool_mutex_);
  task_pool_.push_back(task);
  task_pool_cond_.notify_one();
  return task->get_task_id();
}

void TaskPool::Await(Task* task)
{
  EnqueueTask(task);
  if (stop_) return;
  task->wait();
  delete task;
}

Task* TaskPool::DequeueTask()
{
  std::unique_lock<std::mutex> lock(task_pool_mutex_);
  // wait until new task is registered
  task_pool_cond_.wait(lock,
    [this] { return this->stop_ || !this->task_pool_.empty(); }
  );
  // if thread need to be cleared, exit.
  if (stop_)
    return nullptr;
  // fetch task
  auto task = task_pool_.front();
  task_pool_.pop_front();
  return task;
}

bool TaskPool::IsRunning(const Task* task)
{
  std::unique_lock<std::mutex> lock(task_pool_mutex_);
  if (!task) return false;
  for (const auto t : task_pool_)
    if (t == task) return true;
  for (auto* worker : worker_pool_)
    if (worker->current_task_ == task) return true;
  return false;
}

void TaskPool::ClearTaskPool()
{
  // abort all running tasks
  // and notify all waiting thread to exit
  stop_ = true;
  AbortAllTask();
  for (auto* t : task_pool_) delete t;
  task_pool_.clear();
  task_pool_cond_.notify_all();
  // all thread is done, clear threads.
  for (auto* wthr : worker_pool_) {
    wthr->exit();
    delete wthr;
  }
  worker_pool_.clear();
  pool_size_ = 0;
  stop_ = false;
}


void TaskPool::AbortAllTask()
{
  {
    std::unique_lock<std::mutex> lock(task_pool_mutex_);
    for (auto* wthr : worker_pool_)
      wthr->abort();
    for (auto* t : task_pool_) {
      t->abort_task();
    }
  }
  WaitAllTask();
}

void TaskPool::WaitAllTask()
{
  {
    std::unique_lock<std::mutex> lock(task_pool_mutex_);
    while (!task_pool_.empty()) {
      task_pool_.back()->wait();
    }
  }
  for (auto *wthr : worker_pool_) {
    if (wthr->current_task_)
      wthr->current_task_->wait();
  }
  // XXX: waiting main thread is impossible, so just ignore it.
}

bool TaskPool::is_idle() const
{
  if (!task_pool_.empty()) return false;
  for (auto *tt : worker_pool_)
    if (!tt->is_idle()) return false;
  return true;
}

TaskPool* TASKMAN;

}
