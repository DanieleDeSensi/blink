#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <sched.h>

/* ── random duration samplers ────────────────────────────────────────────── */

/* Exponential: shape parameter accepted for API uniformity but not used —
 * the distribution is fully determined by its mean.                         */
static inline double rand_exp(double mean, double shape)
{
    (void)shape;
    double u = (rand() + 0.5) / (RAND_MAX + 1.0); /* shift away from 0 and 1 */
    return -mean * log(1.0 - u);
}

/* Pareto: shape = tail exponent α.  Must be > 1 for a finite mean.
 * Parameterised so that E[X] = mean regardless of α.
 * Inverse-CDF method: x = x_m * u^(-1/α), u ~ Uniform(0,1).               */
static inline double rand_pareto(double mean, double shape)
{
    double alpha = shape;
    if (alpha <= 1.0) {
        fprintf(stderr, "rand_pareto: shape (alpha) must be > 1, got %g\n", alpha);
        exit(-1);
    }
    if (mean <= 0.0) {
        fprintf(stderr, "rand_pareto: mean must be > 0, got %g\n", mean);
        exit(-1);
    }
    double x_m = mean * (alpha - 1.0) / alpha;
    double u   = (rand() + 0.5) / (RAND_MAX + 1.0); /* shift away from 0 */
    return x_m * pow(u, -1.0 / alpha);
}

/* Log-normal: shape = σ of the underlying normal.
 * Parameterised so that E[X] = mean regardless of σ.
 * Box-Muller transform.                                                      */
static inline double rand_lognormal(double mean, double sigma)
{
    if (mean <= 0.0) {
        fprintf(stderr, "rand_lognormal: mean must be > 0, got %g\n", mean);
        exit(-1);
    }
    if (sigma <= 0.0) {
        fprintf(stderr, "rand_lognormal: sigma must be > 0, got %g\n", sigma);
        exit(-1);
    }
    double mu = log(mean) - 0.5 * sigma * sigma;
    double u1 = (rand() + 0.5) / (RAND_MAX + 1.0); /* shift away from 0 */
    double u2 = (rand() + 0.5) / (RAND_MAX + 1.0);
    double z  = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return exp(mu + sigma * z);
}

/* Dispatch to the selected sampler. */
static inline double rand_duration(double mean, const char *dist, double shape)
{
    if (strcmp(dist, "pareto")    == 0) return rand_pareto(mean, shape);
    if (strcmp(dist, "lognormal") == 0) return rand_lognormal(mean, shape);
    return rand_exp(mean, shape);
}

/*sleep seconds given as double*/
static inline int dsleep(double t)
{
    struct timespec t1, t2;
    t1.tv_sec = (long)t;
    t1.tv_nsec = (t - t1.tv_sec) * 1000000000L;
    return nanosleep(&t1, &t2);
}

/*double comparison function for quicksort*/
static inline int compare_doubles(const void *p1, const void *p2)
{
    if (*(double *)p1 < *(double *)p2)
        return -1;
    else if (*(double *)p1 > *(double *)p2)
        return 1;
    else
        return 0;
}

/* ── globals (shared by signal handler and measurement loop) ─────────────── */
static int    my_rank;
static int    w_size;
static int    master_rank     = 0;
static long long curr_iters;       /* total outer iterations executed (warmup + measured) */
static long long measured_iters;   /* number of recorded samples (excludes warmup) */
static int    warm_up_iters   = 5;
static int    max_samples     = 1000;
static double *durations;

/* Async-signal-safe shutdown flag set by sig_handler; checked in measurement
 * loops. The signal handler does no MPI / no malloc / no stdio — only sets
 * this flag. The main thread observes it between iterations and exits the
 * loop cleanly, then calls write_results() + MPI_Finalize() itself.        */
static volatile sig_atomic_t shutdown_requested = 0;

/* common benchmark parameters – initialised to defaults, set by parse_common_args() */
static int    msg_size            = 1024;
static int    measure_granularity = 1;
static int    rand_seed           = 1;
static int    max_iters           = 1;
static int    endless             = 0;
static double burst_length        = 0.0;
static int    burst_length_rand   = 0;   /* set by -bldist */
static double burst_pause         = 0.0;
static int    burst_pause_rand    = 0;   /* set by -bpdist */
static int    pretty_output       = 0;
static int    debug_mode          = 0;

/* distribution selection for burst length and pause (-bldist / -bpdist).
 * Values: "exp", "pareto", "lognormal".  shape: α for Pareto, σ for log-normal
 * (ignored for exp).  Defaults give exponential with shape unused.          */
static char   burst_dist[16]  = "exp";
static double burst_shape     = 1.5;
static char   pause_dist[16]  = "exp";
static double pause_shape     = 1.5;

/* Bounds-checked accessor for a flag's value argument.  Aborts with a clear
 * message instead of dereferencing argv[argc] (== NULL) when a value-taking
 * flag is supplied as the final command-line token.                         */
static inline const char *arg_value(int argc, char **argv, int *i)
{
    if (*i + 1 >= argc) {
        if (my_rank == master_rank)
            fprintf(stderr, "Missing value for option %s\n", argv[*i]);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    return argv[++(*i)];
}

/* Validate a distribution name / shape pair selected via -bldist/-bpdist so a
 * misconfiguration fails loudly at startup rather than silently producing
 * garbage durations later (e.g. log-normal with a non-positive mean).        */
static inline void validate_burst_dist(const char *what, const char *dist,
                                double mean, double shape)
{
    if (mean <= 0.0) {
        if (my_rank == master_rank)
            fprintf(stderr, "%s: randomised distribution requires a positive mean, got %g\n",
                    what, mean);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if (strcmp(dist, "pareto") == 0 && shape <= 1.0) {
        if (my_rank == master_rank)
            fprintf(stderr, "%s: pareto shape (alpha) must be > 1, got %g\n", what, shape);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if (strcmp(dist, "lognormal") == 0 && shape <= 0.0) {
        if (my_rank == master_rank)
            fprintf(stderr, "%s: lognormal shape (sigma) must be > 0, got %g\n", what, shape);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
}

/*
 * Parse the standard set of command-line flags shared by every benchmark.
 * Unrecognised flags are compacted to the front of argv (after argv[0]) and
 * the new argc is returned, so each benchmark can do a second pass for its
 * own flags.  Also seeds the RNG and resolves -mrand after parsing.
 */
static inline int parse_common_args(int argc, char **argv)
{
    int new_argc       = 1; /* always keep argv[0] (program name) */
    int do_rand_master = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-mrank")        == 0) { master_rank         = atoi(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-mrand")        == 0) { do_rand_master      = 1;               }
        else if (strcmp(argv[i], "-msgsize")      == 0) { msg_size            = atoi(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-endl")         == 0) { endless             = 1;               }
        else if (strcmp(argv[i], "-iter")         == 0) { max_iters           = atoi(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-warmup")       == 0) { warm_up_iters       = atoi(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-blength")      == 0) { burst_length        = atof(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-bpause")       == 0) { burst_pause         = atof(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-bldist")       == 0) { strncpy(burst_dist, arg_value(argc, argv, &i), 15);
                                                          burst_dist[15] = '\0';
                                                          burst_length_rand = 1;                  }
        else if (strcmp(argv[i], "-bpdist")       == 0) { strncpy(pause_dist, arg_value(argc, argv, &i), 15);
                                                          pause_dist[15] = '\0';
                                                          burst_pause_rand  = 1;                  }
        else if (strcmp(argv[i], "-blshape")      == 0) { burst_shape        = atof(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-bpshape")      == 0) { pause_shape        = atof(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-seed")         == 0) { rand_seed           = atoi(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-grty")         == 0) { measure_granularity = atoi(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-maxsamples")   == 0) { max_samples         = atoi(arg_value(argc, argv, &i)); }
        else if (strcmp(argv[i], "-pretty-print") == 0) { pretty_output       = 1;               }
        else if (strcmp(argv[i], "-debug")        == 0) { debug_mode          = 1;               }
        else { argv[new_argc++] = argv[i]; } /* pass through unknown flags */
    }

    /* fail fast on nonsensical numeric parameters */
    if (max_samples < 1) {
        if (my_rank == master_rank)
            fprintf(stderr, "-maxsamples must be >= 1, got %d\n", max_samples);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if (measure_granularity < 1) {
        if (my_rank == master_rank)
            fprintf(stderr, "-grty must be >= 1, got %d\n", measure_granularity);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    /* a randomised burst/pause distribution needs a positive mean and a valid
     * shape — validate now so we never feed log(0)/negative values to a sampler */
    if (burst_length_rand)
        validate_burst_dist("-bldist", burst_dist, burst_length, burst_shape);
    if (burst_pause_rand)
        validate_burst_dist("-bpdist", pause_dist, burst_pause, pause_shape);

    /* seed RNG (shared across all ranks) and resolve randomised master */
    srand(rand_seed);
    if (do_rand_master)
        master_rank = rand() % w_size;

    /* validate the resolved master rank is a real rank.  An out-of-range -mrank
     * would otherwise become an invalid root in MPI_Gather/Bcast/Reduce, and can
     * deadlock benchmarks that branch on (my_rank == master_rank).  Gate the
     * message on rank 0 because master_rank itself may be the bad value.       */
    if (master_rank < 0 || master_rank >= w_size) {
        if (my_rank == 0)
            fprintf(stderr, "-mrank must be in [0, %d), got %d\n", w_size, master_rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    return new_argc;
}

/* Convenience wrappers used by benchmark measurement loops. */
static inline double sample_burst_length(double mean) { return rand_duration(mean, burst_dist, burst_shape); }
static inline double sample_pause_length(double mean)  { return rand_duration(mean, pause_dist, pause_shape); }

/* Record a single measured-iteration latency into the ring buffer.
 * Skips writes during warm-up: callers gate this by `if (k >= warm_up_iters)`
 * at the outer iteration level — see the standard benchmark loop.
 *
 * The ring buffer of size `max_samples` therefore only ever holds measured
 * samples (no warmup pollution); the (oldest → newest) ordering is
 * (measured_iters % max_samples) ... (measured_iters - 1) % max_samples.   */
static inline void record_duration(double t)
{
    durations[measured_iters % max_samples] = t;
    measured_iters++;
}

/* ── output helpers ──────────────────────────────────────────────────────── */

/*format a duration (seconds) with auto-scaled units; "%9.2f UU" is normally
 * 12 chars wide (9 is a minimum field width, so extreme outliers can exceed it)*/
static inline void format_duration(char *buf, size_t len, double t)
{
    if (t < 1e-3)
        snprintf(buf, len, "%9.2f us", t * 1e6);
    else if (t < 1.0)
        snprintf(buf, len, "%9.2f ms", t * 1e3);
    else
        snprintf(buf, len, "%9.2f  s", t);
}

static inline void write_results()
{
    double duration_sum;
    double duration_median;
    int num_samples;
    int i;
    int start_index;
    double *tmp_buf = NULL;

    if (measured_iters > max_samples) /* wrapped the sampling ring buffer */
    {
        num_samples = max_samples;
        start_index = (int)(measured_iters % max_samples);
    }
    else
    {
        num_samples = (int)measured_iters;
        start_index = 0;
    }

    /* MPI_Gather below uses num_samples as BOTH send- and recv-count, which is
     * only valid if every rank contributes the same count.  Ranks normally stay
     * in lockstep, but to be robust (e.g. a SIGUSR1 shutdown that reaches ranks
     * at slightly different iterations) collectively agree on the common minimum
     * so the gather can never mismatch.                                        */
    {
        int common_samples = num_samples;
        MPI_Allreduce(&num_samples, &common_samples, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        /* keep only the most recent common_samples (drop the oldest extras) */
        start_index += (num_samples - common_samples);
        num_samples  = common_samples;
    }

    tmp_buf = (double *)malloc(sizeof(double) * (num_samples > 0 ? num_samples : 1));
    /* copy in chronological order (oldest → newest) using circular indexing */
    for (i = 0; i < num_samples; i++) {
        tmp_buf[i] = durations[(start_index + i) % max_samples];
    }

    double *all_data    = (double *)malloc(sizeof(double)*(num_samples > 0 ? num_samples : 1)*w_size);
    double *sorting_buf = (double *)malloc(sizeof(double)*w_size);

    if (all_data == NULL || sorting_buf == NULL) {
        fprintf(stderr, "Failed to allocate a buffer on rank %d\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    /* print header before the gather so it appears first */
    if (my_rank == master_rank) {
        if (pretty_output) {
            printf("\033[1m\033[36m"
                   "  %5s  %12s  %12s  %12s  %12s"
                   "\033[0m\n",
                   "#", "avg", "min", "max", "median");
            printf("\033[2m"
                   "  -------------------------------------------------------------"
                   "\033[0m\n");
        } else {
            printf("Average,Minimum,Maximum,Median,MainRank\n");
        }
    }

    MPI_Gather(tmp_buf, num_samples, MPI_DOUBLE,
               all_data, num_samples, MPI_DOUBLE,
               master_rank, MPI_COMM_WORLD);

    if (my_rank == master_rank) {
        for (i = 0; i < num_samples; i++) {
            int j;
            duration_sum = 0;
            for (j = 0; j < w_size; j++) {
                sorting_buf[j] = all_data[j*num_samples + i];
                duration_sum  += sorting_buf[j];
            }

            qsort(sorting_buf, w_size, sizeof(double), compare_doubles);
            if (w_size % 2 == 0) /* even: median as mean of two middle values */
                duration_median = (sorting_buf[(w_size-1)/2] + sorting_buf[w_size/2]) / 2.0;
            else                 /* odd: median is the middle value */
                duration_median = sorting_buf[(w_size-1)/2];

            if (pretty_output) {
                char avg_s[24], min_s[24], max_s[24], med_s[24];
                format_duration(avg_s, sizeof(avg_s), duration_sum / w_size);
                format_duration(min_s, sizeof(min_s), sorting_buf[0]);
                format_duration(max_s, sizeof(max_s), sorting_buf[w_size-1]);
                format_duration(med_s, sizeof(med_s), duration_median);
                printf("  %5d  %12s  %12s  %12s  %12s\n",
                       i+1, avg_s, min_s, max_s, med_s);
            } else {
                printf("%.9f,%.9f,%.9f,%.9f,%.9f\n",
                       duration_sum / w_size,
                       sorting_buf[0], sorting_buf[w_size-1],
                       duration_median, tmp_buf[i]);
            }
        }

        if (pretty_output) {
            printf("\033[2m"
                   "  -------------------------------------------------------------"
                   "\033[0m\n");
            printf("  %d samples · %lld iterations total\n", num_samples, curr_iters);
        } else {
            printf("Ran %lld iterations. Measured %d iterations.\n", curr_iters, num_samples);
        }
        fflush(stdout);
    }

    free(sorting_buf);
    free(all_data);
    free(tmp_buf);
}

/* Async-signal-safe handler: only sets a flag.  The measurement loop in each
 * benchmark observes the flag and exits cleanly; write_results() and
 * MPI_Finalize() happen on the main thread, never from signal context.    */
static inline void sig_handler(int sig)
{
    (void)sig;
    shutdown_requested = 1;
}

/* Install the SIGUSR1 shutdown handler with sigaction() rather than signal(),
 * for portable, persistent (non-one-shot) semantics across platforms.       */
static inline void install_shutdown_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);
}

/* Collectively decide whether to shut down.  SIGUSR1 only sets the flag on the
 * rank that received it, so a purely local check would make ranks leave the
 * measurement loop at different iterations — desynchronising the per-iteration
 * collectives and the final MPI_Gather (deadlock / mismatch).  This all-reduce
 * (called once per outer iteration, OUTSIDE the timed region) guarantees every
 * rank breaks on the same iteration.  Returns non-zero if any rank requested
 * shutdown.                                                                   */
static inline int check_shutdown(void)
{
    int local = (int)shutdown_requested;
    int global = 0;
    MPI_Allreduce(&local, &global, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    return global;
}

/* ── combinatorics helpers ───────────────────────────────────────────────── */

/*use Fisher-Yates to permute array*/
static inline void permute(int *a, int n)
{
    int j, t, i;
    for (i = n; i > 1; i--)
    {
        j = rand() % i;
        t = a[i - 1];
        a[i - 1] = a[j];
        a[j] = t;
    }
}

/*mathematical mod without negative numbers*/
static inline int mod(int a, int b)
{
    int c = a % b;
    if (c < 0)
        c += b;
    return c;
}

/* Produce a uniformly random matching of [0..n-1] (each entry pairs with
 * exactly one other entry).  Algorithm: shuffle the index list, then pair
 * adjacent elements.  This is provably uniform over the set of perfect
 * matchings (and far simpler than the prior slot-counting version).
 *
 * For odd n the last element targets itself (it has no partner).
 */
static inline void random_pairs(int *a, int n)
{
    int i;
    int has_self = (n % 2 == 1);
    int pair_n   = has_self ? n - 1 : n;
    int *shuf    = (int *)malloc(sizeof(int) * n);

    if (shuf == NULL) {
        fprintf(stderr, "random_pairs: malloc failed\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    for (i = 0; i < n; i++) shuf[i] = i;
    permute(shuf, n);

    for (i = 0; i < n; i++) a[i] = -1;

    for (i = 0; i < pair_n; i += 2) {
        int x = shuf[i];
        int y = shuf[i + 1];
        a[x] = y;
        a[y] = x;
    }
    if (has_self) {
        /* odd n: one rank has no partner — target itself */
        int lone = shuf[n - 1];
        a[lone] = lone;
    }
    free(shuf);
}

/* Produce fixed-offset pairs: walks i from 0 upward and pairs i with (i+o)
 * whenever both ranks are still free and the partner is not a wrap-around
 * (n - i >= o).  This guarantees disjoint pairs (i, i+o), (i+2o, i+3o), …
 * — for offset o=1 you get (0,1) (2,3) (4,5)…, for o=2 you get (0,2)
 * (1,3) (4,6) (5,7)…, etc.
 *
 * Only non-wrapping reciprocal pairs (i, i+o) are formed; any rank that cannot
 * be matched without wrapping around falls through to a self-pair (a[i] = i,
 * i.e. it performs no communication).  This happens for every offset > n/2, and
 * also for many offsets <= n/2 when n is not a multiple of 2*o (e.g. n=12, o=4
 * leaves ranks 8..11 self-paired).  Full disjoint tiling is only guaranteed when
 * 2*o divides n (notably o=1).  Recommended usage: 1 <= o <= n/2.
 */
static inline void offset_pairs(int *a, int n, int o)
{
    int i, t;
    for (i = 0; i < n; i++)
        a[i] = -1;
    for (i = 0; i < n; i++)
    {
        t = mod(i + o, n);
        if (a[i] == -1 && a[t] == -1)
        {
            if (n - i >= o)
            {
                a[i] = t;
                a[t] = i;
            }
        }
    }
    for (i = 0; i < n; i++)
    {
        if (a[i] == -1)
            a[i] = i; /* no reciprocal partner — self */
    }
}

#define ALIGNMENT (sysconf(_SC_PAGESIZE))
static inline void* malloc_align(size_t size)
{
    void *p = NULL;
    int ret = posix_memalign(&p, ALIGNMENT, size);
    if (ret != 0) {
        fprintf(stderr, "Failed to allocate memory on rank\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    return p;
}
