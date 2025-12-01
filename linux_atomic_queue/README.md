# Atomic Queue Example (C, lock-free)

This project demonstrates lock-free inter-thread communication using a Michael-Scott queue
and C11 atomics (`stdatomic.h`). The program launches one producer and two consumers.

Overview
- Producer reads integers from a file.
- Even numbers go to Consumer A; odd numbers to Consumer B.
- Consumer A computes the square and sends a message to main.
- Consumer B checks primality and sends a message to main.
- The main thread prints messages and ensures a clean shutdown.

Build (release)

```bash
cd /path/to/linux_atomic_queue
make
```
Build (debug)

```bash
cd /path/to/linux_atomic_queue
make debug
```

Run

```bash
make run
```

Run debug
```bash
make run_dbg
```


Testing

A small test file `small_test.txt`, and a medium one, `input.txt` are included. Run:

```bash
make test
```

Notes
- Uses C17 and `gcc`.
- Queues transport `void*` pointers to typed messages.
- Uses an atomic `inFlightCount` to guarantee consumers processed all items before exit.

License: Public domain for educational purposes.
