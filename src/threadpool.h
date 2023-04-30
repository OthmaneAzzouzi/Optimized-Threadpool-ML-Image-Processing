#ifndef INF3170_THREADPOOL_H_
#define INF3170_THREADPOOL_H_

#include <pthread.h> //Pthreads are a standard interface for creating and managing threads in a multi-threaded environment
#include <stdint.h> //Integer types with specific bit widths.
#include <sys/types.h> //Various type definitions (size_t, pid_t, uid_t) used in system calls and other system-related functions

#include "list.h" //Liste chainée

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*func_t)(void *);

struct worker_arg {
  int id;
  struct pool *pool; //When a worker thread is created, it needs to know which pool it belongs to
};

struct task {
  func_t func;
  void *arg;
};

struct pool {
  int nb_threads;
  int barrier_ready;
  pthread_t *threads;
  struct worker_arg *args;

  pthread_barrier_t ready;

  pthread_mutex_t lock;
  pthread_cond_t work_todo;
  pthread_cond_t work_done;

  struct list *task_list;

  int running;
};

struct pool *threadpool_create(int num);
void threadpool_add_task(struct pool *pool, func_t fn, void *arg);
void threadpool_join(struct pool *pool);

#ifdef __cplusplus
}
#endif

#endif /* INF3170_THREADPOOL_H_ */
