/**
 * @file
 *
 * Utilities that ease unit testing.
 *
 * @author Sergio Benitez
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

#include "test.h"

static bool did_timeout = false;

/**
 * Sets up the testing world for the _main_test_function and calls the
 * _main_test_function.
 *
 * In particular, this function sets the KNRM, KGRN, and KRED variables to be
 * color escape sequences if we are writing to a terminal. It also prints a
 * prologue and epilogue.
 *
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 *
 * @return 0 if all tests pass, nonzero otherwise
 */
int begin_testing(int argc, const char *argv[]) {
  (void)(argc);
  (void)(argv);

  if (isatty(fileno(stdout))) {
    KNRM = KNRM_TERM;
    KGRN = KGRN_TERM;
    KRED = KRED_TERM;
  } else {
    KNRM = KGRN = KRED = "";
  }

  printf("Starting unit tests.\n\n");

  bool success = true;
  _main_test_function(&success);

  if (success) {
    printf("%sUnit testing completed successfully.%s\n", KGRN, KNRM);
  } else {
    printf("%sUnit testing failed.%s\n", KRED, KNRM);
  }

  return !success;
}

/**
 * This function is called when a test times out.
 */
static void timeout(int _s) {
  (void)(_s);
  did_timeout = true;
}

pid_t waitpid_with_timeout(pid_t pid, int *status, int sec, bool *timedout, bool *success) {
  did_timeout = false;
  *success = true;

  // A timer that fires after 'sec' seconds.
  struct itimerval itimer = {
    .it_interval = { 0, 0 },
    .it_value = { .tv_sec = sec, .tv_usec = 0 }
  };

  // the signal handler structure
  const struct sigaction timeout_action = {
    .sa_handler = timeout
  };

  // sets the signal handlers
  sigaction(SIGALRM, &timeout_action, NULL);

  // sets up the alarm
  if (setitimer(ITIMER_REAL, &itimer, NULL)) {
    fprintf(stderr, "setitimer failed in _run_test\n");
    exit(1);
  }

  pid_t r = waitpid(pid, status, 0);
  if (r == pid) {
    // cancel the timer
    struct itimerval zero_timer = { { 0, 0 }, { 0, 0 } };
    setitimer(ITIMER_REAL, &zero_timer, NULL);
    did_timeout = *timedout = false;

    if (WIFSIGNALED(*status)) {
      // abnormal signal exit
      printf(":: %s:%d: Test sigkilled: %s\n",
          __FILE__, __LINE__, strsignal(WTERMSIG(*status)));
      *success = false;
    } else {
      *success = WEXITSTATUS(*status) == EXIT_SUCCESS;
    }
  } else if (r == -1 && errno == EINTR) {
    *timedout = did_timeout;
    kill(pid, 9);

    waitpid(pid, status, 0);
    *success = false;
  } else {
    // this should never happen
    fprintf(stderr, "unhandled child exit %d (%d)\n", r, errno);
    exit(EXIT_FAILURE);
  }

  return r;
}


/**
 * Runs a given test (fn), incrementing the number of tests and number of tests
 * passed parameters accordingly. Prints a prologue and epilogue for each test.
 *
 * @param name The name of the test.
 * @param[out] _num_tests Incremented once.
 * @param[out] _num_passed Incremented if the test fn passes.
 *
 * @return true if the test passes, false otherwise
 */
bool _run_test(const char *name, bool (*fn)(void), int *ntests, int *npassed) {
  bool success, timedout;
  int child_pid, stat;
  if (!(child_pid = fork())) {
    // remove the parent's timeout handler
    const struct sigaction default_action = { .sa_handler = SIG_DFL };
    sigaction(SIGALRM, &default_action, NULL);
    exit((fn()) ? EXIT_SUCCESS : EXIT_FAILURE);
  } else {
    waitpid_with_timeout(child_pid, &stat, 5, &timedout, &success);
  }

  (*npassed) += success;
  (*ntests)++;

  const char *c = success ? KGRN : KRED;
  const char *status;
  if (timedout) {
    status = "timeout";
  } else {
    status = success ? "success" : "failed";
  }

  printf(":: %s: %s%s%s\n", name, c, status, KNRM);
  return success;
}

/**
 * Runs a given test suite (fn), setting the number of tests and number of tests
 * passed parameters accordingly. Prints a prologue and epilogue for each test
 * suite.
 *
 * @param name The name of the test suite.
 * @param[out] _num_tests Set to the number of tests run by the suite.
 * @param[out] _num_passed Set to the number of tests passed during the suite.
 *
 * @return true if the the test suite passes, false otherwise
 */
bool _run_suite(const char *name, void (*fn)(bool *, int *, int *)) {
  int num_tests = 0;
  int num_passed = 0;
  bool result = true;

  printf("Running %s...\n", name);
  fn(&result, &num_tests, &num_passed);

  const char *c = result ? KGRN : KRED;
  const char *status = result ? "success" : "failed";
  printf("%s%s: %d/%d passed%s\n\n", c, status, num_passed, num_tests, KNRM);
  return result;
}
