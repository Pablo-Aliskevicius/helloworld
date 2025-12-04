# IPC Exercise — POSIX Message Queues

Chosen IPC method: POSIX message queues (`mqueue`, `mq_open`, `mq_send`, `mq_receive`).

Files
- `main.c` — single program that forks three processes: producer, consumer A, consumer B.
- `Makefile` — build rules.

Design & flow

- Parent creates two POSIX message queues: `/ipc_consumerA` and `/ipc_consumerB` with message size = 4 bytes (int32).
- Parent forks the **producer** first. The producer opens both queues for writing and sends a SIGUSR1 to the parent once ready.
- Parent waits for that signal, then forks **consumer A** and **consumer B**.
- Producer sends integers 1..100 to both queues, then sends `-1` to each queue to indicate end-of-data.
- Consumer A reads from `/ipc_consumerA`, prints PID, received number and its square.
- Consumer B reads from `/ipc_consumerB`, prints PID, received number and whether it is prime.
- Parent waits for children and unlinks (removes) the message queues.

ASCII diagram

```text
Parent
  ├─ creates queues `/ipc_consumerA` `/ipc_consumerB`
  ├─ forks Producer (first)
  │    ├─ opens queues and sends SIGUSR1 to parent (ready)
  │    └─ sends values 1..100 and `-1` terminator to both queues
  ├─ waits SIGUSR1
  ├─ forks Consumer A
  │    └─ reads from `/ipc_consumerA` → prints square
  └─ forks Consumer B
       └─ reads from `/ipc_consumerB` → prints prime check
```

Build

Run in the `linux_inter_process_communications` directory:

```bash
make
```

Run

```bash
./ipc
```

You should see output like:

```
[A: pid 5423] received 12 → square = 144
[B: pid 5424] received 12 → prime = no
```

Notes and decisions
- POSIX message queues were chosen because the producer can send messages before consumers start; queues buffer messages until consumers read them.
- A small signal-based handshake (`SIGUSR1`) is used so the parent ensures the producer begins before forking consumers (per exercise requirement).
- The termination marker is the integer `-1`.
- Error handling uses `perror()` and exits non-zero on unrecoverable errors.

Cleanup

If the program exits unexpectedly and leaves message-queue objects behind, remove them with:

```bash
sudo rm -f /dev/mqueue/ipc_consumerA /dev/mqueue/ipc_consumerB
```

Or use `mq_unlink` in a small helper program; the `mq_*` objects appear under `/dev/mqueue` on Linux.

Extensions

- Broadcast results from consumers back to a parent using additional queues or pipes.
- Replace message queues with shared memory + semaphores for the bonus.
