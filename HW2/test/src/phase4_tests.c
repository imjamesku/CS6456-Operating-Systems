#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include "test.h"
#include "floral.h"

#define ITER_NUM 100
#define NUM_WORK 1 << 14
static volatile int WORK_DONE[ITER_NUM] = { 0 };

static int yield_loop() {
  int i;
  for (i = 0; i < ITER_NUM; i++) {
    printf("%" PRId64 ".%d\n", grn_current()->id, i);
    grn_yield(0);
  }

  return 0;
}

/**
 * Pipes this processes stdout to a file descriptor.
 *
 * @param[out] out Set to write side of the pipe. Should be closed when done.
 * @param[out] saved_stdout Set to an fd that maps to the file description of
 * std out before redirecting it. Should be used to restore stdout.
 *
 * @return The fd for the read side of the pipe if successful, -1 otherwise.
 */
static int pipe_stdout(int *out, int *saved_stdout) {
  int o_fds[2];
  if (pipe(o_fds) < 0)
    return -1;

  if (out)
    *out = o_fds[1];

  if (saved_stdout)
    *saved_stdout = dup(fileno(stdout));

  dup2(o_fds[1], fileno(stdout));
  return o_fds[0];
}

/*
 * A preliminary test. Checks that...
 *
 * @return true if all subtests pass, false if any fail
 */
static bool test_yield_iter() {
  char *line = NULL;
  size_t length = 0;
  char expecting[64];

  // redirect to a pipe before test starts
  int in_fd, out_fd, saved_stdout;
  in_fd = pipe_stdout(&out_fd, &saved_stdout);
  check(in_fd >= 0);

  grn_init(false);
  grn_spawn(yield_loop);
  grn_spawn(yield_loop);
  grn_join();

  // Add this so that getline doesn't block when nothing was printed, i.e., when
  // the library hasn't yet been implemented.
  printf("END\n");

  // restore stdout
  dup2(saved_stdout, fileno(stdout));
  close(saved_stdout);

  // Get a stream to the pipe
  FILE *in_stream = fdopen(in_fd, "r");

  // check the output of the threads
  int i;
  for (i = 0; i < ITER_NUM * 2; ++i) {
    check(getline(&line, &length, in_stream) >= 0);
    /* printf("%s", line); */

    int thread = (i % 2) + 1, iter = i / 2;
    snprintf(expecting, 64, "%d.%d\n", thread, iter);
    check_eq_str(expecting, line);
  }

  // Pull the END line
  check(getline(&line, &length, in_stream) >= 0);
  check_eq_str("END\n", line);

  // check iteration count
  check_eq(i, ITER_NUM * 2);

  // cleanup
  free(line);
  fclose(in_stream);
  close(out_fd);
  return true;
}

static int do_work() {
  int64_t id = grn_current()->id;

  // Ensure the thread ID is valid. Otherwise, fail.
  if (id > ITER_NUM) {
    exit(EXIT_FAILURE);
  }

  // Do work by generating a random number NUM_WORK (16k) times
  srand(time(NULL));
  volatile int64_t work[NUM_WORK];

  int i;
  for (i = 0; i < NUM_WORK; ++i) {
    work[i] = rand();
    WORK_DONE[id - 1] = work[i] - 1;
    grn_yield(0);
  }

  WORK_DONE[id - 1] = i;
  return 0;
}

static bool worker_threads() {
  grn_init(false);

  // Spawn ITER_NUM worker threads
  for (int i = 0; i < ITER_NUM; ++i) {
    grn_spawn(do_work);
  }

  // Wait for them all to finish
  grn_join();

  // Ensure they all did all their work
  for (int i = 0; i < ITER_NUM; ++i) {
    check_eq(WORK_DONE[i], NUM_WORK);
  }

  return true;
}

/**
 * The phase4 test suite. This function is declared via the BEGIN_TEST_SUITE
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
BEGIN_TEST_SUITE(phase4_tests) {
  run_test(test_yield_iter);
  run_test(worker_threads);
}
