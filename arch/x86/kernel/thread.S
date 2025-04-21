/**
 * thread.S - Thread entry point for x86 architecture
 *
 * This file contains the thread entry point for the x86 architecture.
 */

.section .text
.global thread_entry
.type thread_entry, @function

/**
 * Thread entry point
 *
 * This function is the entry point for new threads.
 * It sets up the stack and calls the thread function.
 */
thread_entry:
    /* Get the thread function and argument */
    popl %ebx      /* Thread function */
    popl %eax      /* Thread argument */

    /* Call the thread function */
    pushl %eax     /* Argument */
    call *%ebx     /* Function */

    /* Thread function returned, call thread_exit */
    pushl %eax     /* Return value */
    call thread_exit

    /* Should never reach here */
    hlt
    jmp thread_entry

.size thread_entry, . - thread_entry
