#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "floral.h"
#include "utils.h"
#include "main.h"
#include "thread.h"

/**
 * Returns a unique number.
 *
 * Each call to this function returns a number that is one more than the number
 * returned from the previous call. The first call to this functions returns 0.
 *
 * @return 0 on the first invocation; after, a number than is one more than the
 * previously returned number
 */
int64_t atomic_next_id() {
  // TODO: Make atomic if multiplexing green threads onto OS threads.
  static int64_t number = 0;
  return number++;
}

/**
 * Adds the `thread` to the linked list headed by STATE.threads. Panics if the
 * pointer to the thread being added is NULL.
 *
 * @param thread the thread to add to the linked list; must be non-null
 */
void add_thread(grn_thread *thread) {
  assert(thread);
  if (STATE.threads) {
    STATE.threads->prev = thread;
  }

  thread->prev = NULL;
  thread->next = STATE.threads;
  STATE.threads = thread;
}

/**
 * Removes the `thread` to the linked list headed by STATE.threads. Panics if
 * the pointer to the thread being removed is NULL.
 *
 * @param thread the thread being removed from linked list; must be non-null
 */
void remove_thread(grn_thread *thread) {
  assert(thread);
  if (STATE.threads == thread) {
    STATE.threads = thread->next;
  }

  if (thread->next) {
    thread->next->prev = thread->prev;
  }

  if (thread->prev) {
    thread->prev->next = thread->next;
  }
}

/**
 * Returns a pointer to the thread following `thread` in the linked list headed
 * by STATE.threads. If `thread` is last  in the linked list, this function
 * returns the head of the linked list such that a cycle is formed. Panics if
 * the pointer to the thread parameter is NULL.
 *
 * @param thread to use as a basis for the next thread; must be non-null
 *
 * @return a pointer to the thread after `thread`
 */
grn_thread *next_thread(grn_thread *thread) {
  assert(thread);
  return (thread->next) ? thread->next : STATE.threads;
}

/**
 * Allocates a new grn_thread structure and returns a pointer to it.
 *
 * Allocates and a new grn_thread structure, zeroes out its context, sets its ID
 * to a unique number, sets its status to WAITING, and adds the thread to the
 * linked list headed by STATE.threads. If `alloc_stack` is true, a 16-byte
 * aligned memory region of size `STACK_SIZE` is allocated, and a pointer to the
 * region is stored in the thread's `stack` property.
 *
 * @param alloc_stack whether or not to allocate a stack for the thread
 *
 * @return a pointer to the newly allocated grn_thread structure
 */
grn_thread *grn_new_thread(bool alloc_stack) {
  UNUSED(alloc_stack);
  grn_thread* newThread = (grn_thread*)malloc(sizeof(grn_thread));
  newThread->context.r12 = 0;
  newThread->context.r13 = 0;
  newThread->context.r14 = 0;
  newThread->context.r15 = 0;
  newThread->context.rbp = 0;
  newThread->context.rbx = 0;
  newThread->context.rsp = 0;
  newThread->id = atomic_next_id();
  newThread->status = WAITING;
  if (alloc_stack) {
    newThread->stack = (uint8_t*)malloc(STACK_SIZE);
  } else {
    newThread->stack = NULL;
  }
  add_thread(newThread);

  // FIXME: Allocate a new thread and stack.
  return newThread;
}

/**
 * Frees the resources used by `thread` and the thread itself. Removes `thread`
 * from the linked list headed by STATE.threads.
 *
 * @param thread the thread to deallocate and remove from linked list
 */
void grn_destroy_thread(grn_thread *thread) {
  UNUSED(thread);
  remove_thread(thread);
  free(thread->stack);
  free(thread);

  // FIXME: Free the resources used by `thread`.
}

/**
 * Prints a formatted debug message for `thread`.
 *
 * @param thread the thread to debug pretty-pring
 */
void debug_thread_print(grn_thread *thread) {
  const char *status;
  switch (thread->status) {
    case WAITING: status = "WAITING"; break;
    case READY: status = "READY"; break;
    case RUNNING: status = "RUNNING"; break;
    case ZOMBIE: status = "ZOMBIE"; break;
    default: status = "UNKNOWN";
  }

  fprintf(stderr, ":: Thread ID:\t %" PRId64 "\n", thread->id);
  fprintf(stderr, ":: Status:\t %s\n", status);
  fprintf(stderr, ":: Condition:\t %i\n", thread->condition);
  fprintf(stderr, ":: Stack low:\t %p\n", thread->stack);
  fprintf(stderr, ":: Stack top:\t %p\n", &thread->stack[STACK_SIZE]);
  fprintf(stderr, ":: rsp reg:\t 0x%08" PRIu64 "x\n", thread->context.rsp);
  fflush(stderr);
}
