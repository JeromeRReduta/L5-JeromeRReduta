#include "scheduler.h"

#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include "file_format.h"

static int rr_current_i = 0;
int circ_array_go_back_by(int i, int n, int max_len);
int circ_array_go_forward_by(int i, int n, int max_len);
int spin(int n, int len);


/* Scheduler state information */
struct scheduler_state g_scheduler = { 0 };

/* Flag that gets set when we've received an interrupt */
int g_interrupted = SIGALRM;

/* Function pointer to the scheduling algorithm implementation: */
void (*g_scheduling_algorithm)(struct scheduler_state *sched_state) = NULL;

/**
 * Retrieves the current UNIX time, in seconds.
 */
double get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/**
 * Switches the running context to a different process.
 */
void context_switch(struct process_ctl_block *pcb)
{
    if (pcb->start_time == 0) {
        pcb->start_time = get_time();
    }

    pcb->state = RUNNING;
    /* Update global state variables: */
    g_scheduler.current_process = pcb;
    g_scheduler.current_quantum++;

    /* Reset our alarm "interrupt" to fire again in 1 second: */
    alarm(1);

    /* Tell the process to run */
    int ret = kill(pcb->pid, SIGCONT);
    if (ret == -1) {
        perror("context_switch");
        fprintf(stderr, "Tried to context switch to an invalid process!"
                " Exiting.\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Iterates through the list of process control blocks and checks for "arriving"
 * processes. Since we load the entire list of process executions at the start
 * of the program, we're just checking to see if a given process was supposed to
 * be created during the current quantum. If it was, we need to:
 * - Change it from CREATED to WAITING state
 * - Fork a new process
 * - Execute it with the appropriate parameters (name and workload size).
 */
void handle_arrivals(void)
{
    int i;
    for (i = 0; i < g_scheduler.num_processes; ++i) {
        if (g_scheduler.pcbs[i].creation_quantum == g_scheduler.current_quantum) {
            struct process_ctl_block *pcb = &g_scheduler.pcbs[i];
            pcb->state = WAITING;
            pcb->arrival_time = get_time();

            printf("[*] New process arrival: %s\n", pcb->name);
            pid_t child = fork();
            if (child == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (child == 0) {
                /* First, stop the child before we execute anything: */
                kill(getpid(), SIGSTOP);

                char workload_buf[128];
                snprintf(workload_buf, 128, "%d", pcb->workload);
                int retval = execl(
                        "process",
                        "process", pcb->name, workload_buf, (char *) 0);
                if (retval == -1) {
                    perror("exec");
                }
                exit(1);
            } else {
                pcb->pid = child;
                printf("[i] '%s' [pid=%d] created. Workload = %ds\n",
                        pcb->name, child, pcb->workload);
            }
        }
    }
}

/**
 * This signal handler is very minimal: it records the numeric identifier of the
 * signal that was received. The reason for this is simple: you are technically
 * *NOT* supposed to do work in a signal handler, and many functions are not
 * safe to use here. Instead, we set this flag and handle interrupt logic in
 * from our main loop.
 */
void signal_handler(int signo)
{
    g_interrupted = signo;
}

/**
 * Upon receipt of an interrupt, this function updates the current process
 * state, handles any new process arrivals, and then calls the scheduling logic
 * (a function pointed to by g_scheduling_algorithm).
 */
void interrupt_handler(void)
{
    if (g_interrupted == SIGCHLD) {
        /* A child process terminated, stopped, or continued. We aren't
         * interested in the last two states, so we need to check whether the
         * child terminated or not.*/
        int status;
        pid_t child = waitpid(-1, &status, WNOHANG);
        if (child == 0 || child == -1) {
            g_interrupted = 0;
            return;
        }

        /* A child terminated if waitpid() returned a PID. Now we need to
         * find the PCB corresponding to this PID. */
        struct process_ctl_block *pcb = NULL;
        for (int i = 0; i < g_scheduler.num_processes; ++i) {
            if(g_scheduler.pcbs[i].pid == child) {
                pcb = &g_scheduler.pcbs[i];
                break;
            }
        }

        if (pcb != NULL) {
            /* Disable any active alarm; the process already quit, so we
             * don't need to worry about interrupting it */
            alarm(0);

            pcb->state = TERMINATED;
            pcb->completion_time = get_time();
            g_interrupted = SIGALRM;
        }
    }

    if (g_interrupted == SIGALRM) {
        /* Time quantum has expired */

        g_interrupted = 0;
        printf("\t-> interrupt (%d)\n", g_scheduler.current_quantum);

        /* The process was interrupted, so we should change its state back to
         * waiting. */
        if (g_scheduler.current_process != NULL
                && g_scheduler.current_process->state == RUNNING) {
            /* Tell the process to stop running: */
            kill(g_scheduler.current_process->pid, SIGSTOP);

            /* Put it back in the wait state: */
            g_scheduler.current_process->state = WAITING;
        }

        handle_arrivals();

        g_scheduling_algorithm(&g_scheduler);
    }
}

/**
 * A basic scheduler that simply runs each process in the array of PCBs based on
 * its array index; i.e., index 0 runs first, followed by index 1, and so on.
 * This is about as far from a 'real' scheduler as you can get!
 */
void basic(struct scheduler_state *sched_state)
{
    int i;
    for (i = 0; i < sched_state->num_processes; ++i) {
        struct process_ctl_block *pcb = &sched_state->pcbs[i];
        if(pcb->state == WAITING) {
            context_switch(pcb);
            return;
        }
    }
}


void fifo(struct scheduler_state *sched_state)
{
    // Keep track of  first process ctl block and first quantum
    struct process_ctl_block *first = NULL;
    int first_quantum = INT_MAX;

    int i;
    for (i = 0; i < sched_state->num_processes; ++i) {
        struct process_ctl_block *pcb = &sched_state->pcbs[i];
        if(pcb->state == WAITING && pcb->creation_quantum < first_quantum) {
            first = pcb;
            first_quantum = pcb->creation_quantum;
        }
    }

    if (first != NULL) {
        context_switch(first);
        return;
    }


}

/**
 * @brief      Scheduling algorithm that prioritizes the process with the shortest workload
 *
 * @param      sched_state  sched state array
 */
void psjf(struct scheduler_state *sched_state)
{
    printf("\nUsing psjf\n");
    struct process_ctl_block *smallest = NULL;
    unsigned int smallest_workload = UINT_MAX;

    int i;
    for (i = 0; i < sched_state->num_processes; ++i) {
        struct process_ctl_block *pcb = &sched_state->pcbs[i];
        if(pcb->state == WAITING && pcb->workload < smallest_workload) {
            smallest = pcb;
            smallest_workload = pcb->workload;

/**
            printf("NEW STUFF:\n"
                "\t->smallest is: '%s'\n"
                "\t->smallest_workload is: '%i'\n"
                "\t->finish - start = %f\n",
                smallest == NULL ? "null" : "exists", smallest_workload, get_time() - start_time);
   */     
        }
    }

    if (smallest != NULL) {
        context_switch(smallest);
        return;
    }



}

void sjf(struct scheduler_state *sched_state)
{
    printf("\nUsing sjf\n");
    static struct process_ctl_block *current; 
    unsigned int smallest_workload = UINT_MAX;

    if (current != NULL && current->state == TERMINATED) {
        printf("'%s' is done\n", current->name);
        current = NULL;
    }

    int i;
    for (i = 0; i < sched_state->num_processes; ++i) {
        struct process_ctl_block *pcb = &sched_state->pcbs[i];
        if(pcb->state == WAITING && pcb->workload < smallest_workload && current == NULL) {
            current = pcb;
            smallest_workload = pcb->workload;
        }
    }

    if (current != NULL) {
        context_switch(current);
        return;
    }



}

void rr(struct scheduler_state *sched_state)
{
    printf("\nUsing round robin\n");

   

    struct process_ctl_block* current = NULL;

    for (int i = 0; i < sched_state->num_processes + 1; ++i) {

        int index = circ_array_go_forward_by(rr_current_i, i + 1, sched_state->num_processes);

        struct process_ctl_block *pcb = &sched_state->pcbs[index];

        printf("%s -> %s\n", (&sched_state->pcbs[index])->name, pcb->name);
        if(pcb->state == WAITING) {
            current = pcb;

            rr_current_i = index;
            context_switch(current);
            return;
        }
    }
}

void priority(struct scheduler_state *sched_state)
{
    struct process_ctl_block* curr = &sched_state->pcbs[rr_current_i];
    unsigned int max_priority = 0;
    int i;
    for (i = 0; i < sched_state->num_processes; ++i) {
        int index = circ_array_go_forward_by(rr_current_i, i + 1, sched_state->num_processes);

        struct process_ctl_block *pcb = &sched_state->pcbs[index];
        if(pcb->state == WAITING && pcb->priority > max_priority && index != rr_current_i) {


            printf("OLD PRIO: %d\n NEW PRIO: %d\n", max_priority, pcb->priority);
            max_priority = pcb->priority;
            curr = pcb;

            rr_current_i = index;

        }
    }

    if (curr != NULL && curr->state == WAITING) {

        printf("Name is: '%s'\n", curr->name);
        printf("New prio is: %d\n", curr->priority);
        context_switch(curr);
        return;
    }

}

void insanity(struct scheduler_state *sched_state)
{
    struct process_ctl_block* curr;
    bool all_done = true;

    for (int i = 0; i < sched_state->num_processes; ++i) {
        struct process_ctl_block* pcb = &sched_state->pcbs[i];

        if (pcb->state == WAITING) {
            all_done = false;
        }

    }

    if (!all_done) {

        do {
            curr = &sched_state->pcbs[spin(10, sched_state->num_processes)];
        }
        while (curr->state != WAITING);
        context_switch(curr);
        return;
    }
    

}

int spin(int n, int len)
{
    int chosen_index = 0;

    // Spin 10 times for fun!
    for (int i = 0; i < n; i++) {
        chosen_index = circ_array_go_forward_by(0, rand(), len);
    }

    return chosen_index;
}

// The following circ_array funcs have been lovingly taken from my P2 code
/**
 * @brief      Returns the index from going n indices backward in a circular array
 *
 * @param[in]  i        Starting index
 * @param[in]  n        # of indices to go backward
 * @param[in]  max_len  max length of circular array
 *
 * @return     Index from going n indices backward from index i
 */

int circ_array_go_back_by(int i, int n, int max_len)
{
    return abs((100 + i - n) % max_len);

}

/**
 * @brief      Returns the index from going n indices forward in a circular array
 *
 * @param[in]  i        Starting index
 * @param[in]  n        # of indices to go foward
 * @param[in]  max_len  max length of circular array
 *
 * @return     Index from going n indices forward from index i
 */
int circ_array_go_forward_by(int i, int n, int max_len)
{
    return abs((100 + i + n) % max_len);

}





int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <process-specification>\n", argv[0]);
        return 1;
    }

    read_spec(argv[1], &g_scheduler);

    /* Set up our interrupt. Instead of a hardware interrupt, we'll be using
     * signals, a type of software interrupt. It will fire every 1 second. */
    signal(SIGALRM, signal_handler);
    signal(SIGCHLD, signal_handler);

    g_scheduling_algorithm = &insanity;
    fprintf(stderr, "[i] Ready to start\n");

    while (true) {
        if (g_interrupted != 0) {
            interrupt_handler();
        }

        int terminated = 0;
        for (int i = 0; i < g_scheduler.num_processes; ++i) {
            if (g_scheduler.pcbs[i].state == TERMINATED) {
                terminated++;
            }
        }
        if (terminated == g_scheduler.num_processes) {
            /* All processes have terminated */
            break;
        }

        /* Stop execution until we receive a signal: */
        pause();
    }

    printf("\nExecution complete.\n");
    return 0;
}
