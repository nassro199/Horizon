/**
 * sched_test.c - Test program for the scheduler
 *
 * This file contains a test program for the scheduler.
 */

#include <horizon/kernel.h>
#include <horizon/types.h>
#include <horizon/sched.h>
#include <horizon/thread.h>
#include <horizon/task.h>
#include <horizon/mm.h>
#include <horizon/string.h>
#include <horizon/time.h>
#include <horizon/console.h>
#include <horizon/errno.h>
#include <horizon/thread_context.h>
#include <horizon/stddef.h>

/* Test thread function 1 */
void *test_thread_1(void *arg) {
    int count = 0;

    console_printf("Test thread 1 started\n");

    while (count < 5) {
        console_printf("Test thread 1: count = %d\n", count);
        count++;

        /* Sleep for a while */
        thread_sleep(100);
    }

    console_printf("Test thread 1 finished\n");

    return NULL;
}

/* Test thread function 2 */
void *test_thread_2(void *arg) {
    int count = 0;

    console_printf("Test thread 2 started\n");

    while (count < 5) {
        console_printf("Test thread 2: count = %d\n", count);
        count++;

        /* Sleep for a while */
        thread_sleep(150);
    }

    console_printf("Test thread 2 finished\n");

    return NULL;
}

/* Scheduler test function */
void sched_test(void) {
    thread_t *thread1, *thread2;

    console_printf("Starting scheduler test...\n");

    /* Create test threads */
    thread1 = thread_create(test_thread_1, NULL, THREAD_JOINABLE);
    if (thread1 == NULL) {
        console_printf("Failed to create test thread 1\n");
        return;
    }

    thread2 = thread_create(test_thread_2, NULL, THREAD_JOINABLE);
    if (thread2 == NULL) {
        console_printf("Failed to create test thread 2\n");
        return;
    }

    /* Set thread priorities */
    thread_set_priority(thread1, THREAD_PRIO_NORMAL);
    thread_set_priority(thread2, THREAD_PRIO_NORMAL);

    /* Start threads */
    thread_start(thread1);
    thread_start(thread2);

    /* Wait for threads to finish */
    thread_join(thread1, NULL);
    thread_join(thread2, NULL);

    console_printf("Scheduler test completed\n");
}
