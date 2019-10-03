/**
 * TaskPool : enqueue / get async tasks using thread pool.
 * @brief For easy running of async procedures.
 */
#pragma once

#include <limits>
#include <string>
#include <thread>
#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>

namespace rhythmus
{

class TaskPool;
class TaskThread;

/* @brief A task which is used for execution */
class Task
{
public:
  // constructor for temporary task
  Task();

  virtual void run() = 0;
  virtual void abort() = 0;
  void wait();

  bool is_started() const;
  bool is_finished() const;

  friend class TaskPool;
  friend class TaskThread;

private:
  /* task status
   * 0: not run
   * 1: running
   * 2: finished
   */
  int status_;

  /* owner of this task */
  TaskThread *current_thread_;

  /* condition variable for task is done */
  std::condition_variable task_done_cond_;
  std::mutex mutex_;
};

using TaskAuto = std::shared_ptr<Task>;

class TaskThread
{
public:
  TaskThread(TaskPool *pool);
  TaskThread(const TaskThread&) = delete;
  TaskThread(TaskThread&&) noexcept;
  TaskThread& operator=(TaskThread&&) noexcept;
  ~TaskThread();
  void run();
  void abort();
  void set_task(TaskAuto& task);

  friend class TaskPool;

private:
  std::thread thread_;
  bool is_running_;
  TaskAuto current_task_;
  TaskPool *pool_;
};

class TaskPool
{
public:
  TaskPool(size_t size = 0);
  ~TaskPool();

  void SetPoolSize(size_t size);
  void ClearTaskPool();
  void EnqueueTask(TaskAuto& task);
  TaskAuto DequeueTask();

  void AbortAllTask();
  void WaitAllTask();
  bool is_idle() const;

  static TaskPool& getInstance();

private:
  size_t pool_size_;
  std::vector<TaskThread*> worker_pool_;
  bool stop_;

  /* @brief task pool which needs to be retrieved later.
   * enqueue task with duplicated name in task pool is not allowed. */
  std::list<TaskAuto> task_pool_;
  std::mutex task_pool_mutex_;
  std::condition_variable task_pool_cond_;
};

}
