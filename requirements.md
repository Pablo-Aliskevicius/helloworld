

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

## Thread Functions:
Each thread function will:
1. Receive lines from the main function
1. Process the lines according to its specific task (word count, character count, vowel count)
1. Maintain a running total of its respective count
1. Return the total count to the main function upon completion
1. Terminate gracefully after processing all assigned lines
1. Ensure thread safety when accessing shared resources
1. Handle any potential errors during processing


## Implementation Details:

1. Each thread will manage a queue of lines to process
2. function main will read each line from the file and distribute them to the threads' queues in a round-robin fashion
3. Use `<threads.h>` for threading

## Coding Standards:

1. Use meaningful variable and function names
2. Include comments to explain the code
3. Handle errors appropriately (e.g., file not found, memory allocation failure)
4. Ensure proper synchronization between threads if necessary
5. Free any allocated memory before program termination
6. Follow standard C coding conventions for readability
7. Limit line length to 80 characters where possible
8. Use consistent indentation (4 spaces per level)
9. Avoid global variables unless absolutely necessary
10. Use `const` where applicable to indicate immutability
11. Check return values of system calls and library functions for error handling
12. Modularize code into functions to enhance readability and maintainability
13. Use `size_t` for sizes and counts to ensure portability
14. Document the code with comments explaining the purpose of functions and complex logic
15. Use `#define` for constants instead of magic numbers
16. Avoid deep nesting of control structures to enhance readability
17. Use `enum` for related constants to improve code clarity
18. Prefer `static` functions for internal linkage to limit scope
19. Use `typedef` for complex data structures to improve code readability
20. Ensure proper resource management (e.g., file handles, memory) to prevent leaks
21. Use `assert` for debugging and validating assumptions during development
22. Use PascalCase for function names and snake_case for variable names for consistency
23. Use camelCase for struct and typedef names for clarity
24. Use camelCase for function parameters to differentiate from local variables
25. Avoid using `goto` statements to maintain structured programming principles

