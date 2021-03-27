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


Here are my reports:

BASIC:

<pre>
	Execution complete. Here is your summary with the algorithm: 'basic':
	TURNAROUND TIME
	'Process_A': 5.006055
	'Process_B': 11.014856
	'Process_C': 18.024956
	'Process_D': 25.036254
	'Process_E': 34.049448
	Average turnaround time: 18.626314


	RESPONSE TIME
	'Process_A': 0.000276
	'Process_B': 0.000263
	'Process_C': 0.000324
	'Process_D': 18.024859
	'Process_E': 0.000245
	Average response time: 3.605193


	ORDER COMPLETED
	0) 'Process_A'
	1) 'Process_B'
	2) 'Process_C'
	3) 'Process_D'
	4) 'Process_E'
</pre>

FIFO:

<pre>
	Execution complete. Here is your summary with the algorithm: 'fifo':
	TURNAROUND TIME
	'Process_E': 9.013176
	'Process_C': 15.023413
	'Process_D': 22.034083
	'Process_B': 26.041954
	'Process_A': 29.048117
	Average turnaround time: 20.232149


	RESPONSE TIME
	'Process_E': 0.000095
	'Process_C': 8.013155
	'Process_D': 15.023284
	'Process_B': 21.033633
	'Process_A': 25.041454
	Average response time: 13.822324


	ORDER COMPLETED
	0) 'Process_E'
	1) 'Process_C'
	2) 'Process_D'
	3) 'Process_B'
	4) 'Process_A'
</pre>

SJF:

<pre>
	Execution complete. Here is your summary with the algorithm: 'sjf':
	TURNAROUND TIME
	'Process_E': 9.012065
	'Process_A': 10.016623
	'Process_B': 16.023237
	'Process_C': 24.032357
	'Process_D': 31.042536
	Average turnaround time: 18.025364


	RESPONSE TIME
	'Process_E': 0.000141
	'Process_A': 6.011352
	'Process_B': 11.017246
	'Process_C': 17.023656
	'Process_D': 24.032587
	Average response time: 11.616996


	ORDER COMPLETED
	0) 'Process_E'
	1) 'Process_A'
	2) 'Process_B'
	3) 'Process_C'
	4) 'Process_D'
</pre>



PSJF:
Note: This or SJF might be bugged?

<pre>
	Execution complete. Here is your summary with the algorithm: 'psjf':
	TURNAROUND TIME
	'Process_A': 5.005806
	'Process_B': 11.015301
	'Process_C': 18.022741
	'Process_D': 25.031700
	'Process_E': 34.042357
	Average turnaround time: 18.623581


	RESPONSE TIME
	'Process_A': 0.000262
	'Process_B': 0.000302
	'Process_C': 0.000307
	'Process_D': 18.022689
	'Process_E': 0.000153
	Average response time: 3.604743


	ORDER COMPLETED
	0) 'Process_A'
	1) 'Process_B'
	2) 'Process_C'
	3) 'Process_D'
	4) 'Process_E'

</pre>




RR:

<pre>
	Execution complete. Here is your summary with the algorithm: 'rr':
	TURNAROUND TIME
	'Process_A': 21.003371
	'Process_B': 29.004849
	'Process_C': 32.005931
	'Process_D': 32.005838
	'Process_E': 35.008280
	Average turnaround time: 29.805654


	RESPONSE TIME
	'Process_A': 1.000530
	'Process_B': 3.001141
	'Process_C': 0.000289
	'Process_D': 1.000623
	'Process_E': 0.000115
	Average response time: 1.000540


	ORDER COMPLETED
	0) 'Process_A'
	1) 'Process_B'
	2) 'Process_C'
	3) 'Process_D'
	4) 'Process_E'
</pre>



PRIORITY:

<pre>
	Execution complete. Here is your summary with the algorithm: 'priority':
	TURNAROUND TIME
	'Process_B': 5.006316
	'Process_D': 14.014791
	'Process_C': 19.022925
	'Process_E': 28.032155
	'Process_A': 29.037413
	Average turnaround time: 19.022720


	RESPONSE TIME
	'Process_B': 0.000279
	'Process_D': 7.006712
	'Process_C': 0.000284
	'Process_E': 0.000192
	'Process_A': 25.030971
	Average response time: 6.407688


	ORDER COMPLETED
	0) 'Process_B'
	1) 'Process_D'
	2) 'Process_C'
	3) 'Process_E'
	4) 'Process_A'
</pre>




INSANITY:

<pre>
	Execution complete. Here is your summary with the algorithm: ':D':
	TURNAROUND TIME
	'Process_A': 16.003041
	'Process_B': 21.004685
	'Process_E': 26.006379
	'Process_D': 30.007211
	'Process_C': 32.011883
	Average turnaround time: 25.006640


	RESPONSE TIME
	'Process_A': 3.000692
	'Process_B': 0.000300
	'Process_E': 0.000124
	'Process_D': 7.002009
	'Process_C': 3.001743
	Average response time: 2.600974


	ORDER COMPLETED
	0) 'Process_A'
	1) 'Process_B'
	2) 'Process_E'
	3) 'Process_D'
	4) 'Process_C'
</pre>

Basic:
Average turnaround time: 18.626314
Average response time: 3.605193

FIFO:
Average turnaround time: 20.232149
Average response time: 13.822324

SJF:
Average turnaround time: 18.025364
Average response time: 11.616996

PSJF:
Average turnaround time: 18.623581
Average response time: 3.604743

RR:
Average turnaround time: 29.805654
Average response time: 1.000540

Priority:
Average turnaround time: 19.022720
Average response time: 6.407688

Insanity:
Average turnaround time: 25.006640
Average response time: 2.600974



Turnaround times:
SJF
PSJF
Priority
Basic
FIFO
Insanity
RR

Response times:
RR
Insanity
PSJF
Basic
Priority
SJF
Fifo


So far, it seems that PSJF is the best, as it ranks 3rd in both turnaround times and response times.

The other approaches may rank high on one list, but rank quite low on the others.