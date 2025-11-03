
Create a C program with the following specifications:

Must be written in C
Will be compiled using gcc in Linux
Should read a text file specified as a command line argument
Function main will
1. Take the file name as a command line argument
2. Open the file for reading
3. Create three threads to process the file
4. Each thread will manage a queue of lines to process
5. function main will read each line from the file and distribute them to the threads' queues in a round-robin fashion
6. The first thread will count the number of words in each line
7. The second thread will count the number of characters in each line
8. The third thread will count the number of vowels in each line
At end, the program will print the total counts from each thread
