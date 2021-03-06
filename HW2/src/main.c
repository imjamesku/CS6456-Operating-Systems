/* #define DEBUG */

#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>

#include "floral.h"
#include "utils.h"
#include "main.h"
#include "thread.h"
#include "utils.h"

/*
 * Initial global state.
 */
floral_state STATE = {
  .threads = NULL,
  .current = NULL
};

/**
 * Initializes the floral green thread library.
 */
void grn_init() {
  STATE.current = grn_new_thread(false);
  assert_malloc(STATE.current);
  STATE.current->status = RUNNING;
}

/**
 * Creates a new green thread and executes `fn` inside that thread.
 *
 * Allocates and initializes a new green thread so that the parameter `fn` is
 * executed inside of the new thread. Each thread is allocated its own stack.
 * After allocating and initialization the new thread, the current thread yields
 * its execution.
 *
 * @param fn The function to execute inside a new green thread.
 *
 * @return The thread ID of the newly spawned process.
 */
int grn_spawn(grn_fn fn) {
  UNUSED(fn);
  grn_gc();
  grn_thread *newThread = grn_new_thread(true);
  uint8_t *stackPtr = newThread->stack + STACK_SIZE;

  stackPtr -= 8;
  memcpy(stackPtr, &fn, sizeof(fn));

  void (*startThreadPointer)(void) = start_thread;
  
  stackPtr -= 8;
  memcpy(stackPtr, &startThreadPointer, sizeof(startThreadPointer));
  

  newThread->context.rsp = (uint64_t)stackPtr;
  newThread->status = READY;
  grn_yield(0);

  // FIXME: Allocate a new thread, initialize its context, then yield.
  return newThread->id;
}

/**
 * Garbage collects ZOMBIEd threads.
 *
 * Frees the resources for all threads marked ZOMBIE.
 */
void grn_gc() {
  // FIXME: Free the memory of zombied threads.
  grn_thread* cur = STATE.threads;
  while (cur != NULL) {
    grn_thread *tmp = cur;
    cur = cur->next;
    if (tmp->status == ZOMBIE) {
      grn_destroy_thread(tmp);
    }
  }
}

/**
 * Yields the execution time of the current thread to another thread while the
 * current thread waits on a condition variable to be satisfied.
 *
 * The argument `condition` is the condition (identified by a nonzero number)
 * the thread is waiting on. It is not valid for a thread to wait on 0 as it is
 * not a valid condition identifier.
 *
 * This function should run any thread that is READY next.
 *
 * The current thread is marked READY if it was previous RUNNING, otherwise, its
 * status remained unchanged. The status of the thread being switched to is
 * marked RUNNING. If no thread to switch to is found, this function return -1.
 * Otherwise, it returns 0.
 *
 * @return 0 if execution was yielded, -1 if no yielding occurred
 */
int grn_wait(int condition) {
  UNUSED(condition);
  // debug_thread_print(grn_current());
  STATE.current->condition = condition;
  // return grn_yield(0);

  grn_thread* threadToRun = NULL;
  grn_thread* cur = STATE.current;
  grn_thread *oldThread;
  cur = next_thread(STATE.current);
  while (cur != STATE.current) {
    if (cur->status == READY) {
      threadToRun = cur;
    }
    cur = next_thread(cur);
  }
  
  if (STATE.current->status == RUNNING){
    STATE.current->status = READY;
  }
  if (threadToRun != NULL) {
    remove_thread(threadToRun);
    add_thread(threadToRun);
    
    oldThread = grn_current();
    STATE.current = threadToRun;
    threadToRun->status = RUNNING;
    fprintf(stderr, "123");
    grn_context_switch(&oldThread->context, &threadToRun->context);
    
    return 0;
  }

  // FIXME
  return -1;
}

/**
 * Yields the execution time of the current thread to another thread, while
 * optionally signaling that a given condition has been satisfied.
 *
 * The argument `condition` can be used to signal that a condition (identified
 * by a nonzero number) has been met. If the thread wants to yield without
 * signaling a condition it should pass 0 as `condition`.
 *
 * This function should decide which thread to run next according to the
 * following policy:
 *
 * - If a condition was signaled as ready (i.e. `condition` is nonzero), then a
 *   thread waiting on that condition should be run. If there are multiple, then
 *   the thread which has been waiting on that condition the longest should be
 *   run.
 * - Otherwise, run any READY thread.
 *
 * The current thread is marked READY if it was previous RUNNING, otherwise, its
 * status remained unchanged. The status of the thread being switched to is
 * marked RUNNING. If no thread to switch to is found, this function return -1.
 * Otherwise, it returns 0.
 *
 * @return 0 if execution was yielded, -1 if no yielding occurred
 */
int grn_yield(int condition) {
  UNUSED(condition);
  grn_thread* threadToRun = NULL;
  grn_thread* cur;
  grn_thread *oldThread;
  if (condition != 0) {
    // STATE.current->condition = 0;
    cur = next_thread(STATE.current);
    while (cur != STATE.current) {
      if (cur->condition == condition && cur->status == READY) {
        threadToRun = cur;
      }
      cur = next_thread(cur);
    }
  }
  if (threadToRun == NULL) {
    cur = next_thread(STATE.current);
    while (cur!= STATE.current) {
      if (cur->status == READY) {
        threadToRun = cur;
      }
      cur = next_thread(cur);
    }
  }
  
  if (STATE.current->status == RUNNING){
    STATE.current->status = READY;
  }
  
  if (threadToRun != NULL) {
    remove_thread(threadToRun);
    add_thread(threadToRun);
    threadToRun->status = RUNNING;
    oldThread = STATE.current;
    STATE.current = threadToRun;
    grn_context_switch(&oldThread->context, &threadToRun->context);
    return 0;
  } else {
    return -1;
  }
  
  // FIXME: Yield the current thread's execution time to another READY thread.
}

/**
 * Blocks until all threads except for the initial thread have finished
 * executing.
 *
 * @return 0 on successful wait, nonzero otherwise
 */
int grn_join() {
  // Loop until grn_yield returns nonzero.
  while (!grn_yield(0));

  return 0;
}

/**
 * Exits from the calling thread.
 *
 * If the calling thread is the initial thread, then this function exits the
 * program. Otherwise, the calling thread is marked ZOMBIE so that it is never
 * rescheduled and is eventually garbage collected. This function never returns.
 */
void grn_exit() {
  debug("Thread %" PRId64 " is exiting.\n", STATE.current->id);
  if (STATE.current->id == 0) {
    exit(0);
  }

  STATE.current->status = ZOMBIE;
  grn_yield(0);
}

/**
 * For compatibility across name manglers.
 */
void _grn_exit() { grn_exit(); }

/**
 * Returns a pointer to the current thread if there is one. This pointer is only
 * valid during the lifetime of the thread.
 *
 * @return a pointer to the current thread or NULL if the library hasn't been
 * initialized
 */
grn_thread *grn_current() {
  return STATE.current;
}
