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
class ITaskCallback {
public:
  virtual void run() = 0;
};

/* @brief A task which is used for execution */
class Task
{
public:
  // constructor for temporary task
  Task();
  explicit Task(bool is_async_task);

  virtual void run() = 0;
  virtual void abort() = 0;
  void run_task();
  void abort_task();
  void wait();
  void set_callback(ITaskCallback *callback);

  unsigned get_task_id() const;
  bool is_started() const;
  bool is_finished() const;

  friend class TaskPool;
  friend class TaskThread;

private:
  /* identifier for task. */
  unsigned id_;

  /* task status
   * 0: not run
   * 1: running
   * 2: finished
   */
  int status_;

  /* owner of this task */
  TaskThread *current_thread_;

  ITaskCallback *callback_;

  /* condition variable for task is done */
  std::condition_variable task_done_cond_;
  std::mutex mutex_;

  void _run_state();
  void _finish_state();
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
  void exit();
  void abort();
  void set_task(Task* task);
  bool is_idle() const;

  friend class TaskPool;
  friend class TaskThread;

private:
  std::thread thread_;
  bool is_running_;
  Task* current_task_;
  TaskPool *pool_;
};

class TaskPool
{
public:
  static void Initialize();
  static void Destroy();

  void SetPoolSize(size_t size);
  size_t GetPoolSize() const;
  void ClearTaskPool();
  unsigned EnqueueTask(Task* task);
  void Await(Task* task);
  Task* DequeueTask();
  bool IsRunning(const Task* task);

  void AbortAllTask();
  void WaitAllTask();
  bool is_idle() const;

private:
  TaskPool(size_t size = 0);
  ~TaskPool();

  size_t pool_size_;
  std::vector<TaskThread*> worker_pool_;
  bool stop_;

  /* @brief task pool which needs to be retrieved later.
   * enqueue task with duplicated name in task pool is not allowed. */
  std::list<Task*> task_pool_;
  std::mutex task_pool_mutex_;
  std::condition_variable task_pool_cond_;
};

extern TaskPool* TASKMAN;

}
