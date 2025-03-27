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
#include "common.h"


static inline int copy_buffer_different_dt (const void *input_buffer, size_t scount,
    const MPI_Datatype sdtype, void *output_buffer,
    size_t rcount, const MPI_Datatype rdtype) {

    int sdtype_size;
    MPI_Type_size(sdtype, &sdtype_size);
    int rdtype_size;
    MPI_Type_size(rdtype, &rdtype_size);

    size_t s_size = (size_t) sdtype_size * scount;
    size_t r_size = (size_t) rdtype_size * rcount;

    if(r_size < s_size) {
        memcpy(output_buffer, input_buffer, r_size); // Copy as much as possible
        return MPI_ERR_TRUNCATE;      // Indicate truncation
    }

    memcpy(output_buffer, input_buffer, s_size);        // Perform the memory copy

    return MPI_SUCCESS;
}

static inline int next_poweroftwo(int value)
{

  if(0 == value) {
    return 1;
  }

  return 1 << (8 * sizeof(int) - __builtin_clz(value));
}

int reduce_scatter_recursivehalving(const void *sbuf, void *rbuf, const int rcounts[],
    MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
    int i, rank, size, err = MPI_SUCCESS;
    int tmp_size, remain = 0, tmp_rank;
    size_t count;
    ptrdiff_t *disps = NULL;
    ptrdiff_t extent, true_extent, lb, buf_size, gap = 0;
    char *recv_buf = NULL, *recv_buf_free = NULL;
    char *result_buf = NULL, *result_buf_free = NULL;

    err = MPI_Comm_size(comm, &size);
    err = MPI_Comm_rank(comm, &rank);

    /* Find displacements and the like */
    disps = (ptrdiff_t*) malloc(sizeof(ptrdiff_t) * size);

    disps[0] = 0;
    for(i = 0; i < (size - 1); ++i) {
        disps[i + 1] = disps[i] + rcounts[i];
    }
    count = disps[size - 1] + rcounts[size - 1];

    /* short cut the trivial case */
    if(0 == count) {
        free(disps);
        return MPI_SUCCESS;
    }

    /* get datatype information */
    MPI_Type_get_extent(dtype, &lb, &extent);
    MPI_Type_get_true_extent(dtype, &gap, &true_extent);

    // Calculate the total memory span
    buf_size = true_extent + extent * (count - 1);

    /* Handle MPI_IN_PLACE */
    if(MPI_IN_PLACE == sbuf) {
        sbuf = rbuf;
    }

    /* Allocate temporary receive buffer. */
    recv_buf_free = (char*) malloc(buf_size);
    recv_buf = recv_buf_free - gap;

    /* allocate temporary buffer for results */
    result_buf_free = (char*) malloc(buf_size);
    result_buf = result_buf_free - gap;

    /* copy local buffer into the temporary results */
    err = copy_buffer_different_dt(sbuf, count, dtype, result_buf, count, dtype);

    /* figure out power of two mapping: grow until larger than
    comm size, then go back one, to get the largest power of
    two less than comm size */
    tmp_size = next_poweroftwo (size);
    tmp_size >>= 1;
    remain = size - tmp_size;

    /* If comm size is not a power of two, have the first "remain"
    procs with an even rank send to rank + 1, leaving a power of
    two procs to do the rest of the algorithm */
    if(rank < 2 * remain) {
        if((rank & 1) == 0) {
            err = MPI_Send(result_buf, count, dtype, rank + 1, 0, comm);

            /* we don't participate from here on out */
            tmp_rank = -1;
        } else {
            err = MPI_Recv(recv_buf, count, dtype, rank - 1, 0, comm, MPI_STATUS_IGNORE);

            /* integrate their results into our temp results */
            MPI_Reduce_local(recv_buf, result_buf, count, dtype, op);

            /* adjust rank to be the bottom "remain" ranks */
            tmp_rank = rank / 2;
        }
    } else {
        /* just need to adjust rank to show that the bottom "even
        remain" ranks dropped out */
        tmp_rank = rank - remain;
    }

    /* For ranks not kicked out by the above code, perform the
    recursive halving */
    if(tmp_rank >= 0) {
        size_t *tmp_rcounts = NULL;
        ptrdiff_t *tmp_disps = NULL;
        int mask, send_index, recv_index, last_index;

        /* recalculate disps and rcounts to account for the
        special "remainder" processes that are no longer doing
        anything */
        tmp_rcounts = (size_t*) malloc(tmp_size * sizeof(size_t));
        tmp_disps = (ptrdiff_t*) malloc(tmp_size * sizeof(ptrdiff_t));

        for(i = 0 ; i < tmp_size ; ++i) {
            if(i < remain) {
                /* need to include old neighbor as well */
                tmp_rcounts[i] = rcounts[i * 2 + 1] + rcounts[i * 2];
            } else {
               tmp_rcounts[i] = rcounts[i + remain];
            }
        }

        tmp_disps[0] = 0;
        for(i = 0; i < tmp_size - 1; ++i) {
            tmp_disps[i + 1] = tmp_disps[i] + tmp_rcounts[i];
        }

        /* do the recursive halving communication.  Don't use the
        dimension information on the communicator because I
        think the information is invalidated by our "shrinking"
        of the communicator */
        mask = tmp_size >> 1;
        send_index = recv_index = 0;
        last_index = tmp_size;
        while (mask > 0) {
            int tmp_peer, peer;
            size_t send_count, recv_count;
            MPI_Request request;

            tmp_peer = tmp_rank ^ mask;
            peer = (tmp_peer < remain) ? tmp_peer * 2 + 1 : tmp_peer + remain;

            /* figure out if we're sending, receiving, or both */
            send_count = recv_count = 0;
            if(tmp_rank < tmp_peer) {
                send_index = recv_index + mask;
                for(i = send_index ; i < last_index ; ++i) {
                    send_count += tmp_rcounts[i];
                }
                for(i = recv_index ; i < send_index ; ++i) {
                    recv_count += tmp_rcounts[i];
                }
            } else {
                recv_index = send_index + mask;
                for(i = send_index ; i < recv_index ; ++i) {
                    send_count += tmp_rcounts[i];
                }
                for(i = recv_index ; i < last_index ; ++i) {
                    recv_count += tmp_rcounts[i];
                }
            }

            /* actual data transfer.  Send from result_buf,
            receive into recv_buf */
            if(recv_count > 0) {
                err = MPI_Irecv(recv_buf + tmp_disps[recv_index] * extent,
                recv_count, dtype, peer, 0, comm, &request);
            }
            if(send_count > 0) {
                err = MPI_Send(result_buf + tmp_disps[send_index] * extent,
                            send_count, dtype, peer, 0, comm);
            }

            /* if we received something on this step, push it into
            the results buffer */
            if(recv_count > 0) {
                err = MPI_Wait(&request, MPI_STATUS_IGNORE);

                MPI_Reduce_local(recv_buf + tmp_disps[recv_index] * extent,
                            result_buf + tmp_disps[recv_index] * extent,
                            recv_count, dtype, op);
            }
            /* update for next iteration */
            send_index = recv_index;
            last_index = recv_index + mask;
            mask >>= 1;
        }

        /* copy local results from results buffer into real receive buffer */
        if(0 != rcounts[rank]) {
            err = copy_buffer_different_dt(result_buf + disps[rank] * extent, rcounts[rank],
                                        dtype, rbuf, rcounts[rank], dtype);
        }

        free(tmp_rcounts);
        free(tmp_disps);
    }

    /* Now fix up the non-power of two case, by having the odd
    procs send the even procs the proper results */
    if(rank < (2 * remain)) {
        if((rank & 1) == 0) {
            if(rcounts[rank]) {
                err = MPI_Recv(rbuf, rcounts[rank], dtype, rank + 1, 0, comm, MPI_STATUS_IGNORE);
            }
        } else {
            if(rcounts[rank - 1]) {
                err = MPI_Send(result_buf + disps[rank - 1] * extent,
                rcounts[rank - 1], dtype, rank - 1, 0, comm);
            }
        }
    }
}

int main(int argc, char** argv){

    /*init MPI world*/
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &w_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    /*register signal handler*/
    signal(SIGUSR1,sig_handler); //or SIGUSR1 here

    /*default values*/
    int master_rank=0;
    bool master_rand=false;
    
    int rand_seed=1;
    
    int msg_size=1024;
    int measure_granularity=1;
    max_samples=1000;
    
    warm_up_iters=5;
    int max_iters=1;
    bool endless=false;
    
    double burst_length=0.0;
    bool burst_length_rand=false;
    double burst_pause=0.0;
    bool burst_pause_rand=false;
    
    int i,j,k;

    /*read cmd line args*/
    for(i=1;i<argc;i++){
        if(strcmp(argv[i],"-mrank")==0){
            ++i;
            master_rank=atoi(argv[i]);
        }else if(strcmp(argv[i],"-mrand")==0){
            master_rand=true;
        }else if(strcmp(argv[i],"-msgsize")==0){
            ++i;
            msg_size=atoi(argv[i]);
        }else if(strcmp(argv[i],"-endl")==0){
            endless=true;
        }else if(strcmp(argv[i],"-iter")==0){
            ++i;
            max_iters=atoi(argv[i]);
        }else if(strcmp(argv[i],"-warmup")==0){
            ++i;
            warm_up_iters=atoi(argv[i]);
        }else if(strcmp(argv[i],"-blength")==0){
            ++i;
            burst_length=atof(argv[i]);
        }else if(strcmp(argv[i],"-bpause")==0){
            ++i;
            burst_pause=atof(argv[i]);
        }else if(strcmp(argv[i],"-bprand")==0){
            burst_pause_rand=true;
        }else if(strcmp(argv[i],"-blrand")==0){
            burst_length_rand=true;
        }else if(strcmp(argv[i],"-seed")==0){
            ++i;
            rand_seed=atoi(argv[i]);
        }else if(strcmp(argv[i],"-grty")==0){
            ++i;
            measure_granularity=atoi(argv[i]);
        }else if(strcmp(argv[i],"-maxsamples")==0){
            ++i;
            max_samples=atoi(argv[i]);
        }else{
            if(my_rank==master_rank){
                fprintf(stderr, "Unknown argument: %s\n", argv[i]);
                exit(-1);
            }
        }
    }
    /*set seed such that all ranks share rands*/
    srand(rand_seed);
    
    /*randomized master rank*/
    if(master_rand){
        master_rank=rand()%w_size;
    }
    
    /*pin to core*/
    /*cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);*/
    
    /*allocate buffers*/
    int msg_size_ints;
    int DATA_COUNT;
    int send_buf_size, recv_buf_size;
    int *send_buf;
    int *recv_buf;
    int *recv_counts;
    
    if(msg_size%sizeof(int)!=0){
        if(my_rank==master_rank){
                fprintf(stderr, "Msg-size (%d) must be divisible by size of int (%ld)",msg_size,sizeof(int));
                exit(-1);
        }
    }
    
    send_buf_size=msg_size;
    msg_size_ints=send_buf_size/sizeof(int);
    DATA_COUNT=send_buf_size/sizeof(int);
    recv_buf_size=msg_size/w_size;

    send_buf=(int*)malloc_align(send_buf_size);
    recv_buf=(int*)malloc_align(recv_buf_size);
    durations=(double *)malloc_align(sizeof(double)*max_samples);
    recv_counts =(int*)malloc_align(w_size);
    
    if(send_buf==NULL || recv_buf==NULL || durations==NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }
    
    /*fill send buffer with dummies*/
    for(i=0;i<msg_size_ints;i++){
        send_buf[i]=1;
    }

    for(i=0; i<w_size; i++){
        recv_counts[i] = msg_size_ints/w_size;
    }

    
    /*print basic info to stdout*/
    if(my_rank==master_rank){
        if(endless){
            printf("All-reduce with %d processes, receiver rank: %d, msg-size: %d, test iterations: endless.\n"
                    ,w_size,master_rank,msg_size);
        }else{
            printf("All-reduce with %d processes, receiver rank: %d, msg-size: %d, test iterations: %d.\n"
                    ,w_size,master_rank,msg_size,max_iters);
        }
    }
    
    /*measured iterations*/
    double burst_start_time;
    double measure_start_time;
    double burst_length_mean=burst_length;
    double burst_pause_mean=burst_pause;
    bool burst_cont=false;
    curr_iters=0;
    
    MPI_Barrier(MPI_COMM_WORLD);
    do{
        for(k=0;k<max_iters+warm_up_iters;k++){
            if(burst_length_rand){ /*randomized burst length*/
                burst_length=rand_expo(burst_length_mean);
            }        
            burst_start_time=MPI_Wtime();
            do{
                MPI_Barrier(MPI_COMM_WORLD);
                measure_start_time=MPI_Wtime();
                for(i=0;i<measure_granularity;i++){
                    reduce_scatter_recursivehalving(send_buf, recv_buf, recv_counts, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
                }
                durations[curr_iters%max_samples]=MPI_Wtime()-measure_start_time; /*write result to buffer (lru space)*/
                curr_iters++;
                if(burst_length!=0){ /*bcast needed for synch if bursts timed*/
                    if(my_rank==master_rank){ /*master decides if burst should be continued*/
                        burst_cont=((MPI_Wtime()-burst_start_time)<burst_length);
                    }
                    MPI_Bcast(&burst_cont,1,MPI_INT,master_rank,MPI_COMM_WORLD); /*bcast the masters decision*/
                }
            }while(burst_cont);
            if(burst_pause!=0){
                if(burst_pause_rand){ /*randomized break length*/
                    burst_pause=rand_expo(burst_pause_mean);
                }
                dsleep(burst_pause);
            }
        }
    }while(endless);

    /*write results to file*/
    MPI_Barrier(MPI_COMM_WORLD);
    write_results();
    
    /*free allocated buffers*/
    free(durations);
    free(recv_buf);
    free(send_buf);
    free(recv_counts);
    
    /*exit MPI library*/
    MPI_Finalize();
}

