<div align="center">
  <img src="assets/blink.svg" width="220" alt="Blink logo" />
  <h1>Blink</h1>
  <p>MPI micro-benchmarks for collective and point-to-point communication patterns</p>
</div>

---

Blink is a collection of MPI benchmarks designed for long-running, in-situ measurement of network behaviour under realistic traffic conditions.  Each benchmark runs for a configurable number of iterations (or endlessly until interrupted), records per-iteration latency on every rank, and emits a CSV summary when it finishes — either naturally or via `SIGUSR1`.

## How Blink differs from other benchmarking suites

Most MPI benchmark suites run a fixed iteration sweep and report a single aggregate table (min / median / max per message size) before exiting. Blink takes a different approach on three axes:

**Temporal evolution.** Blink records one measurement per iteration rather than collapsing everything into a final summary. This lets you observe how latency evolves over time, detect jitter events, and correlate performance spikes with external activity on the system.

**Burst traffic modeling.** Real applications rarely issue communication at a steady, uniform rate. Blink supports configurable burst-pause cycles, including exponentially distributed burst lengths and pause durations, so the generated traffic pattern can more closely reflect the bursty nature of production workloads.

**Long-running operation.** Blink is designed to run alongside real workloads or as a background monitor. It can run endlessly (`-endl`), keeps only the most recent *N* samples in a ring buffer (`-maxsamples`), and responds to `SIGUSR1` with a clean shutdown — collecting and printing whatever has been measured so far before calling `MPI_Finalize`.

## Building

Requirements: CMake ≥ 3.16, an MPI implementation (e.g. OpenMPI, MPICH, Intel MPI).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Binaries are placed in `build/bin/`.  Adding a new `.c` or `.cpp` file anywhere under `src/` is enough — CMake picks it up automatically on the next configure.

## Common flags

Every benchmark understands the following flags:

| Flag | Default | Description |
|------|---------|-------------|
| `-msgsize <B>` | `1024` | Message size in bytes |
| `-iter <N>` | `1` | Number of measured iterations (after warm-up) |
| `-warmup <N>` | `5` | Number of warm-up iterations (not recorded) |
| `-endl` | off | Run endlessly until `SIGUSR1` |
| `-grty <N>` | `1` | Measurement granularity — number of MPI calls batched per timed window |
| `-maxsamples <N>` | `1000` | Maximum recorded samples (ring buffer) |
| `-mrank <R>` | `0` | Rank that collects and prints results |
| `-mrand` | off | Pick master rank randomly |
| `-seed <S>` | `1` | RNG seed (shared across ranks) |
| `-blength <s>` | `0` | Burst length in seconds (0 = single shot per iteration) |
| `-blrand` | off | Randomise burst length (exponential distribution, mean = `-blength`) |
| `-bpause <s>` | `0` | Pause between bursts in seconds |
| `-bprand` | off | Randomise pause length (exponential distribution, mean = `-bpause`) |
| `-pretty-print` | off | Human-readable table output instead of CSV (see [Output format](#output-format)) |

## Benchmarks

Benchmarks are grouped by traffic pattern.  The suffix convention is:

| Suffix | Meaning |
|--------|---------|
| `_b` | Blocking MPI call |
| `_nb` | Non-blocking MPI call (`MPI_I*` + `MPI_Waitall`) |
| `_bsnbr` | Non-blocking send, blocking receive (anti-deadlock variant) |
| `_man` | Manual / hand-rolled implementation (no collective primitive) |
| `_comm_only` | Communication-only variant (no computation, C++) |
| `_get` / `_put` | One-sided (RMA) variant |

### Collectives

#### All-to-all — `alltoall/`
Every rank sends a distinct message to every other rank.

| Binary | MPI primitive |
|--------|--------------|
| `alltoall_b` | `MPI_Alltoall` |
| `alltoall_nb` | `MPI_Ialltoall` |
| `alltoall_man` | `MPI_Isend` / `MPI_Irecv` (manual) |
| `alltoall_comm_only` | `MPI_Isend` / `MPI_Irecv` (C++, no memory init) |

#### All-gather — `allgather/`
Every rank broadcasts its buffer; all ranks collect the full result.

| Binary | MPI primitive |
|--------|--------------|
| `allgather_b` | `MPI_Allgather` |
| `allgather_nb` | `MPI_Iallgather` |
| `allgather_comm_only` | `MPI_Isend` / `MPI_Irecv` (C++) |

#### All-reduce — `allreduce/`
Global reduction (`MPI_SUM` over `double`) delivered to all ranks.

| Binary | MPI primitive |
|--------|--------------|
| `allreduce_b` | `MPI_Allreduce` |
| `allreduce_nb` | `MPI_Iallreduce` |

#### Barrier — `barrier/`

| Binary | MPI primitive |
|--------|--------------|
| `barrier_b` | `MPI_Barrier` |
| `barrier_nb` | `MPI_Ibarrier` |

#### Broadcast — `broadcast/`
Root sends one buffer to all other ranks.

| Binary | MPI primitive |
|--------|--------------|
| `broadcast_b` | `MPI_Bcast` |
| `broadcast_nb` | `MPI_Ibcast` |

#### Gather — `gather/`
All ranks send to root; root collects all messages.

| Binary | MPI primitive |
|--------|--------------|
| `gather_b` | `MPI_Gather` |
| `gather_nb` | `MPI_Igather` |

#### Scatter — `scatter/`
Root distributes a distinct chunk to every rank.

| Binary | MPI primitive |
|--------|--------------|
| `scatter_b` | `MPI_Scatter` |
| `scatter_nb` | `MPI_Iscatter` |

#### Reduce — `reduce/`
Global reduction (`MPI_SUM` over `double`) to root only.

| Binary | MPI primitive |
|--------|--------------|
| `reduce_b` | `MPI_Reduce` |
| `reduce_nb` | `MPI_Ireduce` |

#### Reduce-scatter — `reduce_scatter/`
Reduction followed by a scatter of the result chunks.

| Binary | MPI primitive |
|--------|--------------|
| `reduce_scatter_b` | `MPI_Reduce_scatter` |
| `reduce_scatter_nb` | `MPI_Ireduce_scatter` |

---

### Point-to-point patterns

#### Ping-pong — `pingpong/`
Latency measurement between a pair of ranks.

| Binary | Description |
|--------|------------|
| `pingpong_b` | Single pair (rank 0 ↔ rank 1) |
| `pingpong_pairwise_b` | All pairs simultaneously |

#### Incast — `incast/`
All non-root ranks send to root simultaneously.

| Binary | Variant |
|--------|---------|
| `incast_b` | Blocking receive at root |
| `incast_nb` | Non-blocking (`MPI_Irecv` at root) |
| `incast_bsnbr` | Non-blocking send, blocking receive |
| `incast_get` | One-sided: root issues `MPI_Get` from each sender |
| `incast_put` | One-sided: senders issue `MPI_Put` to root window |

#### Pairwise — `pairwise/`
Each rank exchanges messages with exactly one partner.  Partners are chosen by offset or at random.

| Binary | Variant |
|--------|---------|
| `pairwise_b` | Blocking (`MPI_Sendrecv`) |
| `pairwise_nb` | Non-blocking (`MPI_Isend` / `MPI_Irecv`) |
| `pairwise_bsnbr` | Non-blocking send, blocking receive |

Extra flags: `-offset <O>` (default `1`) — fixed partner offset; `-mode <M>` (default `offpair`) — pairing mode (`offpair` for fixed offset, `rand` for random pairing).

#### Ring — `ring/`
Each rank sends to its right neighbour and receives from its left.

| Binary | Variant |
|--------|---------|
| `ring_nb` | Non-blocking (`MPI_Isend` / `MPI_Irecv`) |
| `ring_bsnbr` | Non-blocking send, blocking receive |

Extra flag: `-rring` — randomise the ring order instead of using rank order.

---

### Synthetic / application-inspired patterns

#### 2-D stencil — `stencil/`
Each rank exchanges halo data with its four Cartesian neighbours (north, south, west, east).  Uses an `MPI_Cart_create` communicator.  Boundary ranks have `MPI_PROC_NULL` neighbours which complete immediately with no data transfer, so no special-casing is needed for edge ranks.

Binary: `stencil_2d_nb`

Extra flags:

| Flag | Default | Description |
|------|---------|-------------|
| `-dimx <X>` | `0` | Fix the X dimension of the process grid; Y is derived as `w_size / X`.  `0` lets `MPI_Dims_create` choose automatically. |
| `-periodic` | off | Make the Cartesian grid periodic (toroidal) in both dimensions |

```bash
mpirun -n 16 build/bin/stencil_2d_nb -msgsize 65536 -iter 100 -dimx 4
```

#### k-random partners — `kpartners/`
Each rank communicates with *k* randomly chosen partners per iteration.  The partner sets are derived from *k* independent random permutations (same seed on all ranks), guaranteeing that the resulting graph is k-regular: every rank sends exactly *k* messages and receives exactly *k* messages.

Binary: `kpartners_nb`

Extra flag:

| Flag | Default | Description |
|------|---------|-------------|
| `-k <K>` | `1` | Number of random partners per rank (capped at `w_size - 1`) |

```bash
mpirun -n 32 build/bin/kpartners_nb -k 4 -msgsize 4096 -iter 200
```

---

## Output format

### CSV (default)

All benchmarks print a CSV block to stdout on the master rank.  Each line corresponds to one measured iteration:

```
Average,Minimum,Maximum,Median,MainRank
<avg>,<min>,<max>,<median>,<master_rank_latency>
...
Ran N iterations. Measured M iterations.
```

Times are in seconds (9 decimal places).  The `Average`, `Minimum`, `Maximum`, and `Median` columns are computed across all MPI ranks for that iteration.  `MainRank` is the raw value recorded on the master rank.

### Pretty-print

Pass `-pretty-print` to get a human-readable table with auto-scaled units (`us` / `ms` / `s`) instead of CSV:

```bash
mpirun -n 8 build/bin/alltoall_nb -iter 5 -pretty-print
```

```
     #          avg          min          max       median
  -------------------------------------------------------------
      1      12.45 us      9.12 us     18.67 us     11.34 us
      2      11.98 us      9.08 us     17.23 us     11.12 us
  -------------------------------------------------------------
  5 samples · 10 iterations total
```

The `MainRank` column is omitted in pretty mode.  Pipe through a tool such as `sed 's/\x1b\[[0-9;]*m//g'` to strip ANSI colour codes if needed.

## Early termination

Send `SIGUSR1` to the master process to trigger a clean shutdown: remaining iterations are skipped, results collected so far are printed, and all ranks call `MPI_Finalize`.

```bash
kill -USR1 <pid_of_rank_0>
```
