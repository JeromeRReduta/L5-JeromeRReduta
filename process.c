/**
 * @file
 *
 * This simulates a running process by sleeping for a particular amount of time
 * and printing its current progress.
 */

#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static const int update_interval = 100;
static pid_t my_pid;
static char *name;

void resume(int signal)
{
    printf("-> Executing '%s' [pid=%d]\n", name, my_pid);
}

void print_percbar(long work_left, long work_ms)
{
    static const char *bar_chars
        = "####################--------------------";
    static const int bar_sz = 20;
    double frac = 1.0 - (double) work_left / (double) work_ms;
    int perc = round(100.0 * frac);
    printf("\r%s [%.20s] %.1f%%",
            name,
            &bar_chars[bar_sz - perc / 5],
            100.0 * frac + 0.0);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s name workload\n", argv[0]);
        return 1;
    }

    name = argv[1];

    int workload = atoi(argv[2]);
    long work_ms = workload * 1000;
    long work_left = work_ms;
    my_pid = getpid();

    print_percbar(work_left, work_ms);

    while (work_left > 0) {
        int retval;
        struct timespec ts1, ts2;
        do {
            ts1.tv_sec = 0;
            ts1.tv_nsec = update_interval * 1E6;
            retval = nanosleep(&ts1, &ts2);
            ts1 = ts2;
        } while (retval != 0);
        work_left = work_left - update_interval;
        print_percbar(work_left, work_ms);
    }

    return 0;
}
