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

/*draw a exponentially distributed number with expectation=mean*/
static double rand_expo(double mean)
{
    double lambda = 1.0 / mean;
    double u = rand() / (RAND_MAX + 1.0);
    return -log(1 - u) / lambda;
}

/*sleep seconds given as double*/
static int dsleep(double t)
{
    struct timespec t1, t2;
    t1.tv_sec = (long)t;
    t1.tv_nsec = (t - t1.tv_sec) * 1000000000L;
    return nanosleep(&t1, &t2);
}

/*double comparison function for quicksort*/
int compare_doubles(const void *p1, const void *p2)
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
static int    curr_iters;
static int    warm_up_iters   = 5;
static int    max_samples     = 1000;
static double *durations;

/* common benchmark parameters – initialised to defaults, set by parse_common_args() */
static int    msg_size            = 1024;
static int    measure_granularity = 1;
static int    rand_seed           = 1;
static int    max_iters           = 1;
static int    endless             = 0;
static double burst_length        = 0.0;
static int    burst_length_rand   = 0;
static double burst_pause         = 0.0;
static int    burst_pause_rand    = 0;
static int    pretty_output       = 0;

/*
 * Parse the standard set of command-line flags shared by every benchmark.
 * Unrecognised flags are compacted to the front of argv (after argv[0]) and
 * the new argc is returned, so each benchmark can do a second pass for its
 * own flags.  Also seeds the RNG and resolves -mrand after parsing.
 */
static int parse_common_args(int argc, char **argv)
{
    int new_argc       = 1; /* always keep argv[0] (program name) */
    int do_rand_master = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if      (strcmp(argv[i], "-mrank")        == 0) { master_rank         = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-mrand")        == 0) { do_rand_master      = 1;               }
        else if (strcmp(argv[i], "-msgsize")      == 0) { msg_size            = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-endl")         == 0) { endless             = 1;               }
        else if (strcmp(argv[i], "-iter")         == 0) { max_iters           = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-warmup")       == 0) { warm_up_iters       = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-blength")      == 0) { burst_length        = atof(argv[++i]); }
        else if (strcmp(argv[i], "-bpause")       == 0) { burst_pause         = atof(argv[++i]); }
        else if (strcmp(argv[i], "-bprand")       == 0) { burst_pause_rand    = 1;               }
        else if (strcmp(argv[i], "-blrand")       == 0) { burst_length_rand   = 1;               }
        else if (strcmp(argv[i], "-seed")         == 0) { rand_seed           = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-grty")         == 0) { measure_granularity = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-maxsamples")   == 0) { max_samples         = atoi(argv[++i]); }
        else if (strcmp(argv[i], "-pretty-print") == 0) { pretty_output       = 1;               }
        else { argv[new_argc++] = argv[i]; } /* pass through unknown flags */
    }

    /* seed RNG (shared across all ranks) and resolve randomised master */
    srand(rand_seed);
    if (do_rand_master)
        master_rank = rand() % w_size;

    return new_argc;
}

/* ── output helpers ──────────────────────────────────────────────────────── */

/*format a duration (seconds) into a fixed 12-char string with auto-scaled units*/
static void format_duration(char *buf, size_t len, double t)
{
    if (t < 1e-3)
        snprintf(buf, len, "%9.2f us", t * 1e6);
    else if (t < 1.0)
        snprintf(buf, len, "%9.2f ms", t * 1e3);
    else
        snprintf(buf, len, "%9.2f  s", t);
}

static void write_results()
{
    double duration_sum;
    double duration_median;
    int num_samples;
    int i;
    int start_index;
    double *tmp_buf = NULL;

    if (curr_iters > max_samples) /* wrapped the sampling ring buffer */
    {
        num_samples = max_samples;
        start_index = curr_iters % max_samples;
        tmp_buf = (double *)malloc(sizeof(double)*num_samples);
        /* copy in chronological order */
        memcpy(tmp_buf, &(durations[start_index]), sizeof(double)*(num_samples - start_index));
        memcpy(&tmp_buf[num_samples - start_index], durations, sizeof(double)*start_index);
    }
    else
    {
        num_samples = curr_iters - warm_up_iters;
        start_index = warm_up_iters;
        tmp_buf = (double *)malloc(sizeof(double)*num_samples);
        memcpy(tmp_buf, &(durations[start_index]), sizeof(double)*num_samples);
    }

    double *all_data    = (double *)malloc(sizeof(double)*num_samples*w_size);
    double *sorting_buf = (double *)malloc(sizeof(double)*w_size);

    if (all_data == NULL || sorting_buf == NULL) {
        fprintf(stderr, "Failed to allocate a buffer on rank %d\n", my_rank);
        exit(-1);
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
            printf("  %d samples · %d iterations total\n", num_samples, curr_iters);
        } else {
            printf("Ran %d iterations. Measured %d iterations.\n", curr_iters, num_samples);
        }
        fflush(stdout);
    }

    free(sorting_buf);
    free(all_data);
    free(tmp_buf);
}

/*signal handler*/
void sig_handler(int sig)
{
    write_results();
    MPI_Finalize();
    exit(0);
}

/* ── combinatorics helpers ───────────────────────────────────────────────── */

/*use Fisher-Yates to permute array*/
static void permute(int *a, int n)
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
static int mod(int a, int b)
{
    int c = a % b;
    if (c < 0)
        c += b;
    return c;
}

/*produce random pairs*/
static void random_pairs(int *a, int n)
{
    int i, j;
    for (i = 0; i < n; i++)
        a[i] = -1;
    if (n % 2 == 1)
    {
        n--;
        a[n] = n; /*if odd last rank targets itself*/
    }
    int k = 1;
    int t;
    for (i = 0; i < n; i++)
    {
        if (a[i] == -1)
        {
            t = rand() % (n - k);
            for (j = i + 1; j < n; j++)
            {
                if (a[j] == -1)
                {
                    if (t == 0)
                    {
                        a[i] = j;
                        a[j] = i;
                        k += 2;
                        break;
                    }
                    t--;
                }
            }
        }
    }
}

/*produce fixed offset pairs*/
static void offset_pairs(int *a, int n, int o)
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
            a[i] = i;
    }
}

#define ALIGNMENT (sysconf(_SC_PAGESIZE))
static void* malloc_align(size_t size)
{
    void *p = NULL;
    int ret = posix_memalign(&p, ALIGNMENT, size);
    if (ret != 0) {
        fprintf(stderr, "Failed to allocate memory on rank\n");
        exit(-1);
    }
    return p;
}
