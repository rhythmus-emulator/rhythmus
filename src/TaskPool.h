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
#include <atomic>
#include <condition_variable>

namespace rhythmus
{

class TaskPool;
class TaskThread;
class TaskGroup;
class ITaskCallback {
public:
  virtual void run() = 0;
};

/**
 * @brief A task which is used for execution
 *
 * Originally, Task was an isolated, non-cancelable object, which is
 * independent to other objects. But, Task is mostly used for loading
 * **other object**, so it's actually impossible. and should consider
 * about canceling Task while/before loading.
 * (e.g. Song preview loading cancel)
 *
 * So, Two main feature of Task is
 * * Cancelable unless it's already loading.
 * * Dependent to other object.
 *
 * By these features, be aware to use Task object as written below.
 * * Cancel and use callback function of TaskGroup to release task-loaded object.
 *
 * And Task object is polled by TaskThread worker object, but not instantly.
 * Task object can be modified until next TASKMAN->Update().
 * That is, only Task running is async, other things are synced to main thread.
 * (e.g. Task appending, TaskGroup onfinished callback execution, ...)
 */
class Task
{
public:
  Task();

  virtual void run() = 0;
  virtual void abort() = 0;
  void run_task();
  void abort_task();
  void wait();
  void set_callback(ITaskCallback *callback);
  void set_group(TaskGroup* g);

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
   * 3: canceled
   */
  int status_;

  /* owner of this task */
  TaskThread *current_thread_;

  ITaskCallback *callback_;
  TaskGroup* group_;

  /* condition variable for task is done */
  std::condition_variable task_done_cond_;
  std::mutex mutex_;

  void _run_state();
  void _finish_state();
};

using TaskAuto = std::shared_ptr<Task>;

class TaskGroup
{
public:
  bool IsIdle() const;
  bool IsFinished() const;
  double GetProgress() const;
  void AddFinishedTask();
  virtual void OnFinishedCallback() = 0;

  friend class Task;
private:
  unsigned total_tasks_;
  unsigned loaded_tasks_;
};

/* @brief Working thread for Task object. */
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
  bool is_idle() const;

  friend class TaskPool;
  friend class TaskThread;

private:
  std::thread thread_;
  bool is_running_;
  Task* current_task_;
  TaskPool *pool_;
};

/* @brief Container of TaskThread and Task object */
class TaskPool
{
public:
  static void Initialize();
  static void Destroy();

  void SetTaskGroup(TaskGroup* g);
  void UnsetTaskGroup();

  void SetPoolSize(size_t size);
  size_t GetPoolSize() const;
  void ClearTaskPool();
  void EnqueueTask(Task* task);
  void EnqueueFinishedTask(Task* task);
  Task* DequeueTask();
  void Update();

  void AbortAllTask();
  bool is_idle() const;

private:
  TaskPool(size_t size = 0);
  ~TaskPool();

  size_t pool_size_;
  size_t task_count_;
  std::vector<TaskThread*> worker_pool_;
  bool stop_;
  TaskGroup* curr_taskgroup_;

  /* @brief task pool which needs to be retrieved later.
   * enqueue task with duplicated name in task pool is not allowed. */
  std::vector<Task*> task_pool_internal_;
  std::list<Task*> task_pool_;
  std::list<Task*> finished_task_pool_;
  std::mutex task_pool_mutex_;
  std::mutex finished_task_pool_mutex_;
  std::condition_variable task_pool_cond_;
};

extern TaskPool* TASKMAN;

}
