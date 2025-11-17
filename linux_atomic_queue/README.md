# Atomic Queue Example (C, lock-free)

This project demonstrates lock-free inter-thread communication using a Michael-Scott queue
and C11 atomics (`stdatomic.h`). The program launches one producer and two consumers.

Overview
- Producer reads integers from a file.
- Even numbers go to Consumer A; odd numbers to Consumer B.
- Consumer A computes the square and sends a message to main.
- Consumer B checks primality and sends a message to main.
- The main thread prints messages and ensures a clean shutdown.

Build

```bash
cd /path/to/linux_atomic_queue
make
```

Run

```bash
./atomic_queue input.txt
```

Testing

A small test file `small_test.txt` is included. Run:

```bash
./atomic_queue small_test.txt
```

Notes
- Uses C17 and `gcc`.
- Queues transport `void*` pointers to typed messages.
- Uses an atomic `inFlightCount` to guarantee consumers processed all items before exit.

License: Public domain for educational purposes.
