# Regulation of a Social Problem — Content Moderation via Priority Queue

A systems-programming project that models the automated moderation of social media content at scale. The core idea: millions of posts are generated daily and each is assigned a **risk score** reflecting its potential harmfulness. A high-performance, bitmap-optimized priority queue acts as the moderation buffer, ensuring that the most harmful content is always processed first.

---

## Project Description

Modern social platforms face an overwhelming volume of user-generated content, much of it requiring human or automated review for harmful material. Reviewing everything equally is impossible — intelligent prioritization is essential.

This project proposes and implements a **priority-based content regulation engine**:

- Each incoming post receives a **risk score** (0–100), computed from a statistical distribution that models real-world content patterns (most content is benign, a minority is highly harmful).
- The score is mapped to one of **256 priority buckets** using an optimized split strategy: low-risk posts (score ≤ 45) are compressed into the bottom 10% of buckets; high-risk posts get fine-grained resolution across the remaining 90%.
- A fixed-size buffer (10 million slots, ~120 MB RAM) holds the current moderation backlog. When full, incoming posts only enter if they outrank the lowest-priority item currently stored — ensuring the buffer always holds the most dangerous content.
- A **6-day simulation** ingests 360 million posts (60M/day) with a 10:1 production-to-consumption ratio, mimicking a real platform under continuous load. Per-bucket statistics are exported to CSV for analysis.

The result is a system that **automatically surfaces the worst content** while gracefully discarding low-risk overflow — a practical model for scalable content regulation.

---

## Features

- Bitmap-based priority queue with O(1) find-max / find-min via `__builtin_clzll` / `__builtin_ctzll`
- 256 priority levels mapped from continuous risk scores
- Capacity-capped buffer with automatic eviction of lowest-priority entries
- 6-day multi-million-post simulation with daily snapshots
- CSV export of per-bucket counts for each simulated day
- Python visualization pipeline (matplotlib, pandas, numpy)
- Unit test suite and complexity analysis scripts
- Valgrind-clean memory management

---

## Prerequisites

| Dependency | Purpose |
|---|---|
| `gcc` / `clang` | Compile C source files |
| `make` | Build automation |
| `pthread` (POSIX) | Multi-threaded simulation |
| Python 3.x | Visualization (optional) |

**Linux (Debian/Ubuntu):**
```bash
sudo apt update
sudo apt install gcc make python3 python3-venv python3-pip
```

---

## Getting Started

### 1. Clone the repository

```bash
git clone <repo-url>
cd Regulation-of-a-social-problem
```

### 2. (Optional) Set up the Python visualization environment

```bash
make env
```

### 3. Build

```bash
make
```

---

## Usage

### Run the ingestion benchmark

Simulates 90 million post insertions and reports throughput (M ops/sec):

```bash
make run
# or
./main
```

### Run the 6-day social media simulation

Processes 360 million posts over 6 simulated days and generates `data_6jours.csv` plus charts in `graphiques_output/`:

```bash
make real
```

### Generate visualizations manually

```bash
python3 plot_buckets.py
```

---

## Testing & Analysis

```bash
make test       # Unit tests: empty queue, push, pop, priority ordering, bitmap ops
make comp       # Complexity analysis of queue operations
make valgrind   # Valgrind memory check on main benchmark
make valgrind-real  # Valgrind memory check on simulation
```

---

## All Make Targets

| Command | Description |
|---|---|
| `make` / `make all` | Compile the main program |
| `make run` | Build and run the ingestion benchmark |
| `make real` | Run 6-day simulation with visualization |
| `make test` | Run unit test suite |
| `make comp` | Run complexity analysis |
| `make env` | Create Python venv and install requirements |
| `make clean` | Remove generated files |
| `make re` | Clean and rebuild |
| `make help` | Show all targets |
| `make valgrind` | Valgrind on main program |
| `make valgrind-real` | Valgrind on simulation |

---

## Project Structure

```
.
├── main.c                  # Ingestion benchmark (90M posts)
├── test_situation_reel.c   # 6-day simulation (360M posts, CSV output)
├── priority_queue.h        # Public API
├── priority_queue.c        # Bitmap priority queue implementation
├── test.sh                 # Unit test runner
├── complexity.sh           # Complexity analysis script
├── plot_buckets.py         # Visualization script
├── requirements.txt        # Python dependencies
└── Makefile                # Build system
```

**Generated at runtime:**
- `data_6jours.csv` — per-day, per-bucket post counts
- `graphiques_output/` — charts produced by `plot_buckets.py`

---

## Technical Details

### Priority Queue

- **Structure:** Linked lists per priority level, backed by a flat memory pool (no dynamic allocation per item)
- **Bitmap:** 4 × 64-bit words tracking which buckets are non-empty — max/min lookup is O(1)
- **Eviction policy:** When the buffer is full, a new post replaces the current minimum-priority item only if it scores higher, ensuring the buffer always holds the top-priority backlog

### Risk Score → Bucket Mapping

A two-range split at score 45:
- Scores 0–45 → buckets 0–25 (low resolution; safe content)
- Scores 45–100 → buckets 26–255 (high resolution; harmful content)

This gives much finer discrimination where it matters most.

### Simulation Model

- **Daily volume:** 60 million posts
- **Distribution:** 50% near-zero risk (normal, μ=0), 30% mid-range (μ=50), 20% high-risk (μ=95)
- **Ratio:** 10 posts produced per 1 post consumed (realistic backlog pressure)
- **Duration:** 6 simulated days

---

## Team

**Team 3.4**

- ALLAOUI Amine
- AYYILDIZ Yigit
- BAYRAK Azra
- DEBBIH Serine
- ELBARADIE Ziad
- SCAPIN Leo
- TRASHI Renaldo
