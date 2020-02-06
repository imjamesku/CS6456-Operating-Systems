#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "test.h"
#include "thread.h"
#include "floral.h"

static grn_context a_context;
static grn_context b_context;

static bool context_switch_test() {
  uint64_t r12, r13, r14, r15, rsp;
  srand(time(NULL));

  b_context.r12 = rand();
  b_context.r13 = rand();
  b_context.r14 = rand();
  b_context.r15 = rand();

  asm("sub $8, %%rsp\n\t"
      "mov %%rsp, %0\n\t"
      "mov %%rbp, %1\n\t"
      "add $8, %%rsp"
      : "=m" (b_context.rsp), "=m" (b_context.rbp)
      : :);

  grn_context_switch(&a_context, &b_context);

  asm("mov %%r12, %0\n\t"
      "mov %%r13, %1\n\t"
      "mov %%r14, %2\n\t"
      "mov %%r15, %3\n\t"
      "mov %%rsp, %4\n\t"
      : "=m" (r12), "=m" (r13), "=m" (r14), "=m" (r15), "=m" (rsp)
      : :);

  // check restoration
  check_eq(r12, b_context.r12);
  check_eq(r13, b_context.r13);
  check_eq(r14, b_context.r14);
  check_eq(r15, b_context.r15);
  check_eq(rsp, b_context.rsp + 8);

  asm("add $1, %%r12\n\t"
      "add $2, %%r14\n\t"
      "mov %%r12, %%r13\n\t"
      "mov %%r14, %%r15"
      : : : "r13", "r15");

  // check remainder of saving
  grn_context_switch(&b_context, &a_context);
  check_eq(b_context.r12, b_context.r13);
  check_eq(b_context.r14, b_context.r15);
  return true;
}

/**
 * The phase2 test suite. This function is declared via the BEGIN_TEST_SUITE
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
BEGIN_TEST_SUITE(phase2_tests) {
  run_test(context_switch_test);
}
