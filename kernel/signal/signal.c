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

/* Initialize the signal subsystem */
void signal_init(void) {
    /* Initialize the signal subsystem */
    signalfd_init();
}

/* Get pending signals */
void signal_get_pending(sigset_t *set) {
    /* Check parameters */
    if (set == NULL) {
        return;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        sigemptyset(set);
        return;
    }

    /* Get the pending signals */
    memcpy(set, &task->sigpending, sizeof(sigset_t));
}

/* Clear a pending signal */
void signal_clear_pending(int sig) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= _NSIG) {
        return;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return;
    }

    /* Clear the signal */
    sigdelset(&task->sigpending, sig);
}

/* Return from signal handler and cleanup stack frame */
int signal_sigreturn(void) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Return from the signal handler */
    return task_signal_return(task);
}

/* Synchronously wait for queued signals */
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

    /* Wait for the signals */
    return task_signal_timedwait(task, uthese, uinfo, NULL);
}

/* Synchronously wait for queued signals */
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

    /* Wait for the signals */
    return task_signal_timedwait(task, uthese, uinfo, uts);
}

/* Send a signal to a process */
int signal_kill(pid_t pid, int sig) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= _NSIG) {
        return -1;
    }

    /* Get the task */
    task_struct_t *task = task_get(pid);

    if (task == NULL) {
        return -1;
    }

    /* Send the signal */
    return task_signal(task, sig);
}

/* Send a signal to a thread */
int signal_tkill(pid_t tid, int sig) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= _NSIG) {
        return -1;
    }

    /* Get the task */
    task_struct_t *task = task_get(tid);

    if (task == NULL) {
        return -1;
    }

    /* Send the signal */
    return task_signal(task, sig);
}

/* Send a signal to a specific thread in a thread group */
int signal_tgkill(pid_t tgid, pid_t tid, int sig) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= _NSIG) {
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

    /* Send the signal */
    return task_signal(task, sig);
}

/* Change the action taken by a process on receipt of a specific signal */
int signal_sigaction(int sig, const struct sigaction *act, struct sigaction *oact) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= _NSIG) {
        return -1;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the old action */
    if (oact != NULL) {
        memcpy(oact, &task->sigaction[sig], sizeof(struct sigaction));
    }

    /* Set the new action */
    if (act != NULL) {
        memcpy(&task->sigaction[sig], act, sizeof(struct sigaction));
    }

    return 0;
}

/* Change the action taken by a process on receipt of a specific signal (with sigset size) */
int signal_rt_sigaction(int sig, const struct sigaction *act, struct sigaction *oact, size_t sigsetsize) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= _NSIG) {
        return -1;
    }

    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigaction */
    return signal_sigaction(sig, act, oact);
}

/* Change the signal mask of the calling process */
int signal_sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the old mask */
    if (oldset != NULL) {
        memcpy(oldset, &task->sigmask, sizeof(sigset_t));
    }

    /* Set the new mask */
    if (set != NULL) {
        switch (how) {
            case SIG_BLOCK:
                /* Block signals */
                for (int i = 0; i < _NSIG_WORDS; i++) {
                    task->sigmask.sig[i] |= set->sig[i];
                }
                break;

            case SIG_UNBLOCK:
                /* Unblock signals */
                for (int i = 0; i < _NSIG_WORDS; i++) {
                    task->sigmask.sig[i] &= ~set->sig[i];
                }
                break;

            case SIG_SETMASK:
                /* Set the signal mask */
                memcpy(&task->sigmask, set, sizeof(sigset_t));
                break;

            default:
                return -1;
        }
    }

    return 0;
}

/* Change the signal mask of the calling process (with sigset size) */
int signal_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset, size_t sigsetsize) {
    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigprocmask */
    return signal_sigprocmask(how, set, oldset);
}

/* Examine pending signals */
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

    /* Get the pending signals */
    memcpy(set, &task->sigpending, sizeof(sigset_t));

    return 0;
}

/* Examine pending signals (with sigset size) */
int signal_rt_sigpending(sigset_t *set, size_t sigsetsize) {
    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigpending */
    return signal_sigpending(set);
}

/* Wait for a signal */
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

    /* Save the old mask */
    sigset_t oldmask;
    memcpy(&oldmask, &task->sigmask, sizeof(sigset_t));

    /* Set the new mask */
    memcpy(&task->sigmask, mask, sizeof(sigset_t));

    /* Wait for a signal */
    task_wait_signal(task);

    /* Restore the old mask */
    memcpy(&task->sigmask, &oldmask, sizeof(sigset_t));

    return -1; /* Always returns -1 with errno set to EINTR */
}

/* Wait for a signal (with sigset size) */
int signal_rt_sigsuspend(const sigset_t *mask, size_t sigsetsize) {
    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Call the normal sigsuspend */
    return signal_sigsuspend(mask);
}

/* Set and/or get signal stack context */
int signal_sigaltstack(const stack_t *ss, stack_t *oss) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Get the old stack */
    if (oss != NULL) {
        memcpy(oss, &task->sigaltstack, sizeof(stack_t));
    }

    /* Set the new stack */
    if (ss != NULL) {
        /* Check if the stack is valid */
        if (ss->ss_flags & ~SS_DISABLE) {
            return -1;
        }

        /* Check if the stack is currently in use */
        if (task->sigaltstack.ss_flags & SS_ONSTACK) {
            return -1;
        }

        /* Set the stack */
        memcpy(&task->sigaltstack, ss, sizeof(stack_t));
    }

    return 0;
}

/* Queue a signal and data to a process */
int signal_rt_sigqueueinfo(pid_t pid, int sig, siginfo_t *uinfo) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= _NSIG) {
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
    return task_signal_queue(task, sig, uinfo);
}

/* Synchronously wait for queued signals */
int signal_rt_sigtimedwait(const sigset_t *uthese, siginfo_t *uinfo, const struct timespec *uts, size_t sigsetsize) {
    /* Check parameters */
    if (uthese == NULL) {
        return -1;
    }

    /* Check if the sigset size is valid */
    if (sigsetsize != sizeof(sigset_t)) {
        return -1;
    }

    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Wait for the signals */
    return task_signal_timedwait(task, uthese, uinfo, uts);
}

/* Return from signal handler and cleanup stack frame */
int signal_rt_sigreturn(void) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Return from the signal handler */
    return task_signal_return(task);
}

/* Wait for signal */
int signal_pause(void) {
    /* Get the current task */
    task_struct_t *task = task_current();

    if (task == NULL) {
        return -1;
    }

    /* Wait for a signal */
    task_wait_signal(task);

    return -1; /* Always returns -1 with errno set to EINTR */
}
