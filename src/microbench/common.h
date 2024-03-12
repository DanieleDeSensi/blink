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

/*global variables because of signal handling*/
static int my_rank;
static int w_size;
static int master_rank;
static int curr_iters;
static int warm_up_iters;
static int max_samples;
static double *durations;

static void write_results()
{
    double duration_sum;
    double duration_median;
    int num_samples;
    int i;
    int start_index;    
    double *tmp_buf = NULL;

    if (curr_iters - warm_up_iters > max_samples) // Wrapped the sampling recording
    {
        num_samples = max_samples;
        start_index = curr_iters % max_samples;        
        tmp_buf = (double *)malloc(sizeof(double)*num_samples);
        // Copy the data from the durations buffer to tmp_buf (so that it is in the proper order)
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
    

    double *all_data =  (double *)malloc(sizeof(double)*num_samples*w_size);
    double *sorting_buf = (double*)malloc(sizeof(double)*w_size);
    
    if(all_data == NULL || sorting_buf == NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }

    /*print file header*/
    if (my_rank == master_rank)
    {
        printf("Average,Minimum,Maximum,Median,MainRank\n");
    }

    // We need to first gather all the data
    MPI_Gather(tmp_buf, num_samples, MPI_DOUBLE, all_data, num_samples, MPI_DOUBLE, master_rank, MPI_COMM_WORLD);

    if(my_rank == master_rank){   
        for(i = 0; i < num_samples; i++){
            int j;
            duration_sum = 0;
            for(j = 0; j < w_size; j++){
                sorting_buf[j] = all_data[j*num_samples + i];
                duration_sum += sorting_buf[j];
            }

            qsort(sorting_buf, w_size, sizeof(double), compare_doubles);
            if (w_size % 2 == 0)
            { /*even: then median as mean of middle values*/
                duration_median = (sorting_buf[(w_size - 1) / 2] + sorting_buf[w_size / 2]) / 2;
            }
            else
            { /*odd: else median as middle value*/
                duration_median = sorting_buf[(w_size - 1) / 2];
            }
            printf("%.9f,%.9f,%.9f,%.9f,%.9f\n", duration_sum / w_size, sorting_buf[0], sorting_buf[w_size - 1], duration_median, tmp_buf[i]);
        }

        printf("Ran %d iterations. Measured %d iterations.\n", curr_iters, num_samples);
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

/*use Fisher-Yates to permute array*/
static void permute(int *a, int n)
{
    int j;
    int t;
    int i;
    for (i = n; i > 1; i--)
    {
        j = rand() % i;
        t = a[i - 1];
        a[i - 1] = a[j];
        a[j] = t;
    }
}

/*mathematical mod without negativ numbers*/
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
    {
        a[i] = -1;
    }
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
    int i;
    for (i = 0; i < n; i++)
    {
        a[i] = -1;
    }
    int t;
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
