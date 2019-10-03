#include "TaskPool.h"

namespace rhythmus
{

// --------------------------------- class Task

Task::Task()
  : status_(0), current_thread_(nullptr)
{
}

void Task::wait()
{
  std::unique_lock<std::mutex> l(mutex_);
  task_done_cond_.wait(l, [this] { return this->status_ >= 2; });
}

bool Task::is_started() const
{
  return status_ >= 1;
}

bool Task::is_finished() const
{
  return status_ >= 2;
}


//---------------------------- class TaskThread

TaskThread::TaskThread(TaskPool *pool)
  : is_running_(true), pool_(pool)
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
  is_running_ = false;
  abort();
  current_task_ = nullptr;
  if (thread_.joinable())
    thread_.join();
}

void TaskThread::run()
{
  thread_ = std::thread([this]
  {
    for (; this->is_running_;)
    {
      current_task_ = this->pool_->DequeueTask();
      if (!current_task_)
        break;

      // mark as running state
      current_task_->status_ = 1;
      current_task_->current_thread_ = this;

      // run
      current_task_->run();

      // mark as finished state
      current_task_->status_ = 2;
      current_task_->current_thread_ = nullptr;
      current_task_->task_done_cond_.notify_all();
    }
  });
}

void TaskThread::set_task(TaskAuto& task)
{
  current_task_ = task;
}

void TaskThread::abort()
{
  if (current_task_)
    current_task_->abort();
}

// ----------------------------- class TaskPool

TaskPool::TaskPool(size_t size) : pool_size_(0), stop_(false)
{
  SetPoolSize(size);
}

TaskPool::~TaskPool()
{
  ClearTaskPool();
}

void TaskPool::SetPoolSize(size_t size)
{
  if (size == 0)
    return;

  // before re-allocating, cancel all task (if exists)
  ClearTaskPool();

  for (size_t i = 0; i < size; ++i)
  {
    TaskThread *tt = new TaskThread(this);
    worker_pool_.push_back(tt);
    tt->run();
  }

  pool_size_ = size;
}

void TaskPool::EnqueueTask(TaskAuto task)
{
  std::lock_guard<std::mutex> lock(task_pool_mutex_);
  task_pool_.push_back(task);
  task_pool_cond_.notify_one();
}

TaskAuto TaskPool::DequeueTask()
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
  TaskAuto task = task_pool_.front();
  task_pool_.pop_front();
  return task;
}

void TaskPool::ClearTaskPool()
{
  // prepare status to exit thread
  stop_ = true;
  AbortAllTask();
  WaitAllTask();
  // notify all waiting thread to exit
  task_pool_cond_.notify_all();
  // all thread is done, clear threads.
  for (auto *wthr : worker_pool_)
    delete wthr;
  worker_pool_.clear();
  pool_size_ = 0;
  stop_ = false;
}


void TaskPool::AbortAllTask()
{
  task_pool_.clear();
  for (auto *wthr : worker_pool_)
    wthr->abort();
}

void TaskPool::WaitAllTask()
{
  while (!task_pool_.empty())
  {
    task_pool_.back()->wait();
  }
}

bool TaskPool::is_idle() const
{
  return task_pool_.empty();
}

TaskPool& TaskPool::getInstance()
{
  static TaskPool pool;
  return pool;
}


}