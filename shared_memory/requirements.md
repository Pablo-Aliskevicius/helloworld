# Shared memory exercise

## Functional requirements

1. Implement a chat application, in C under Linux.
2. There will be a server program, and a client program.
3. Only one instance of the server will run.
4. Several instances of the client may run. Limit to 5 for conveniency.

### General

1. Messages will be passed using shared memory.
2. Each client will login to the server using the process ID as identifier.
3. Events will be communicated between the server and each client using an ad-hoc queue, created with `mq_open`

### Server program

1. The server will allocate the shared memory when it starts, using `shm_open`
2. If the memory already existed, assume another instance is running and exit after showing an error message.
3. The server will also close the memory when it exits, using `shm_unlink`.
4. The server will log events on standard output as: 
    1. Print a timestamp up to the microsecond.
    2. Print what happened:
        1. Map open
        2. Client connect + client ID
        3. Message received + client ID
        4. Broadcasting message 
        5. Finished broadcasting, all clients acknowledged with the list of clients.
        6. Client disconnected with client ID
        7. Client failed to acknowledge timely a disconnection request with client ID.
        8. Server refused a client connection, too many clients already connected.
5. The server will exit when the user types `ctrl-C` on the console.
6. Clients will connect to and disconnect from the server by sending a standard message to an ad-hoc queue, named `71dcbdfc-8b5c-45b6-93cf-5e961df6f4f4-listener`.
7. If there are connected clients when the server exits, the server will disconnect from clients by broadcasting the custom message `6b540d1b-cd12-4bd9-bdfd-64cbcf1ed258` and waiting 100 milliseconds for acknowledgment.
8. The server will clean up all queues and memory maps.


### Client program

1. When the client starts, it will open the shared memory using `mmap`.
2. If failed (server not running), show an error message and exit.
3. The client will ask the server for a connection, using the global queue. 
3. If failed (server refused connection, too many clients already connected), show an error message and exit.
3. Show the process ID in the console.
3. Enter a loop:
    1. The user will be prompted to enter a string.
    2. The client will notify the server that a new string is available, and its length.
    2. The server will give the client an offset in shared memory.
    3. The client will write the string in the location specified by the server.
    4. The client will notify the server that the string is available.
    5. The server will notify all other client instances (different process ID) that a string is available, its location and length.
    6. The other clients will retrieve and display the strings.
    7. The other clients will notify the server that the string was written and it can recycle the location.
    8. If the message is 'bye' or 'exit' (case insensitive), break the loop and exit the program.
5. At exit, release shared memory using `munmap` and close any open message queues.

## Non functional requirements

1. Support the C17 standard.
1. Code should be commented to document design decisions.
1. Separate the code in three folders: client, server and common.
1. Put the output (.o and executables) in folder `output`
1. Generate a make file.
    1. Targets: clean, all, server, client

## Testing

1. Open a terminal, run the client.
    - Expected result: error message and exit.

2. In the same terminal, run the server
    - Expected result: message that says that the map was created.

3. In a separate terminal, run a client.
    - Expected result: In the server console, message that says that the client is connected.

4. In a third terminal, run another client.
    - Expected result: In the server console, message that says that the client is connected.

5. In one of the client windows, type a string.
    - Expected result #1: The server shows that it received a message
    - Expected result #2: The second client shows the message.
    - Expected result #3: The server shows that it finished broadcasting.

6. In a fourth terminal, run a third client.
    - Expected result: In the server console, message that says that the client is connected.

7. Enter a string in the third client.
    - Expected result #1: The server shows that it received a message
    - Expected result #2: The first and second clients show the message.
    - Expected result #3: The server shows that it finished broadcasting.

8. Run `ipcs` in a separate terminal, to see the state of system queues and memory maps.

8. Exit one of the clients by typing 'bye'
    - Expected result: the client exits.

9. From one of the two clients, send a message.
    - Expected result #1: The server shows that it received a message
    - Expected result #2: The other client shows the message.
    - Expected result #3: The server shows that it finished broadcasting.

10. Exit all clients by entering 'bye'

11. Exit the server by typing `Ctrl-C`

## Coding standards

1. Functions should be at most 40 lines long.
2. Use Kernighan braces.


