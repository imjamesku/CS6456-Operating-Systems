/**
 * Context switches to new_context from old_context.
 *
 * Saves the current execution context into the grn_context structure pointed to
 * by the first paramter and restores the execution context pointed to by the
 * second paramter. As such, when this function returns, it jumps to the address
 * at the top of the stack (pointed to by %rsp).
 *
 * This function has the following signature in C:
 * grn_context_switch(grn_context *old_context, grn_context *new_context)
 *
 * @param old_context pointer to where to save the current context
 * @param new_context pointer to the context to restore
 */
.globl grn_context_switch
grn_context_switch:
  // FIXME: Save the old context, restore the new.

/**
 * Initial function implicitly executed by a thread.
 *
 * This function should be jumped into implicitly by the green threads library.
 * It expects a function pointer at the top of the stack and subsequently calls
 * that function. When that function returns, this function calls grn_exit. It
 * does not expect grn_exit to return. If it does, this function loops
 * infinitely.
 *
 * @param fn [expected at top of stack] a function to call
 */
.globl start_thread
start_thread:
  push    %rbp
  mov     %rsp, %rbp
  mov     0x8(%rbp), %r11
  callq   *%r11
  callq   _grn_exit
loop:
  jmp     loop
