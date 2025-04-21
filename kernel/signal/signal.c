/**
 * signal.c - Horizon kernel signal implementation
 *
 * This file contains the implementation of the signal subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/signal.h>
#include <horizon/task.h>
#include <horizon/string.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Initialize the signal subsystem
 */
void signal_init(void) {
    /* Initialize the signal subsystem */
    /* Initialize signal structures */
}

/**
 * Get pending signals for the current task
 *
 * @param set Signal set to fill with pending signals
 */
void signal_get_pending(sigset_t *set) {
    /* Check parameters */
    if (set == NULL) {
        return;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        *set = 0; /* Empty set */
        return;
    }

    /* Get the pending signals */
    *set = task->pending.signal;
}

/**
 * Clear a pending signal for the current task
 *
 * @param sig Signal number to clear
 */
void signal_clear_pending(int sig) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return;
    }

    /* Clear the signal */
    task->pending.signal &= ~(1ULL << (sig - 1));
}

/**
 * Return from signal handler and cleanup stack frame
 *
 * @return 0 on success, negative error code on failure
 */
int signal_sigreturn(void) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Restore the saved signal mask */
    thread->signal_mask = thread->saved_signal_mask;

    /* Return from the signal handler */
    return 0;
}

/**
 * Synchronously wait for queued signals
 *
 * @param uthese Set of signals to wait for
 * @param uinfo Signal information structure to fill
 * @return Signal number on success, negative error code on failure
 */
int signal_sigwaitinfo(const sigset_t *uthese, siginfo_t *uinfo) {
    /* Check parameters */
    if (uthese == NULL) {
        return -1;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Save the old signal mask */
    sigset_t oldmask = thread->signal_mask;

    /* Block all signals except those we're waiting for */
    thread->signal_mask = ~(*uthese);

    /* Wait for a signal */
    int sig = signal_wait(task, uthese, uinfo);

    /* Restore the old signal mask */
    thread->signal_mask = oldmask;

    return sig;
}

/**
 * Synchronously wait for queued signals with timeout
 *
 * @param uthese Set of signals to wait for
 * @param uinfo Signal information structure to fill
 * @param uts Timeout
 * @return Signal number on success, negative error code on failure
 */
int signal_sigtimedwait(const sigset_t *uthese, siginfo_t *uinfo, const struct timespec *uts) {
    /* Check parameters */
    if (uthese == NULL) {
        return -1;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Save the old signal mask */
    sigset_t oldmask = thread->signal_mask;

    /* Block all signals except those we're waiting for */
    thread->signal_mask = ~(*uthese);

    /* Wait for a signal with timeout */
    int sig = signal_timedwait(task, uthese, uinfo, uts);

    /* Restore the old signal mask */
    thread->signal_mask = oldmask;

    return sig;
}

/**
 * Send a signal to a process
 *
 * @param pid Process ID
 * @param sig Signal number
 * @return 0 on success, negative error code on failure
 */
int signal_kill(pid_t pid, int sig) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -1;
    }

    /* Get the task */
    task_struct_t *task = task_get(pid);

    if (task == NULL) {
        return -1;
    }

    /* Create signal info */
    siginfo_t info;
    memset(&info, 0, sizeof(siginfo_t));
    info.si_signo = sig;
    info.si_code = SI_USER;
    info._sifields._kill.si_pid = task_current()->pid;
    info._sifields._kill.si_uid = task_current()->uid;

    /* Send the signal */
    return signal_send_info(task, sig, &info);
}

/**
 * Send a signal to a thread
 *
 * @param tid Thread ID
 * @param sig Signal number
 * @return 0 on success, negative error code on failure
 */
int signal_tkill(pid_t tid, int sig) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -1;
    }

    /* Get the task */
    task_struct_t *task = task_get(tid);

    if (task == NULL) {
        return -1;
    }

    /* Get the thread */
    thread_t *thread = task_get_thread(task, tid);

    if (thread == NULL) {
        return -1;
    }

    /* Create signal info */
    siginfo_t info;
    memset(&info, 0, sizeof(siginfo_t));
    info.si_signo = sig;
    info.si_code = SI_TKILL;
    info._sifields._kill.si_pid = task_current()->pid;
    info._sifields._kill.si_uid = task_current()->uid;

    /* Send the signal */
    return signal_send_info_thread(thread, sig, &info);
}

/**
 * Send a signal to a specific thread in a thread group
 *
 * @param tgid Thread group ID
 * @param tid Thread ID
 * @param sig Signal number
 * @return 0 on success, negative error code on failure
 */
int signal_tgkill(pid_t tgid, pid_t tid, int sig) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -1;
    }

    /* Get the task */
    task_struct_t *task = task_get(tid);

    if (task == NULL) {
        return -1;
    }

    /* Check if the task is in the thread group */
    if (task->tgid != tgid) {
        return -1;
    }

    /* Get the thread */
    thread_t *thread = task_get_thread(task, tid);

    if (thread == NULL) {
        return -1;
    }

    /* Create signal info */
    siginfo_t info;
    memset(&info, 0, sizeof(siginfo_t));
    info.si_signo = sig;
    info.si_code = SI_TKILL;
    info._sifields._kill.si_pid = task_current()->pid;
    info._sifields._kill.si_uid = task_current()->uid;

    /* Send the signal */
    return signal_send_info_thread(thread, sig, &info);
}

/**
 * Change the action taken by a process on receipt of a specific signal
 *
 * @param sig Signal number
 * @param act New signal action
 * @param oact Old signal action
 * @return 0 on success, negative error code on failure
 */
int signal_sigaction(int sig, const struct sigaction *act, struct sigaction *oact) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -1;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Get the old action */
    if (oact != NULL && thread->sigactions != NULL) {
        memcpy(oact, &thread->sigactions[sig], sizeof(struct sigaction));
    }

    /* Set the new action */
    if (act != NULL) {
        /* Allocate signal actions if not already allocated */
        if (thread->sigactions == NULL) {
            thread->sigactions = kmalloc(sizeof(struct sigaction) * SIGRTMAX, 0);
            if (thread->sigactions == NULL) {
                return -1;
            }
            memset(thread->sigactions, 0, sizeof(struct sigaction) * SIGRTMAX);
        }

        memcpy(&thread->sigactions[sig], act, sizeof(struct sigaction));
    }

    return 0;
}

/**
 * Change the action taken by a process on receipt of a specific signal (with sigset size)
 *
 * @param sig Signal number
 * @param act New signal action
 * @param oact Old signal action
 * @param sigsetsize Size of signal set
 * @return 0 on success, negative error code on failure
 */
int signal_rt_sigaction(int sig, const struct sigaction *act, struct sigaction *oact, size_t sigsetsize) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -1;
    }

    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigaction */
    return signal_sigaction(sig, act, oact);
}

/**
 * Change the signal mask of the calling process
 *
 * @param how How to change the mask (SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK)
 * @param set New signal mask
 * @param oldset Old signal mask
 * @return 0 on success, negative error code on failure
 */
int signal_sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Get the old mask */
    if (oldset != NULL) {
        *oldset = thread->signal_mask;
    }

    /* Set the new mask */
    if (set != NULL) {
        switch (how) {
            case SIG_BLOCK:
                /* Block signals */
                thread->signal_mask |= *set;
                break;

            case SIG_UNBLOCK:
                /* Unblock signals */
                thread->signal_mask &= ~(*set);
                break;

            case SIG_SETMASK:
                /* Set the signal mask */
                thread->signal_mask = *set;
                break;

            default:
                return -1;
        }
    }

    return 0;
}

/**
 * Change the signal mask of the calling process (with sigset size)
 *
 * @param how How to change the mask (SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK)
 * @param set New signal mask
 * @param oldset Old signal mask
 * @param sigsetsize Size of signal set
 * @return 0 on success, negative error code on failure
 */
int signal_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset, size_t sigsetsize) {
    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigprocmask */
    return signal_sigprocmask(how, set, oldset);
}

/**
 * Examine pending signals
 *
 * @param set Signal set to fill with pending signals
 * @return 0 on success, negative error code on failure
 */
int signal_sigpending(sigset_t *set) {
    /* Check parameters */
    if (set == NULL) {
        return -1;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Get the pending signals */
    *set = task->pending.signal & ~thread->signal_mask;

    return 0;
}

/**
 * Examine pending signals (with sigset size)
 *
 * @param set Signal set to fill with pending signals
 * @param sigsetsize Size of signal set
 * @return 0 on success, negative error code on failure
 */
int signal_rt_sigpending(sigset_t *set, size_t sigsetsize) {
    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigpending */
    return signal_sigpending(set);
}

/**
 * Wait for a signal
 *
 * @param mask Signal mask to use while waiting
 * @return -1 with errno set to EINTR
 */
int signal_sigsuspend(const sigset_t *mask) {
    /* Check parameters */
    if (mask == NULL) {
        return -1;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Save the old mask */
    sigset_t oldmask = thread->signal_mask;

    /* Set the new mask */
    thread->signal_mask = *mask;

    /* Wait for a signal */
    sched_block_thread(thread);

    /* Restore the old mask */
    thread->signal_mask = oldmask;

    return -1; /* Always returns -1 with errno set to EINTR */
}

/**
 * Wait for a signal (with sigset size)
 *
 * @param mask Signal mask to use while waiting
 * @param sigsetsize Size of signal set
 * @return -1 with errno set to EINTR
 */
int signal_rt_sigsuspend(const sigset_t *mask, size_t sigsetsize) {
    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigsuspend */
    return signal_sigsuspend(mask);
}

/**
 * Set and/or get signal stack context
 *
 * @param ss New signal stack
 * @param oss Old signal stack
 * @return 0 on success, negative error code on failure
 */
int signal_sigaltstack(const stack_t *ss, stack_t *oss) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Get the old stack */
    if (oss != NULL) {
        /* If no alternate stack is defined, return default values */
        if (thread->user_stack == NULL) {
            oss->ss_sp = NULL;
            oss->ss_size = 0;
            oss->ss_flags = SS_DISABLE;
        } else {
            oss->ss_sp = thread->user_stack;
            oss->ss_size = 0; /* Unknown size */
            oss->ss_flags = 0;
        }
    }

    /* Set the new stack */
    if (ss != NULL) {
        /* Check if the stack is valid */
        if (ss->ss_flags & ~SS_DISABLE) {
            return -1;
        }

        /* Check if the stack is disabled */
        if (ss->ss_flags & SS_DISABLE) {
            thread->user_stack = NULL;
        } else {
            /* Set the stack */
            thread->user_stack = ss->ss_sp;
        }
    }

    return 0;
}

/**
 * Queue a signal and data to a process
 *
 * @param pid Process ID
 * @param sig Signal number
 * @param uinfo Signal information
 * @return 0 on success, negative error code on failure
 */
int signal_rt_sigqueueinfo(pid_t pid, int sig, siginfo_t *uinfo) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -1;
    }

    /* Check parameters */
    if (uinfo == NULL) {
        return -1;
    }

    /* Get the task */
    task_struct_t *task = task_get(pid);

    if (task == NULL) {
        return -1;
    }

    /* Queue the signal */
    return signal_queue(task, sig, uinfo);
}

/**
 * Synchronously wait for queued signals with timeout and sigset size
 *
 * @param uthese Set of signals to wait for
 * @param uinfo Signal information structure to fill
 * @param uts Timeout
 * @param sigsetsize Size of signal set
 * @return Signal number on success, negative error code on failure
 */
int signal_rt_sigtimedwait(const sigset_t *uthese, siginfo_t *uinfo, const struct timespec *uts, size_t sigsetsize) {
    /* Check parameters */
    if (uthese == NULL) {
        return -1;
    }

    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigtimedwait */
    return signal_sigtimedwait(uthese, uinfo, uts);
}

/**
 * Return from signal handler and cleanup stack frame (with rt prefix)
 *
 * @return 0 on success, negative error code on failure
 */
int signal_rt_sigreturn(void) {
    /* Call the normal sigreturn */
    return signal_sigreturn();
}

/**
 * Wait for signal
 *
 * @return -1 with errno set to EINTR
 */
int signal_pause(void) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the current thread */
    thread_t *thread = thread_self();

    if (thread == NULL) {
        return -1;
    }

    /* Wait for a signal */
    sched_block_thread(thread);

    return -1; /* Always returns -1 with errno set to EINTR */
}
