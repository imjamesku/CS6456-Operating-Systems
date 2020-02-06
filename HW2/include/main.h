#ifndef FLORAL_MAIN_H
#define FLORAL_MAIN_H

#include "floral.h"

/**
 * This structure keeps track of the global state for the green threads library.
 */
typedef struct floral_state_struct {
  /**
   * A pointer to the head of the linked list of threads being managed by the
   * library.
   */
  grn_thread *threads;

  /**
   * Pointer to the currently active thread.
   */
  grn_thread *current;
} floral_state;

floral_state STATE;

void grn_gc();

#endif
