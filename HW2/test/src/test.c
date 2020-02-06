/**
 * @file
 *
 * The unit tests execution binary.
 *
 * @author Sergio Benitez
 * @date 09/03/2014
 */

#include "test.h"

/**
 * The main function. Simply calls the test utils begin_testing function, which
 * sets up the testing world and calls the _main_test_function.
 *
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 *
 * @return 0 on success, nonzero otherwise
 */
int main(int argc, const char *argv[]) {
  return begin_testing(argc, argv);
}

/**
 * The _main_test_function. This function is delcared via the BEGIN_TESTING
 * macro for easy testing.
 *
 * The main test function takes one parameter as a pointer: the result. The main
 * test function should set result to true if all test suites pass (which
 * implies all tests pass). If not all test suites pass, it should set result to
 * false.
 *
 * The helper macro/function run_suite can manage the pointer's state when the
 * main test function function is declared using the appropriate parameter names
 * as is done by BEGIN_TESTING. The function must be named _main_test_function.
 *
 * @param[out] _result Set to true if all test suites pass, false otherwise.
 */
BEGIN_TESTING {
  run_suite(phase1_tests);
  run_suite(phase2_tests);
  run_suite(phase3_tests);
  run_suite(phase4_tests);
  run_suite(phase5_tests);

  // TODO: Uncomment this to test the extra credit phase.
  // run_suite(phase6_tests);
}
