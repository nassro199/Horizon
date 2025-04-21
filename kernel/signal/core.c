/**
 * core.c - Horizon kernel signal core implementation
 * 
 * This file contains the core implementation of the signal subsystem.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/signal.h>
#include <horizon/task.h>
#include <horizon/thread.h>
#include <horizon/sched.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/errno.h>

/* Define NULL if not defined */
#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Set up signal handling for a task
 * 
 * @return 0 on success, negative error code on failure
 */
int signal_setup(void) {
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Initialize pending signals */
    task->pending.signal = 0;
    INIT_LIST_HEAD(&task->pending.list);
    
    return 0;
}

/**
 * Send a signal to a task
 * 
 * @param task Task to send signal to
 * @param sig Signal number
 * @return 0 on success, negative error code on failure
 */
int signal_send(struct task_struct *task, int sig) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Create signal info */
    siginfo_t info;
    memset(&info, 0, sizeof(siginfo_t));
    info.si_signo = sig;
    info.si_code = SI_KERNEL;
    
    /* Send the signal with info */
    return signal_send_info(task, sig, &info);
}

/**
 * Send a signal with info to a task
 * 
 * @param task Task to send signal to
 * @param sig Signal number
 * @param info Signal information
 * @return 0 on success, negative error code on failure
 */
int signal_send_info(struct task_struct *task, int sig, siginfo_t *info) {
    /* Check parameters */
    if (task == NULL || info == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Set the signal bit */
    task->pending.signal |= (1ULL << (sig - 1));
    
    /* Queue the signal */
    return signal_queue(task, sig, info);
}

/**
 * Send a signal to a thread
 * 
 * @param thread Thread to send signal to
 * @param sig Signal number
 * @return 0 on success, negative error code on failure
 */
int signal_send_thread(struct thread *thread, int sig) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Create signal info */
    siginfo_t info;
    memset(&info, 0, sizeof(siginfo_t));
    info.si_signo = sig;
    info.si_code = SI_KERNEL;
    
    /* Send the signal with info */
    return signal_send_info_thread(thread, sig, &info);
}

/**
 * Send a signal with info to a thread
 * 
 * @param thread Thread to send signal to
 * @param sig Signal number
 * @param info Signal information
 * @return 0 on success, negative error code on failure
 */
int signal_send_info_thread(struct thread *thread, int sig, siginfo_t *info) {
    /* Check parameters */
    if (thread == NULL || info == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Get the task */
    task_struct_t *task = thread->task;
    
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Set the signal bit */
    task->pending.signal |= (1ULL << (sig - 1));
    
    /* Queue the signal */
    return signal_queue_thread(thread, sig, info);
}

/**
 * Queue a signal to a task
 * 
 * @param task Task to queue signal to
 * @param sig Signal number
 * @param info Signal information
 * @return 0 on success, negative error code on failure
 */
int signal_queue(struct task_struct *task, int sig, siginfo_t *info) {
    /* Check parameters */
    if (task == NULL || info == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Allocate a signal queue entry */
    sigqueue_t *q = kmalloc(sizeof(sigqueue_t), 0);
    
    if (q == NULL) {
        return -ENOMEM;
    }
    
    /* Initialize the signal queue entry */
    INIT_LIST_HEAD(&q->list);
    memcpy(&q->info, info, sizeof(siginfo_t));
    
    /* Add the signal to the queue */
    list_add_tail(&q->list, &task->pending.list);
    
    /* Wake up the task if it's waiting for this signal */
    thread_t *thread;
    list_for_each_entry(thread, &task->threads, process_threads) {
        /* Check if the thread is blocked and the signal is not blocked */
        if (thread->state == THREAD_STATE_BLOCKED && 
            !(thread->signal_mask & (1ULL << (sig - 1)))) {
            /* Wake up the thread */
            sched_unblock_thread(thread);
        }
    }
    
    return 0;
}

/**
 * Queue a signal to a thread
 * 
 * @param thread Thread to queue signal to
 * @param sig Signal number
 * @param info Signal information
 * @return 0 on success, negative error code on failure
 */
int signal_queue_thread(struct thread *thread, int sig, siginfo_t *info) {
    /* Check parameters */
    if (thread == NULL || info == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Get the task */
    task_struct_t *task = thread->task;
    
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Allocate a signal queue entry */
    sigqueue_t *q = kmalloc(sizeof(sigqueue_t), 0);
    
    if (q == NULL) {
        return -ENOMEM;
    }
    
    /* Initialize the signal queue entry */
    INIT_LIST_HEAD(&q->list);
    memcpy(&q->info, info, sizeof(siginfo_t));
    
    /* Add the signal to the queue */
    list_add_tail(&q->list, &task->pending.list);
    
    /* Check if the thread is blocked and the signal is not blocked */
    if (thread->state == THREAD_STATE_BLOCKED && 
        !(thread->signal_mask & (1ULL << (sig - 1)))) {
        /* Wake up the thread */
        sched_unblock_thread(thread);
    }
    
    return 0;
}

/**
 * Dequeue a signal from a task
 * 
 * @param task Task to dequeue signal from
 * @param info Signal information to fill
 * @return Signal number on success, negative error code on failure
 */
int signal_dequeue(struct task_struct *task, siginfo_t *info) {
    /* Check parameters */
    if (task == NULL || info == NULL) {
        return -EINVAL;
    }
    
    /* Check if there are any pending signals */
    if (task->pending.signal == 0) {
        return -EAGAIN;
    }
    
    /* Find the first pending signal */
    int sig = 1;
    while (sig < SIGRTMAX) {
        if (task->pending.signal & (1ULL << (sig - 1))) {
            break;
        }
        sig++;
    }
    
    /* Check if we found a signal */
    if (sig >= SIGRTMAX) {
        return -EAGAIN;
    }
    
    /* Find the signal in the queue */
    sigqueue_t *q;
    list_for_each_entry(q, &task->pending.list, list) {
        if (q->info.si_signo == sig) {
            /* Copy the signal info */
            memcpy(info, &q->info, sizeof(siginfo_t));
            
            /* Remove the signal from the queue */
            list_del(&q->list);
            
            /* Free the signal queue entry */
            kfree(q);
            
            /* Clear the signal bit */
            task->pending.signal &= ~(1ULL << (sig - 1));
            
            return sig;
        }
    }
    
    /* Signal not found in queue, create a default one */
    memset(info, 0, sizeof(siginfo_t));
    info->si_signo = sig;
    info->si_code = SI_KERNEL;
    
    /* Clear the signal bit */
    task->pending.signal &= ~(1ULL << (sig - 1));
    
    return sig;
}

/**
 * Dequeue a signal from a thread
 * 
 * @param thread Thread to dequeue signal from
 * @param info Signal information to fill
 * @return Signal number on success, negative error code on failure
 */
int signal_dequeue_thread(struct thread *thread, siginfo_t *info) {
    /* Check parameters */
    if (thread == NULL || info == NULL) {
        return -EINVAL;
    }
    
    /* Get the task */
    task_struct_t *task = thread->task;
    
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Check if there are any pending signals that are not blocked */
    sigset_t pending = task->pending.signal & ~thread->signal_mask;
    
    if (pending == 0) {
        return -EAGAIN;
    }
    
    /* Find the first pending signal that is not blocked */
    int sig = 1;
    while (sig < SIGRTMAX) {
        if (pending & (1ULL << (sig - 1))) {
            break;
        }
        sig++;
    }
    
    /* Check if we found a signal */
    if (sig >= SIGRTMAX) {
        return -EAGAIN;
    }
    
    /* Find the signal in the queue */
    sigqueue_t *q;
    list_for_each_entry(q, &task->pending.list, list) {
        if (q->info.si_signo == sig) {
            /* Copy the signal info */
            memcpy(info, &q->info, sizeof(siginfo_t));
            
            /* Remove the signal from the queue */
            list_del(&q->list);
            
            /* Free the signal queue entry */
            kfree(q);
            
            /* Clear the signal bit */
            task->pending.signal &= ~(1ULL << (sig - 1));
            
            return sig;
        }
    }
    
    /* Signal not found in queue, create a default one */
    memset(info, 0, sizeof(siginfo_t));
    info->si_signo = sig;
    info->si_code = SI_KERNEL;
    
    /* Clear the signal bit */
    task->pending.signal &= ~(1ULL << (sig - 1));
    
    return sig;
}

/**
 * Check if a task has pending signals
 * 
 * @param task Task to check
 * @return 1 if task has pending signals, 0 otherwise
 */
int signal_pending(struct task_struct *task) {
    /* Check parameters */
    if (task == NULL) {
        return 0;
    }
    
    /* Check if there are any pending signals */
    return (task->pending.signal != 0);
}

/**
 * Check if a thread has pending signals that are not blocked
 * 
 * @param thread Thread to check
 * @return 1 if thread has pending signals that are not blocked, 0 otherwise
 */
int signal_pending_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return 0;
    }
    
    /* Get the task */
    task_struct_t *task = thread->task;
    
    if (task == NULL) {
        return 0;
    }
    
    /* Check if there are any pending signals that are not blocked */
    return ((task->pending.signal & ~thread->signal_mask) != 0);
}

/**
 * Handle pending signals for a task
 * 
 * @param task Task to handle signals for
 * @return 0 on success, negative error code on failure
 */
int signal_do_signal(struct task_struct *task) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Check if there are any pending signals */
    if (task->pending.signal == 0) {
        return 0;
    }
    
    /* Handle signals for all threads */
    thread_t *thread;
    list_for_each_entry(thread, &task->threads, process_threads) {
        signal_do_signal_thread(thread);
    }
    
    return 0;
}

/**
 * Handle pending signals for a thread
 * 
 * @param thread Thread to handle signals for
 * @return 0 on success, negative error code on failure
 */
int signal_do_signal_thread(struct thread *thread) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Get the task */
    task_struct_t *task = thread->task;
    
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Check if there are any pending signals that are not blocked */
    sigset_t pending = task->pending.signal & ~thread->signal_mask;
    
    if (pending == 0) {
        return 0;
    }
    
    /* Dequeue a signal */
    siginfo_t info;
    int sig = signal_dequeue_thread(thread, &info);
    
    if (sig < 0) {
        return 0;
    }
    
    /* Handle the signal */
    return signal_handle_thread(thread, sig);
}

/**
 * Handle a signal for a task
 * 
 * @param task Task to handle signal for
 * @param sig Signal number
 * @return 0 on success, negative error code on failure
 */
int signal_handle(struct task_struct *task, int sig) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Handle the signal for the main thread */
    if (task->main_thread != NULL) {
        return signal_handle_thread(task->main_thread, sig);
    }
    
    return -EINVAL;
}

/**
 * Handle a signal for a thread
 * 
 * @param thread Thread to handle signal for
 * @param sig Signal number
 * @return 0 on success, negative error code on failure
 */
int signal_handle_thread(struct thread *thread, int sig) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Check if the thread has signal actions */
    if (thread->sigactions == NULL) {
        /* Default action */
        if (sig == SIGKILL || sig == SIGSTOP) {
            /* Kill or stop the thread */
            thread->state = THREAD_STATE_STOPPED;
            return 0;
        }
        
        /* Ignore other signals */
        return 0;
    }
    
    /* Get the signal action */
    struct sigaction *act = &thread->sigactions[sig];
    
    /* Check the signal action */
    if (act->sa_handler == SIG_DFL) {
        /* Default action */
        if (sig == SIGKILL || sig == SIGSTOP) {
            /* Kill or stop the thread */
            thread->state = THREAD_STATE_STOPPED;
            return 0;
        }
        
        /* Ignore other signals */
        return 0;
    } else if (act->sa_handler == SIG_IGN) {
        /* Ignore the signal */
        return 0;
    } else {
        /* Call the signal handler */
        /* This would be implemented with a proper signal handler call */
        /* For now, just return success */
        return 0;
    }
}

/**
 * Change the signal mask of a task
 * 
 * @param task Task to change signal mask for
 * @param how How to change the mask (SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK)
 * @param set New signal mask
 * @param oldset Old signal mask
 * @return 0 on success, negative error code on failure
 */
int signal_mask(struct task_struct *task, int how, const sigset_t *set, sigset_t *oldset) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Handle the signal mask for the main thread */
    if (task->main_thread != NULL) {
        return signal_mask_thread(task->main_thread, how, set, oldset);
    }
    
    return -EINVAL;
}

/**
 * Change the signal mask of a thread
 * 
 * @param thread Thread to change signal mask for
 * @param how How to change the mask (SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK)
 * @param set New signal mask
 * @param oldset Old signal mask
 * @return 0 on success, negative error code on failure
 */
int signal_mask_thread(struct thread *thread, int how, const sigset_t *set, sigset_t *oldset) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
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
                return -EINVAL;
        }
    }
    
    return 0;
}

/**
 * Change the signal action of a task
 * 
 * @param task Task to change signal action for
 * @param sig Signal number
 * @param act New signal action
 * @param oldact Old signal action
 * @return 0 on success, negative error code on failure
 */
int signal_action(struct task_struct *task, int sig, const struct sigaction *act, struct sigaction *oldact) {
    /* Check parameters */
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Handle the signal action for the main thread */
    if (task->main_thread != NULL) {
        return signal_action_thread(task->main_thread, sig, act, oldact);
    }
    
    return -EINVAL;
}

/**
 * Change the signal action of a thread
 * 
 * @param thread Thread to change signal action for
 * @param sig Signal number
 * @param act New signal action
 * @param oldact Old signal action
 * @return 0 on success, negative error code on failure
 */
int signal_action_thread(struct thread *thread, int sig, const struct sigaction *act, struct sigaction *oldact) {
    /* Check parameters */
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Get the old action */
    if (oldact != NULL && thread->sigactions != NULL) {
        memcpy(oldact, &thread->sigactions[sig], sizeof(struct sigaction));
    }
    
    /* Set the new action */
    if (act != NULL) {
        /* Allocate signal actions if not already allocated */
        if (thread->sigactions == NULL) {
            thread->sigactions = kmalloc(sizeof(struct sigaction) * SIGRTMAX, 0);
            if (thread->sigactions == NULL) {
                return -ENOMEM;
            }
            memset(thread->sigactions, 0, sizeof(struct sigaction) * SIGRTMAX);
        }
        
        memcpy(&thread->sigactions[sig], act, sizeof(struct sigaction));
    }
    
    return 0;
}

/**
 * Wait for a signal
 * 
 * @param task Task to wait for signal
 * @param set Set of signals to wait for
 * @param info Signal information to fill
 * @return Signal number on success, negative error code on failure
 */
int signal_wait(struct task_struct *task, const sigset_t *set, siginfo_t *info) {
    /* Check parameters */
    if (task == NULL || set == NULL) {
        return -EINVAL;
    }
    
    /* Handle the signal wait for the main thread */
    if (task->main_thread != NULL) {
        return signal_wait_thread(task->main_thread, set, info);
    }
    
    return -EINVAL;
}

/**
 * Wait for a signal
 * 
 * @param thread Thread to wait for signal
 * @param set Set of signals to wait for
 * @param info Signal information to fill
 * @return Signal number on success, negative error code on failure
 */
int signal_wait_thread(struct thread *thread, const sigset_t *set, siginfo_t *info) {
    /* Check parameters */
    if (thread == NULL || set == NULL) {
        return -EINVAL;
    }
    
    /* Get the task */
    task_struct_t *task = thread->task;
    
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Check if there are any pending signals that we're waiting for */
    sigset_t pending = task->pending.signal & *set;
    
    if (pending != 0) {
        /* Find the first pending signal that we're waiting for */
        int sig = 1;
        while (sig < SIGRTMAX) {
            if (pending & (1ULL << (sig - 1))) {
                break;
            }
            sig++;
        }
        
        /* Check if we found a signal */
        if (sig < SIGRTMAX) {
            /* Dequeue the signal */
            siginfo_t siginfo;
            int ret = signal_dequeue(task, &siginfo);
            
            if (ret < 0) {
                return ret;
            }
            
            /* Copy the signal info */
            if (info != NULL) {
                memcpy(info, &siginfo, sizeof(siginfo_t));
            }
            
            return sig;
        }
    }
    
    /* No pending signals, block the thread */
    sched_block_thread(thread);
    
    /* Thread has been woken up, check for signals again */
    return signal_wait_thread(thread, set, info);
}

/**
 * Wait for a signal with timeout
 * 
 * @param task Task to wait for signal
 * @param set Set of signals to wait for
 * @param info Signal information to fill
 * @param timeout Timeout
 * @return Signal number on success, negative error code on failure
 */
int signal_timedwait(struct task_struct *task, const sigset_t *set, siginfo_t *info, const struct timespec *timeout) {
    /* Check parameters */
    if (task == NULL || set == NULL) {
        return -EINVAL;
    }
    
    /* Handle the signal wait for the main thread */
    if (task->main_thread != NULL) {
        return signal_timedwait_thread(task->main_thread, set, info, timeout);
    }
    
    return -EINVAL;
}

/**
 * Wait for a signal with timeout
 * 
 * @param thread Thread to wait for signal
 * @param set Set of signals to wait for
 * @param info Signal information to fill
 * @param timeout Timeout
 * @return Signal number on success, negative error code on failure
 */
int signal_timedwait_thread(struct thread *thread, const sigset_t *set, siginfo_t *info, const struct timespec *timeout) {
    /* Check parameters */
    if (thread == NULL || set == NULL) {
        return -EINVAL;
    }
    
    /* Get the task */
    task_struct_t *task = thread->task;
    
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Check if there are any pending signals that we're waiting for */
    sigset_t pending = task->pending.signal & *set;
    
    if (pending != 0) {
        /* Find the first pending signal that we're waiting for */
        int sig = 1;
        while (sig < SIGRTMAX) {
            if (pending & (1ULL << (sig - 1))) {
                break;
            }
            sig++;
        }
        
        /* Check if we found a signal */
        if (sig < SIGRTMAX) {
            /* Dequeue the signal */
            siginfo_t siginfo;
            int ret = signal_dequeue(task, &siginfo);
            
            if (ret < 0) {
                return ret;
            }
            
            /* Copy the signal info */
            if (info != NULL) {
                memcpy(info, &siginfo, sizeof(siginfo_t));
            }
            
            return sig;
        }
    }
    
    /* No pending signals, block the thread with timeout */
    if (timeout != NULL) {
        /* Calculate wakeup time */
        u64 wakeup_time = get_timestamp() + timeout->tv_sec * 1000000000ULL + timeout->tv_nsec;
        
        /* Set wakeup time */
        thread->wakeup_time = wakeup_time;
        
        /* Block the thread */
        sched_block_thread(thread);
        
        /* Thread has been woken up, check if it was due to timeout */
        if (get_timestamp() >= wakeup_time) {
            return -EAGAIN;
        }
    } else {
        /* Block the thread indefinitely */
        sched_block_thread(thread);
    }
    
    /* Thread has been woken up, check for signals again */
    return signal_timedwait_thread(thread, set, info, NULL);
}

/**
 * Send a signal to a process with a value
 * 
 * @param pid Process ID
 * @param sig Signal number
 * @param value Signal value
 * @return 0 on success, negative error code on failure
 */
int signal_sigqueue(pid_t pid, int sig, const union sigval value) {
    /* Check if the signal is valid */
    if (sig < 0 || sig >= SIGRTMAX) {
        return -EINVAL;
    }
    
    /* Get the task */
    task_struct_t *task = task_get(pid);
    
    if (task == NULL) {
        return -ESRCH;
    }
    
    /* Create signal info */
    siginfo_t info;
    memset(&info, 0, sizeof(siginfo_t));
    info.si_signo = sig;
    info.si_code = SI_QUEUE;
    info._sifields._rt.si_pid = task_current()->pid;
    info._sifields._rt.si_uid = task_current()->uid;
    info._sifields._rt.si_sigval = value;
    
    /* Send the signal */
    return signal_send_info(task, sig, &info);
}

/**
 * Wait for a signal
 * 
 * @param set Set of signals to wait for
 * @param sig Signal number to fill
 * @return 0 on success, negative error code on failure
 */
int signal_sigwait(const sigset_t *set, int *sig) {
    /* Check parameters */
    if (set == NULL || sig == NULL) {
        return -EINVAL;
    }
    
    /* Get the current task */
    task_struct_t *task = task_current();
    
    if (task == NULL) {
        return -EINVAL;
    }
    
    /* Get the current thread */
    thread_t *thread = thread_self();
    
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /* Wait for a signal */
    siginfo_t info;
    int ret = signal_wait_thread(thread, set, &info);
    
    if (ret < 0) {
        return ret;
    }
    
    /* Set the signal number */
    *sig = ret;
    
    return 0;
}
