# Simple Contiguous Memory Allocator

A C++ console application simulating a **contiguous memory allocation system**. This program allows users to request, release, and compact memory for processes using different allocation strategies.

---

## Features

- Supports **First-Fit, Best-Fit, and Worst-Fit** allocation strategies.
- Dynamic memory initialization with a maximum of **16 MB**.
- Tracks processes and free memory holes.
- Memory compaction to reduce fragmentation.
- Console commands for interactive memory management.

---

## Commands

| Command | Description |
|---------|-------------|
| `RQ <name> <size[K|M]> [f|b|w]` | Request memory for a process. Optional allocation strategy: `f` = first-fit, `b` = best-fit, `w` = worst-fit. |
| `RL <name>` | Release memory of a process. |
| `CMP` or `COMPACT` | Compact memory to merge free spaces. |
| `STAT` | Display current memory layout (processes and holes). |
| `SIZE <size[K|M]>` | Reinitialize memory with a new size. |
| `HELP` or `?` | Display help commands. |
| `X` or `EXIT` | Exit the program. |

---

## Memory Model

- Memory is represented as a vector of **sections**.
- Each section can be a **process** or a **hole**.
- Processes occupy a contiguous block; holes represent free memory.
- Supports **merging adjacent holes** and **splitting holes** when allocating memory.

---

## Allocation Strategies

1. **First-Fit (`f`)**: Allocates the first hole large enough for the requested memory.  
2. **Best-Fit (`b`)**: Allocates the smallest hole that fits the request.  
3. **Worst-Fit (`w`)**: Allocates the largest hole available.

---

## Usage Example

```bash
allocator> SIZE 1M
-> Allocated 1048576

allocator> RQ P1 100K f
-> Allocated 102400 for [P1] successfully.

allocator> RQ P2 200K b
-> Allocated 204800 for [P2] successfully.

allocator> STAT
[1] P1 - [0: 102400) - 102400
[2] P2 - [102400: 307200) - 204800
[0] hole - [307200: 1048576) - 741376

allocator> RL P1
-> Released [P1]: 102400

allocator> CMP
-> Successfully compacted memory
