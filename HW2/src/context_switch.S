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
 *
 * The first two parameters to a function are passed in the rdi and rsi
 */
.globl grn_context_switch
grn_context_switch:
  // FIXME: Save the old context, restore the new.
  movq     %rsp, (%rdi)
  movq     %r15, 0x8(%rdi)
  movq     %r14, 0x10(%rdi)
  movq     %r13, 0x18(%rdi)
  movq     %r12, 0x20(%rdi)
  movq     %rbx, 0x28(%rdi)
  movq     %rbp, 0x30(%rdi)
  movq     (%rsi), %rsp
  movq     0x8(%rsi), %r15
  movq     0x10(%rsi), %r14
  movq     0x18(%rsi), %r13
  movq     0x20(%rsi), %r12
  movq     0x28(%rsi), %rbx
  movq     0x30(%rsi), %rbp
  ret

  

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
