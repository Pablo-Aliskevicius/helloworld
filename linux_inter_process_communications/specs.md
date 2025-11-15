You are teaching C development for Linux. 
Generate specs for an exercise in inter-process communications, for your students.
The specs must be in .md format. 

# Exercise Specification: Inter-Process Communication (IPC) in Linux (C Programming)

## Overview
In this exercise, you will design and implement a small system composed of multiple processes that communicate with each other using **Linux IPC mechanisms**.  
By completing this assignment, you will gain hands-on experience with process creation, synchronization, and data exchange in C.

---

## Learning Objectives
- Understand and use **fork()** to create processes.
- Implement communication using:
  - **Pipes (unnamed and/or named FIFOs)**
  - **Message queues (System V or POSIX)**
  - **Shared memory + semaphores** (optional advanced extension)
- Practice error checking, resource cleanup, and safe IPC patterns.
- Learn how to design process interactions using diagrams and protocols.

---

## Requirements

### 1. Communication Scenario
You will implement a system containing **one producer process** and **two consumer processes**.

- The **producer** generates a sequence of integer values (e.g., 1–100).
- The **consumers** receive values from the producer and perform different operations:
  - **Consumer A:** computes the square of each number.
  - **Consumer B:** checks if each number is prime.

### 2. IPC Mechanism
Choose **one IPC mechanism** from the list below and implement the full system using it:

1. **Unnamed pipes**
2. **Named pipes (FIFOs)**
3. **POSIX message queues**
4. **System V message queues**
5. **Shared memory + semaphores**  
   *(More challenging. Bonus credit.)*

Your program must:
- Initialize the chosen IPC mechanism.
- Send all data from the producer to both consumers.
- Close and clean up all IPC resources properly.

### 3. Process Structure
Your main program must:
1. Create the producer process.
2. Create the two consumer processes.
3. Synchronize them so the producer runs first.
4. Ensure consumers exit correctly when all data has been processed.

### 4. Output Requirements
Each consumer must print:
- Its PID.
- Each number received.
- The processed result (square or prime-check response).

Example (Consumer A):
```
[A: pid 5423] received 12 → square = 144
```

Example (Consumer B):
```
[B: pid 5424] received 12 → prime = no
```

### 5. Error Handling
You must handle:
- Failed memory allocations.
- Failed IPC calls.
- Interrupted system calls.
- Unexpected termination of other processes.

Use `perror()` for diagnostics and exit with a non-zero status on errors.

---

## Deliverables

### 1. Source Code
Submit:
- `producer.c`
- `consumerA.c`
- `consumerB.c`
- Or a single `main.c` that forks three processes
- Any headers or support files

### 2. Build Instructions
Provide a **Makefile** containing at least:
```make
all:
    gcc -Wall -Wextra -o ipc main.c
```

Modify as needed for multiple source files.

### 3. README.md

Your README must include:

- Your chosen IPC method.
- A diagram of process communication (ASCII diagram acceptable).
- Instructions to compile and run your program.
- Notes on design decisions.

### 4. Optional Extension (Bonus)

Add support for:

- Broadcasting results from consumers back to the parent.
- Using select(), poll(), or epoll with pipes.
- Signal-based shutdown (SIGTERM/SIGUSR1).

## Grading Criteria
| Category	    | Points |
|:--------------|-------:|
|Correct IPC Implementation	| 40 |
| Clean process creation and synchronization | 20 |
| Correct output & functionality | 20 |
| Error handling & cleanup | 10 |
| Code clarity & documentation | 10 |
| Bonus (optional) | +10 |


## Notes

You may test your program using:

```bash
strace -f ./ipc
```

(Use the option `-o filename` to separate `strace` output from `ipc` output)
or view IPC objects (for message queues / shared memory) using:

```bash
ipcs -q
ipcs -m
ipcs -s
```

## Good luck!

Make sure your code is robust, readable, and well-documented.