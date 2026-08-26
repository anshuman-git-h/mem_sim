# Multi-Level Cache & Virtual Memory Simulator

A from-scratch C++ simulator that models how a memory access travels through
the **TLB → page table → L1/L2/L3 caches → RAM → disk**, and lets you swap in
different page-replacement and cache-replacement algorithms to see how they
affect **AMAT (Average Memory Access Time)**, hit rates, and page faults under
different workload locality patterns.

## Why this project

Modern CPU (and GPU) performance is dominated by the memory hierarchy, not
raw ALU throughput. This simulator is a sandbox for the exact questions a
memory-systems / performance-architecture team asks: *how sensitive is AMAT
to associativity and replacement policy, and how does workload locality
change the answer?*

## Architecture

```
 Virtual Address
       |
       v
   +-------+   miss   +------------+   miss (fault)   +-------+
   |  TLB  |--------->| Page Table |----------------->| Disk  |
   +-------+          +------------+                   +-------+
       | hit                | hit
       v                    v
                    Physical Address
                          |
                          v
                  +----+  +----+  +----+
                  | L1 |->| L2 |->| L3 |----> RAM
                  +----+  +----+  +----+
```

- **TLB**: fully-associative, LRU.
- **Page Table**: configurable replacement — **FIFO, LRU, Optimal (Belady,
  offline lookahead), Clock (second-chance), LFU**.
- **Cache levels (L1/L2/L3)**: independent size / line size / associativity /
  latency per level, configurable replacement — **LRU, FIFO, Random,
  Pseudo-LRU (binary tree PLRU, matches what's used in real set-associative
  hardware)**.
- **Workload generator**: Sequential, Random, Looping (cyclic working set —
  exposes FIFO/Belady's-anomaly-style pathologies), Working-Set (tunable hot
  set + locality probability).

## Build

```bash
# CMake
mkdir build && cd build
cmake .. && make

# or directly
g++ -std=c++17 -O2 -Iinclude src/*.cpp -o memsim
```

## Run

```bash
./memsim 500000     # arg = number of memory accesses per configuration
```

This sweeps all **5 page algorithms × 4 cache algorithms × 4 workloads (80
configurations)**, prints a comparison table, and writes `results.csv` for
further analysis / plotting (e.g. a Python/matplotlib notebook comparing
AMAT vs. associativity vs. policy).

Sample output (20K accesses):

```
PageAlg  CacheA Workload         TLB-Hit    PgFault     L1-Hit    L2-Hit    L3-Hit   AMAT(cyc)
----------------------------------------------------------------------------------------------
FIFO     LRU    RANDOM             1.53%     94.12%      0.39%      2.61%     98.68%   188406.79
OPTIMAL  LRU    RANDOM             4.23%     69.09%     60.01%     25.62%     95.70%   138310.02
LRU      LRU    WORKING_SET       73.52%      9.61%     10.17%     70.63%     95.15%    19265.38
OPTIMAL  LRU    WORKING_SET       79.37%      8.00%     12.43%     82.69%     91.56%    16044.76
```

Optimal (Belady) always dominates as expected — the interesting comparisons
are LRU vs. Clock vs. LFU under the Looping and Working-Set workloads, where
the theoretical gap in the textbook is much smaller than it is under Random.

## Extending it (good next steps for depth)

- Multi-core coherence: add a MESI/MOESI protocol across per-core L1/L2 with
  a shared L3, and measure coherence-traffic overhead.
- NUMA-style access: model asymmetric RAM latency by "socket."
- Prefetching: stride/next-line prefetcher into L1, measure AMAT delta.
- Write-back vs. write-through + dirty-bit eviction cost.
- Replace the offline Belady lookahead with an approximation (e.g. hawkeye-
  style PC-based reuse prediction) and compare to true Optimal.

## Repository layout

```
include/   Types.h, Tlb.h, PageTable.h, Cache.h, MemoryHierarchy.h, WorkloadGenerator.h
src/       matching .cpp files + main.cpp (CLI driver / experiment sweep)
CMakeLists.txt
results.csv (generated)
```
