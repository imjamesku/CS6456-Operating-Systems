#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

#include "test.h"
#include "thread.h"
#include "floral.h"

static volatile grn_thread *t1;
static volatile grn_thread *t2;

static bool one_thread_yield() {
  grn_init(false);
  check_eq(grn_yield(0), -1);
  return true;
}

static bool two_threads_yield() {
  grn_init(false);
  t1 = grn_new_thread(false);
  asm("sub $8, %%rsp\n\t"
      "mov %%rsp, %0\n\t"
      "mov %%rbp, %1\n\t"
      "add $8, %%rsp"
      : "=m" (t1->context.rsp), "=m" (t1->context.rbp)
      : :);

  t1->status = READY;

  grn_yield(0);
  check_eq(grn_current()->id, 1);
  check_eq(grn_current()->status, RUNNING);

  grn_yield(0);
  check_eq(grn_current()->id, 0);
  check_eq(t1->status, READY);
  check_eq(grn_current()->status, RUNNING);

  grn_yield(0);
  check_eq(grn_current()->id, 1);
  check_eq(grn_current()->status, RUNNING);

  grn_yield(0);
  check_eq(grn_current()->id, 0);
  check_eq(t1->status, READY);
  check_eq(grn_current()->status, RUNNING);

  grn_yield(0);
  check_eq(grn_current()->id, 1);

  grn_exit();
  check_eq(grn_yield(0), -1);
  check_eq(grn_current()->id, 0);

  return true;
}

static bool three_threads_yield() {
  grn_init(false);
  t1 = grn_new_thread(false);
  t2 = grn_new_thread(false);
  asm("sub $8, %%rsp\n\t"
      "mov %%rsp, %0\n\t"
      "mov %%rbp, %1\n\t"
      "add $8, %%rsp"
      : "=m" (t1->context.rsp), "=m" (t1->context.rbp)
      : :);

  t2->context = t1->context;
  t1->status = READY;
  t2->status = READY;

  // thread 0 is running

  // wait on condition 7
  grn_wait(7);

  // some other thread is running

  check_eq(grn_current()->status, RUNNING);

  // signal condition 7
  grn_yield(7);

  // thread 0 should be back

  check_eq(grn_current()->id, 0);
  check_eq(grn_current()->status, RUNNING);

  return true;
}

/**
 * The phase3 test suite. This function is declared via the BEGIN_TEST_SUITE
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
BEGIN_TEST_SUITE(phase3_tests) {
  run_test(one_thread_yield);
  run_test(two_threads_yield);
  run_test(three_threads_yield);
}
