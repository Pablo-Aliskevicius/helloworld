
# Create a C program with the following specifications:

- Must be written in C   
- Will be compiled using gcc in Linux   
- Should read a text file specified as a command line argument   

## Functionality:

### Function main
Function main will perform the following tasks:   
1. Take the file name as a command line argument
2. Open the file for reading
3. Create three threads to process the file

Each thread will perform a different task:
   - Thread 1: Count the number of words in each line it receives
   - Thread 2: Count the number of characters in each line it receives
   - Thread 3: Count the number of vowels in each line it receives

4. Read the file line by line and distribute the lines to the three threads in a round-robin fashion
5. At end, the program will print the total counts from each thread, and the total number of lines processed.


### Implementation Details:

1. Each thread will manage a queue of lines to process
2. function main will read each line from the file and distribute them to the threads' queues in a round-robin fashion



