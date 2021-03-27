# CPU Scheduling

In this lab, we'll be implementing CPU scheduling algorithms. We could actually design our own Linux kernel modules for this, but the complexity is extreme: CFS (Completely Fair Scheduling) is somewhere around 10,000 lines of code!

Instead, we will implement our scheduler using signals: `SIGSTOP` and `SIGCONT`. These two signals allow us to start and stop processes, and we'll maintain context information to know which processes should be executed and provide scheduling metrics.

Your mission for this lab is:
1. Implement the scheduling algorithms below
2. Add a new command line option that allows you to select the scheduler. For example, if you run `./scheduler fifo processes1.txt` then the FIFO algorithm will be used.
3. Scheduling metrics. After the program finishes running, you'll print some stats about the execution.

## Algorithms

You will implement the following scheduling algorithms. **NOTE**: if there are ties (i.e., two processes are both valid candidates to run), use the index order of `g_scheduler->pcbs` array.

### Basic
This scheduler gives you an example implementation to base your other algorithms off of. It simply iterates through the list of processes, finds the next one that needs to be run, and then context switches to it.

### FIFO - First In, First Out
Similar to the above, but processes are executed in the order of their arrival (first in, first out).

### SJF - Shortest Job First
The shortest process is run next, based on workload size. If a new process arrives, the currently-running process is **NOT** context switched out. This means that for each interrupt, you will simply re-run the last process until it completes.

### PSJF - Preemptive Shortest Job First 
Similar to SJF, but with preemption. If a task with a smaller overall workload arrives, you will switch to it and run it instead.

### RR - Round Robin
Each process gets a turn. Every time you receive an interrupt, switch to the next process in the list. Once you hit the end of the list, start back at the beginning.

### Priority
The process with the highest priority is run next. If two processes have the same priority, then switch between them round robin style.

### Insanity
Choose a random number to determine what process to run.


## Algorithm Selector

The second command line argument should specify the algorithm as a case-insensitive string (use the shortened versions above, if applicable -- 'rr' for Round Robin, and so on). Modify the main() function to allow switching the algorithm.

## Metrics

You will print:
* Turnaround times
* Response times
* Average turnaround and response time
* Completion order

...at the end of the program's execution. Here's an example run of the program below:

```
$ ./scheduler ./specifications/processes2.txt
Reading process specification: ./specifications/processes2.txt
[i] Generating process control block: Process_A
[i] Generating process control block: Process_B
[i] Generating process control block: Process_C
[i] Ready to start
        -> interrupt (0)
[*] New process arrival: Process_A
[i] 'Process_A' [pid=4842] created. Workload = 3s
[*] New process arrival: Process_B
[i] 'Process_B' [pid=4843] created. Workload = 1s
[*] New process arrival: Process_C
[i] 'Process_C' [pid=4844] created. Workload = 1s
Process_A [######--------------] 30.0%  -> interrupt (1)
Process_A [############--------] 63.3%  -> interrupt (2)
Process_A [###################-] 96.7%  -> interrupt (3)
Process_A [####################] 100.0% -> interrupt (4)
Process_B [##################--] 90.0%  -> interrupt (5)
Process_B [####################] 100.0% -> interrupt (6)
Process_C [##################--] 90.0%  -> interrupt (7)
Process_C [####################] 100.0% -> interrupt (8)

Execution complete. Summary:
----------------------------
Turnaround Times:
 - Process_A 3.90s
 - Process_B 5.16s
 - Process_C 6.51s
Average turnaround time: 5.19s

Response Times:
 - Process_A 0.00s
 - Process_B 3.90s
 - Process_C 5.16s
Average response time: 3.02s

Completion Order:
 0.) Process_A
 1.) Process_B
 2.) Process_C
```

## Evaluation

What is the best scheduling algorithm based on these results? Edit this README file to add your response and explain your choice.
