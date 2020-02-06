#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

#include "test.h"
#include "thread.h"
#include "main.h"

static grn_thread *thread_from_id(int64_t id) {
  grn_thread *thread;
  for (thread = STATE.threads; thread != NULL; thread = thread->next) {
    if (thread->id == id) return thread;
  }

  return NULL;
}

static bool check_ids() {
  const int NUM = 512;
  grn_thread *threads[NUM];

  for (int i = 0; i < NUM; ++i) {
    grn_thread *t = grn_new_thread(false);
    check(t);
    check_eq(t->id, i);
    threads[i] = t;
  }

  // Free resources
  for (int i = 0; i < NUM; ++i) {
    grn_destroy_thread(threads[i]);
  }

  return true;
}

static bool check_status() {
  const int NUM = 512;
  grn_thread *threads[NUM];

  for (int i = 0; i < NUM; ++i) {
    grn_thread *t = grn_new_thread(false);
    check(t);
    check_eq(t->status, WAITING);
    threads[i] = t;
  }

  // Free resources
  for (int i = 0; i < NUM; ++i) {
    grn_destroy_thread(threads[i]);
  }

  return true;
}

static bool alloc_no_stack() {
  const int NUM = 512;
  grn_thread *threads[NUM];

  for (int i = 0; i < NUM; ++i) {
    grn_thread *t = grn_new_thread(false);
    check(t);
    check_eq(t->stack, NULL);
    threads[i] = t;
  }

  // Free resources
  for (int i = 0; i < NUM; ++i) {
    grn_destroy_thread(threads[i]);
  }

  return true;
}

static bool alloc_with_stack() {
  const int NUM = 64;
  grn_thread *threads[NUM];

  // Ensure each thread has some stack
  for (int i = 0; i < NUM; ++i) {
    grn_thread *t = grn_new_thread(true);
    check(t);

    check_neq(t->stack, NULL);
    check(((uint64_t) t->stack) % 16 == 0);
    threads[i] = t;
  }

  // Ensure each stack is unique
  for (int i = 0; i < NUM; ++i) {
    grn_thread *thread1 = threads[i];
    for (int j = 0; j < NUM; ++j) {
      if (i == j) continue;
      grn_thread *thread2 = threads[j];
      check_neq(thread1->stack, thread2->stack);

      uint64_t higher = (uint64_t) max(thread1->stack, thread2->stack);
      uint64_t lower = (uint64_t) min(thread1->stack, thread2->stack);
      check((higher - lower) >= STACK_SIZE);
    }
  }

  // Free resources
  for (int i = 0; i < NUM; ++i) {
    grn_destroy_thread(threads[i]);
  }

  return true;
}

static bool linked_list_membership() {
  const int NUM = 512;
  int64_t ids[NUM];

  // Allocate half the threads with a stack
  for (int i = 0; i < NUM / 2; ++i) {
    grn_thread *t = grn_new_thread(true);
    check(t);
    ids[i] = t->id;
  }

  // And another half without
  for (int i = NUM / 2; i < NUM; ++i) {
    grn_thread *t = grn_new_thread(false);
    check(t);
    ids[i] = t->id;
  }

  // Make sure we can find them in the list
  for (int i = 0; i < NUM; ++i) {
    grn_thread *t = thread_from_id(ids[i]);
    check_neq(t, NULL);
    grn_destroy_thread(t);
  }

  // Make sure we can no longer find them there
  for (int i = 0; i < NUM; ++i) {
    grn_thread *t = thread_from_id(ids[i]);
    check_eq(t, NULL);
  }

  return true;
}

/**
 * The phase1 test suite. This function is declared via the BEGIN_TEST_SUITE
 * macro for easy testing.
 *
 * A test suite function takes three parameters as pointers: the result, the
 * number of tests, and the number of tests passed. The test suite function
 * should increment the number of tests each time a test is run and the number
 * of tests passed when a test passes. If all tests pass, result should be set
 * to true. If any of the tests fail, result should be set to false.
 *
 * The helper macro/function run_test can manage the pointers' state when the
 * suite function is declared using the appropriate parameter names as is done
 * by BEGIN_TEST_SUITE.
 *
 * @param[out] _result Set to true if all tests pass, false otherwise.
 * @param[out] _num_tests Set to the number of tests run by this suite.
 * @param[out] _num_passed Set to the number of tests passed during this suite.
 */
BEGIN_TEST_SUITE(phase1_tests) {
  run_test(check_ids);
  run_test(check_status);
  run_test(alloc_no_stack);
  run_test(alloc_with_stack);
  run_test(linked_list_membership);
}
