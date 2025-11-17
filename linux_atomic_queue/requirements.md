# Requirements

The task is to show atomic operations in a C program in Linux, using lock-free patterns.

## Functional requirements

1. The program will start one producer and two consumer threads.
2. The producer thread will read integer numbers from a file passed in the command line.
3. Even numbers will be sent to consumer thread A. 
4. Consumer thread A will calculate the square of the number, and pass both the number and its square to the main thread.
5. Odd numbers will be sent to consumer thread B.
6. Consumer thread B will check whether the number is prime.
7. Consumer thread B will pass both the number and its square to the main thread.
8. When the producer thread is done reading the file, it will:
    1. Notify the main thread that fact, and the count of numbers it read from the file.
    2. Notify the consumer threads that they may exit.
    3. End.
9. The main thread will print to standard output the results it received from the producer and consumer threads.
    1. When the producer thread is done, it will print 'Finished reading the file, %d numbers read'
    2. For each pair of number received from consumer thread A, it will print a message formatted as "%d x %d = %d"
    3. For each number and Boolean received from consumer thread B, it will print a message matching either "%d is prime" or "%d is not prime"
10. Use custom queues for inter-thread communications.
11. Use atomics for synchronization. 
12. Prefer <stdatomic.h> to the older `__sync` functions.

## Non functional requirements

- The language must be C.
- Use the C17 standard.
- The code must be compiled using gcc in Linux.
- Each of the threads (main, producer and two consumers) must be implemented in a separate C source file.

## Coding Standards

- Provide detailed comments explaining the rationale behind the code.
- Use Kernighan braces.
- Use PascalCase for structs and functions
- Use camelCase for function parameters and global variables.
- Use UPPER_CASE for constants.
- Use lower_case for variables.
- No function should be longer than 40 lines.
- Use static functions for encapsulation.