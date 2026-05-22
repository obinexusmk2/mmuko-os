// hello.c - Sample program for MMUKO time-payload scheduling.
// This program runs for a fixed duration, simulating a real workload.

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <errno.h>
#include <unistd.h>
#endif

static unsigned long hello_pid(void) {
#ifdef _WIN32
    return (unsigned long)_getpid();
#else
    return (unsigned long)getpid();
#endif
}

static void hello_sleep_ms(uint32_t ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000U);
    req.tv_nsec = (long)((ms % 1000U) * 1000000UL);

    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
        /* Keep sleeping for the remaining interval. */
    }
#endif
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("[HELLO] Process started: pid=%lu\n", hello_pid());
    printf("[HELLO] Simulating work for 3 seconds...\n");

    for (int i = 0; i < 3; i++) {
        printf("[HELLO] Working... tick %d/3\n", i + 1);
        hello_sleep_ms(1000);
    }

    printf("[HELLO] Process completed successfully.\n");
    return 0;
}
