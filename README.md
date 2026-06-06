<div align="center">
  <img src="assets/blink.svg" width="600" alt="Blink logo" />
</div>

<div align="center">

[![CI](https://github.com/DanieleDeSensi/blink/actions/workflows/ci.yml/badge.svg?branch=dev)](https://github.com/DanieleDeSensi/blink/actions/workflows/ci.yml?query=branch%3Adev)
[![codecov](https://codecov.io/gh/DanieleDeSensi/blink/branch/dev/graph/badge.svg)](https://codecov.io/gh/DanieleDeSensi/blink/tree/dev)

</div>


Blink is a collection of MPI benchmarks designed for long-running, in-situ measurement of network behaviour under realistic traffic conditions.  Each benchmark runs for a configurable number of iterations (or endlessly until interrupted), records per-iteration latency on every rank, and emits a CSV summary when it finishes — either naturally or via `SIGUSR1`.

## How Blink differs from other benchmarking suites

Most MPI benchmark suites run a fixed iteration sweep and report a single aggregate table (min / median / max per message size) before exiting. Blink takes a different approach on three axes:

**Temporal evolution.** Blink records one measurement per iteration rather than collapsing everything into a final summary. This lets you observe how latency evolves over time, detect jitter events, and correlate performance spikes with external activity on the system.

**Burst traffic modeling.** Real applications rarely issue communication at a steady, uniform rate. Blink supports configurable burst-pause cycles with pluggable inter-arrival distributions — exponential, Pareto, and log-normal — so the generated traffic pattern can more closely reflect the bursty, heavy-tailed nature of production workloads.

**Long-running operation.** Blink is designed to run alongside real workloads or as a background monitor. It can run endlessly (`-endl`), keeps only the most recent *N* samples in a ring buffer (`-maxsamples`), and responds to `SIGUSR1` with a clean shutdown — collecting and printing whatever has been measured so far before calling `MPI_Finalize`.

## Building

Requirements: CMake ≥ 3.16, an MPI implementation (e.g. OpenMPI, MPICH, Intel MPI).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Binaries are placed in `build/bin/`.  Adding a new `.c` or `.cpp` file anywhere under `src/` is enough — CMake picks it up automatically on the next configure.


## Testing

Correctness tests are built automatically alongside the benchmarks (requires GTest >= 1.14, fetched automatically if not found on the system).  Pass `-DBLINK_TESTS=OFF` to skip them.

Each test is a single-process GTest binary that spawns the actual benchmark under `mpirun -n N <binary> -debug` via `popen()` and parses structured `DEBUG key=val` output.  Most suites require **8 MPI ranks**; `test_pingpong` also covers a 2-rank case for `pingpong_b`.

### Run all tests

```bash
ctest --test-dir build --output-on-failure
```

| Useful flags | Effect |
|---|---|
| `-j <N>` | Run up to N test suites in parallel |
| `-V` | Always print test output (verbose) |
| `--output-on-failure` | Print output only on failure |
| `-R <regex>` | Run only suites whose name matches |

### Run a single test suite

```bash
ctest --test-dir build -R test_collectives --output-on-failure
```

### Test suites

| Suite | Benchmarks covered | Properties verified |
|---|---|---|
| `test_collectives` | `allgather_b/nb/comm_only`, `allreduce_b/nb`, `alltoall_b/nb/man/comm_only`, `barrier_b/nb`, `broadcast_b/nb`, `gather_b/nb`, `scatter_b/nb`, `reduce_b/nb`, `reduce_scatter_b/nb` | Coverage, communicator size, root rank, data integrity (plain + burst), b/nb agreement, allgather transfer-volume agreement |
| `test_pairwise` | `pairwise_b/nb/bsnbr` | Coverage, validity, symmetry (offpair/rpair), bijection, exact pairs, non-reciprocal modes (perm/rot) (plain + burst) |
| `test_ring` | `ring_nb`, `ring_bsnbr` | Coverage, validity, exact sequential neighbours, consistency (sequential + random), completeness (plain + burst) |
| `test_incast` | `incast_b/nb/bsnbr/get/put` | Coverage, roles (one receiver, N-1 senders), target correctness (plain + burst) |
| `test_pingpong` | `pingpong_b` (2 ranks), `pingpong_pairwise_b` (8 ranks) | Coverage, communicator size, data integrity (plain + burst) |
| `test_kpartners` | `kpartners_nb` | Coverage, communicator size, k value, slot coherence (plain + burst) |
| `test_stencil` | `stencil_2d_nb` | Coverage, communicator size, data integrity, neighbour-graph consistency (plain, periodic, burst) |
| `test_misc` | cross-cutting (`barrier_nb`, `incast_b`, `dist_test`, `alltoall_nb`) | Ring-buffer wrap, warm-up exclusion, pretty-print formatter, `-mrand` determinism, sampler self-check, SIGUSR1 clean shutdown |

### Overriding MPI launcher

The test binaries bake in the MPI launcher paths at configure time.  Override them at runtime with environment variables:

```bash
BLINK_MPIEXEC=/opt/mpi/bin/mpirun BLINK_NPROC_FLAG=-np BLINK_BIN_DIR=/custom/bin ctest --test-dir build
```

### Continuous integration

Every push and pull request triggers the [CI workflow](.github/workflows/ci.yml): it
builds every benchmark and the test suite on Ubuntu with OpenMPI, runs `ctest`, and
produces a coverage report.  The badges at the top of this file show the latest result.

The build is instrumented via `-DBLINK_COVERAGE=ON` (`-O0 --coverage`, atomic counters
so concurrent MPI ranks don't corrupt the `.gcda` files); a [`gcovr`](https://gcovr.com)
summary of `src/` is printed to the run's job summary, attached as an artifact, and
uploaded to Codecov.  To reproduce a coverage run locally:

```bash
cmake -B build-cov -DBLINK_TESTS=ON -DBLINK_COVERAGE=ON
cmake --build build-cov -j$(nproc)
ctest --test-dir build-cov --output-on-failure
gcovr --root . --filter 'src/' --print-summary
```

> **Coverage badge setup (one-time):** the test badge works out of the box.  To activate
> the coverage badge, sign in at <https://codecov.io> with GitHub, enable this repository,
> and add a `CODECOV_TOKEN` repository secret (Settings → Secrets and variables → Actions).
> Until then CI still publishes the coverage summary in each run's job-summary and artifact.


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
| `-blength <s>` | `0` | Mean burst length in seconds (0 = single shot per iteration) |
| `-bldist <D>` | — | Randomise burst length using distribution `D` (`exp`, `pareto`, `lognormal`) |
| `-blshape <S>` | `1.5` | Shape parameter for burst distribution (α for Pareto, σ for log-normal) |
| `-bpause <s>` | `0` | Mean pause between bursts in seconds |
| `-bpdist <D>` | — | Randomise pause length using distribution `D` (`exp`, `pareto`, `lognormal`) |
| `-bpshape <S>` | `1.5` | Shape parameter for pause distribution (α for Pareto, σ for log-normal) |
| `-pretty-print` | off | Human-readable table output instead of CSV (see [Output format](#output-format)) |

### Burst distributions

When `-bldist` or `-bpdist` is supplied, burst lengths / pauses are drawn independently on each iteration from the chosen distribution, parameterised so that the empirical mean equals `-blength` / `-bpause` regardless of the shape parameter:

| Distribution | Flag value | Shape parameter | Notes |
|---|---|---|---|
| Exponential | `exp` | — (ignored) | Light-tailed; fully determined by its mean |
| Pareto | `pareto` | α (`-blshape` / `-bpshape`, must be > 1) | Heavy-tailed; α ≤ 2 → infinite variance |
| Log-normal | `lognormal` | σ (`-blshape` / `-bpshape`, > 0) | Log-symmetric; larger σ → heavier tail |

```bash
# Pareto bursts (α=2, heavy tail) with exponential pauses
mpirun -n 8 build/bin/alltoall_nb -iter 500 \
    -blength 0.01 -bldist pareto  -blshape 2.0 \
    -bpause  0.05 -bpdist exp
```

The sampler implementations can be verified independently:

```bash
mpirun -n 1 build/bin/dist_test
```

## Auxiliary tools

Besides the benchmarks, `src/tools/` builds a few helper binaries:

| Binary | Purpose |
|--------|---------|
| `dist_test` | Verifies the burst-distribution samplers against their theoretical CDFs (ASCII histogram + mean / variance checks + KS test).  Run single-rank: `mpirun -n 1 build/bin/dist_test`. |
| `checker` | An all-to-all workload (like `alltoall_b`) that additionally writes a per-iteration wall-clock timestamp log on the master rank (`checker_<YYYYMMDD_HHMMSS>.log`, local time) — useful for correlating throughput dips with external system activity over a long run.  Accepts the common flags. |
| `null_dummy` | Bare `MPI_Init` / `MPI_Finalize` with no communication; a baseline for measuring MPI startup / teardown overhead. |

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
Global reduction (`MPI_SUM` over `int`) delivered to all ranks.

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
Global reduction (`MPI_SUM` over `int`) to root only.

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

Extra flags: `-offset <O>` (default `1`) — fixed partner offset; `-mode <M>` (default `offpair`) — pairing mode. Supported modes:

| Mode | Meaning |
|------|---------|
| `offpair` | Disjoint reciprocal pairs at fixed offset `O` (e.g. `(0,1) (2,3) …`) |
| `rpair` | Uniformly random disjoint pairs (same seed on all ranks) |
| `perm` | Random permutation: each rank sends to one partner (not necessarily reciprocal) |
| `rot` | Ring rotation: rank `i` sends to `(i+O) mod n` |

#### Ring — `ring/`
Each rank simultaneously sends to both its left and right neighbours and receives one message from each (bidirectional ring exchange).

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
