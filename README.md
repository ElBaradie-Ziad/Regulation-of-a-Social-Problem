# Priority Queue Implementation

C project implementing an optimized priority queue data structure with bitmap-based bucket organization for efficient priority tracking (up to 256 priority levels). Includes multi-threaded real-world simulation with Python-based visualization tools.

## Table of Contents

- [Features](#features)
- [Prerequisites](#prerequisites)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Testing](#testing)
- [Project Structure](#project-structure)
- [Technical Details](#technical-details)
- [Team](#team)

## Features

- Bitmap-based priority queue with optimized bucket organization
- Support for up to 256 priority levels
- Multi-threaded real-world simulation using POSIX threads
- Automated visualization of simulation data
- Comprehensive unit test suite
- Complexity analysis tools

## Prerequisites

### Required

- C compiler (gcc/clang)
- Make
- POSIX threads (pthread) support

### For Visualization

- Python 3.x

**Linux users:** Ensure Python 3 and venv support are installed:

```zsh
# Debian/Ubuntu
sudo apt update
sudo apt install python3 python3-venv python3-pip

# Other distributions: use your package manager (dnf, pacman, etc.)
```

## Getting Started

### 1. Clone and Navigate

```zsh
cd problem-solving-2
```

### 2. Set Up Python Environment (Optional but Recommended)

```zsh
make env
```

This creates a virtual environment and installs dependencies (pandas, matplotlib, numpy).

### 3. Build the Project

```zsh
make
```

## Usage

### Basic Usage

**Build and run the main program:**

```zsh
make run
```

**Run directly after building:**

```zsh
./main
```

### Real-World Simulation

**Run 30-second simulation with automatic visualization:**

```zsh
make real
```

This will:

1. Compile [test_situation_reel.c](test_situation_reel.c) with pthread support
2. Execute the simulation for 30 seconds
3. Generate `data_6jours.csv` with simulation data
4. Automatically create visualizations in `graphiques_output/` (requires Python)

**Run simulation directly:**

```zsh
./test_situation_reel
```

### Manual Python Environment Setup

If you prefer manual setup instead of `make env`:

```zsh
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Testing

### Run Unit Tests

```zsh
make test
```

Verifies empty queue operations, insertion, deletion, priority handling, and bitmap operations.

### Run Complexity Analysis

```zsh
make comp
```

Analyzes time complexity of priority queue operations using [complexity.sh](complexity.sh).

### Memory Checking with Valgrind

You can use Valgrind to check for memory leaks and errors:

**Check the main program:**

```zsh
make valgrind
```

**Check the real-world simulation:**

```zsh
make valgrind-real
```

Both commands will run the corresponding binary under Valgrind with leak checking enabled. Make sure Valgrind is installed on your system.

### All Make Targets

| Command              | Description                                 |
| -------------------- | ------------------------------------------- |
| `make` or `make all` | Compile the main program                    |
| `make run`           | Build and execute the main program          |
| `make real`          | Launch 30s simulation with visualization    |
| `make test`          | Run unit test suite                         |
| `make comp`          | Run complexity analysis                     |
| `make env`           | Create Python venv and install requirements |
| `make clean`         | Remove generated files                      |
| `make re`            | Clean and rebuild completely                |
| `make help`          | Display all available targets               |
| `make valgrind`      | Run Valgrind on the main program            |
| `make valgrind-real` | Run Valgrind on the real-world simulation   |

## Project Structure

```text
.
├── main.c                  # Main program entry point
├── test_situation_reel.c   # Real-world simulation (multi-threaded, 30s)
├── priority_queue.h        # Priority queue header
├── priority_queue.c        # Priority queue implementation with bitmap optimization
├── test.sh                 # Unit test runner script
├── complexity.sh           # Complexity analysis script
├── plot_buckets.py         # Python visualization script
├── requirements.txt        # Python dependencies
└── Makefile                # Build configuration
```

### Generated Files

- `data_6jours.csv` - Simulation output data
- `graphiques_output/` - Generated visualization graphs

## Technical Details

### Priority Queue Implementation

- **Algorithm:** Bitmap-based bucket organization
- **Priority Levels:** Up to 256
- **Optimization:** Efficient priority tracking using bitmaps

### Real-World Simulation

- **Concurrency:** POSIX threads (pthread)
- **Duration:** 30 seconds
- **Output:** CSV data and graphical visualizations

### Visualization

- **Tools:** Python (pandas, matplotlib, numpy)
- **Script:** [plot_buckets.py](plot_buckets.py)
- **Output Directory:** `graphiques_output/`

## Team

**Team 3.4:**

- ALLAOUI Amine
- AYYILDIZ Yigit
- BAYRAK Azra
- DEBBIH Serine
- ELBARADIE Ziad
- SCAPIN Leo
- TRASHI Renaldo
