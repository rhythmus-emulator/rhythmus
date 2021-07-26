#include "TaskPool.h"
#include "Error.h"
#include "common.h"

namespace rhythmus
{

unsigned __task_id = 1;

// --------------------------------- class Task

Task::Task() :
  id_(__task_id++), status_(0), current_thread_(nullptr),
  callback_(nullptr), group_(nullptr)
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

void Task::set_group(TaskGroup* g)
{
  group_ = g;
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

//---------------------------- class TaskGroup

bool TaskGroup::IsIdle() const { return total_tasks_ == 0; }
bool TaskGroup::IsFinished() const { return total_tasks_ == loaded_tasks_; }
double TaskGroup::GetProgress() const { return (double)loaded_tasks_ / total_tasks_; }
void TaskGroup::AddFinishedTask()
{
  // XXX: MUST be called in main thread (TASKMAN::Update)
  R_ASSERT(loaded_tasks_ < total_tasks_);
  ++loaded_tasks_;
  if (loaded_tasks_ == total_tasks_) {
    OnFinishedCallback();
  }
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

      // mark as finished state and move to finished task
      this->current_task_->current_thread_ = nullptr;
      this->pool_->EnqueueFinishedTask(this->current_task_);
      this->current_task_ = nullptr;
    }
  });
}

void TaskThread::abort()
{
  if (current_task_)
    current_task_->abort_task();
}

bool TaskThread::is_idle() const { return current_task_ == nullptr; }

// ----------------------------- class TaskPool

TaskPool::TaskPool(size_t size) :
  pool_size_(0), task_count_(0), stop_(false), curr_taskgroup_(nullptr)
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

void TaskPool::SetTaskGroup(TaskGroup* g)
{
  curr_taskgroup_ = g;
}

void TaskPool::UnsetTaskGroup()
{
  curr_taskgroup_ = nullptr;
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

void TaskPool::EnqueueTask(Task* task)
{
  R_ASSERT(task != nullptr);
  task->set_group(curr_taskgroup_);
  // if new task is added after AbortAllTask() while Destroy(),
  // then TASKMAN hangs. To prevent it, Don't enqueue new task while halting.
  // such case may occur when running task enqueue new task during destruction.
  if (stop_) {
    task->abort();
    task->_finish_state();
    delete task;
    return;
  }
  task_pool_internal_.push_back(task);
  ++task_count_;
}

void TaskPool::EnqueueFinishedTask(Task* task)
{
  std::lock_guard<std::mutex> lock(finished_task_pool_mutex_);
  finished_task_pool_.push_back(task);
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

void TaskPool::Update()
{
  // inform tasks to run Task object.
  if (!task_pool_internal_.empty()) {
    {
      std::lock_guard<std::mutex> lock(task_pool_mutex_);
      for (auto* t : task_pool_internal_) task_pool_.push_back(t);
      task_pool_internal_.clear();
    }
    task_pool_cond_.notify_all();
  }

  // process finished task
  if (!finished_task_pool_.empty()) {
    std::list<Task*> tlist;
    {
      std::lock_guard<std::mutex> lock(finished_task_pool_mutex_);
      finished_task_pool_.swap(tlist);
    }
    for (auto* t : tlist) {
      if (t->group_) t->group_->AddFinishedTask();
      delete t;
      R_ASSERT(task_count_ > 0);
      --task_count_;
    }
  }
}

void TaskPool::ClearTaskPool()
{
  // abort all running tasks and notify all waiting thread to exit
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
  // To clear finished tasks completely
  Update();
  R_ASSERT(task_count_ == 0);
  // reset stop status flag
  stop_ = false;
}


void TaskPool::AbortAllTask()
{
  for (auto* wthr : worker_pool_)
    wthr->abort();
  for (auto* t : task_pool_)
    t->abort_task();
}

bool TaskPool::is_idle() const
{
  return task_count_ == 0;
}

TaskPool* TASKMAN;

}
