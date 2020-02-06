#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

#include "test.h"
#include "thread.h"
#include "main.h"
#include "floral.h"

static grn_thread *thread_from_id(int64_t id) {
  grn_thread *thread;
  for (thread = STATE.threads; thread != NULL; thread = thread->next) {
    if (thread->id == id) return thread;
  }

  return NULL;
}

static bool test_gc() {
  const int NUM = 64;
  int64_t ids[NUM];

  // Initialize
  grn_init(false);

  for (int i = 0; i < NUM; ++i) {
    grn_thread *t = grn_new_thread(true);
    check(t);
    ids[i] = t->id;
  }

  // Set every other thread as zombie
  for (int i = 0; i < NUM; i += 2) {
    grn_thread *t = thread_from_id(ids[i]);
    check_neq(t, NULL);
    check_neq(t->status, ZOMBIE);
    t->status = ZOMBIE;
  }

  // Destroy those threads
  grn_gc();

  // Ensure they're not around any more
  for (int i = 0; i < NUM; i += 2) {
    grn_thread *t = thread_from_id(ids[i]);
    check_eq(t, NULL);
  }

  // Ensure the other ones are
  for (int i = 1; i < NUM; i += 2) {
    grn_thread *t = thread_from_id(ids[i]);
    check_neq(t, NULL);
    check_neq(t->status, ZOMBIE);
  }

  // Now kill those and ensure they're gone
  for (int i = 1; i < NUM; i += 2) {
    grn_thread *t = thread_from_id(ids[i]);
    check_neq(t, NULL);
    t->status = ZOMBIE;
    grn_gc();

    t = thread_from_id(ids[i]);
    check_eq(t, NULL);
  }

  return true;
}

/**
 * The phase5 test suite. This function is declared via the BEGIN_TEST_SUITE
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
BEGIN_TEST_SUITE(phase5_tests) {
  run_test(test_gc);
}
