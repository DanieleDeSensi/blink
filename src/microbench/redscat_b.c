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

static inline ptrdiff_t datatype_span(MPI_Datatype dtype, size_t count, ptrdiff_t *gap) {
    if(count == 0) {
      *gap = 0;
      return 0;                                 // No memory span required for zero repetitions
    }
  
    MPI_Aint lb, extent;
    MPI_Aint true_lb, true_extent;
    
    // Get extend and true extent (true extent does not include padding)
    MPI_Type_get_extent(dtype, &lb, &extent);
    MPI_Type_get_true_extent(dtype, &true_lb, &true_extent);
  
    *gap = true_lb;                             // Store the true lower bound
  
    return true_extent + extent * (count - 1);  // Calculate the total memory span
}
  

static inline int copy_buffer(const void *input_buffer, void *output_buffer,
    size_t count, const MPI_Datatype datatype) {

    int datatype_size;
    MPI_Type_size(datatype, &datatype_size);                // Get the size of the MPI datatype

    size_t total_size = count * (size_t)datatype_size;

    memcpy(output_buffer, input_buffer, total_size);        // Perform the memory copy

    return MPI_SUCCESS;
}


int reduce_scatter_ring( const void *sbuf, void *rbuf, const int rcounts[],
    MPI_Datatype dtype, MPI_Op op, MPI_Comm comm)
{
    int ret, line, rank, size, i, k, recv_from, send_to;
    int inbi;
    size_t total_count, max_block_count;
    ptrdiff_t *displs = NULL;
    char *tmpsend = NULL, *tmprecv = NULL, *accumbuf = NULL, *accumbuf_free = NULL;
    char *inbuf_free[2] = {NULL, NULL}, *inbuf[2] = {NULL, NULL};
    ptrdiff_t extent, lb, max_real_segsize, dsize, gap = 0;
    MPI_Request reqs[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};

    ret = MPI_Comm_size(comm, &size);
    ret = MPI_Comm_rank(comm, &rank);

    /* Determine the maximum number of elements per node,
    corresponding block size, and displacements array.
    */
    displs = (ptrdiff_t*) malloc(size * sizeof(ptrdiff_t));
    displs[0] = 0;
    total_count = rcounts[0];
    max_block_count = rcounts[0];
    for(i = 1; i < size; i++) {
        displs[i] = total_count;
        total_count += rcounts[i];
        if(max_block_count < rcounts[i]) max_block_count = rcounts[i];
    }

    /* Special case for size == 1 */
    if(1 == size) {
        if(MPI_IN_PLACE != sbuf) {
            ret = copy_buffer((char*) sbuf, (char*) rbuf,total_count, dtype);
        }
        free(displs);
        return MPI_SUCCESS;
    }

    /* Allocate and initialize temporary buffers, we need:
    - a temporary buffer to perform reduction (size total_count) since
    rbuf can be of rcounts[rank] size.
    - up to two temporary buffers used for communication/computation overlap.
    */
    ret = MPI_Type_get_extent(dtype, &lb, &extent);

    max_real_segsize = datatype_span(dtype, max_block_count, &gap);
    dsize = datatype_span(dtype, total_count, &gap);

    accumbuf_free = (char*)malloc(dsize);
    accumbuf = accumbuf_free - gap;

    inbuf_free[0] = (char*)malloc(max_real_segsize);
    inbuf[0] = inbuf_free[0] - gap;

    if(size > 2) {
        inbuf_free[1] = (char*)malloc(max_real_segsize);
        inbuf[1] = inbuf_free[1] - gap;
    }

    /* Handle MPI_IN_PLACE for size > 1 */
    if(MPI_IN_PLACE == sbuf) {
        sbuf = rbuf;
    }

    ret = copy_buffer((char*) sbuf, accumbuf, total_count, dtype);

    /* Computation loop */

    /*
    For each of the remote nodes:
    - post irecv for block (r-2) from (r-1) with wrap around
    - send block (r-1) to (r+1)
    - in loop for every step k = 2 .. n
    - post irecv for block (r - 1 + n - k) % n
    - wait on block (r + n - k) % n to arrive
    - compute on block (r + n - k ) % n
    - send block (r + n - k) % n
    - wait on block (r)
    - compute on block (r)
    - copy block (r) to rbuf
    Note that we must be careful when computing the beginning of buffers and
    for send operations and computation we must compute the exact block size.
    */
    send_to = (rank + 1) % size;
    recv_from = (rank + size - 1) % size;

    inbi = 0;
    /* Initialize first receive from the neighbor on the left */
    ret = MPI_Irecv(inbuf[inbi], max_block_count, dtype, recv_from, 0, comm, &reqs[inbi]);
    tmpsend = accumbuf + displs[recv_from] * extent;
    ret = MPI_Send(tmpsend, rcounts[recv_from], dtype, send_to, 0, comm);

    for(k = 2; k < size; k++) {
        const int prevblock = (rank + size - k) % size;

        inbi = inbi ^ 0x1;

        /* Post irecv for the current block */
        ret = MPI_Irecv(inbuf[inbi], max_block_count, dtype, recv_from, 0, comm, &reqs[inbi]);

        /* Wait on previous block to arrive */
        ret = MPI_Wait(&reqs[inbi ^ 0x1], MPI_STATUS_IGNORE);

        /* Apply operation on previous block: result goes to rbuf
        rbuf[prevblock] = inbuf[inbi ^ 0x1] (op) rbuf[prevblock]
        */
        tmprecv = accumbuf + displs[prevblock] * extent;
        MPI_Reduce_local(inbuf[inbi ^ 0x1], tmprecv, rcounts[prevblock], dtype, op);

        /* send previous block to send_to */
        ret = MPI_Send(tmprecv, rcounts[prevblock], dtype, send_to, 0, comm);
    }

    /* Wait on the last block to arrive */
    ret = MPI_Wait(&reqs[inbi], MPI_STATUS_IGNORE);

    /* Apply operation on the last block (my block)
    rbuf[rank] = inbuf[inbi] (op) rbuf[rank] */
    tmprecv = accumbuf + displs[rank] * extent;
    MPI_Reduce_local(inbuf[inbi], tmprecv, rcounts[rank], dtype, op);

    /* Copy result from tmprecv to rbuf */
    ret = copy_buffer(tmprecv, (char *)rbuf, rcounts[rank], dtype);

    if(NULL != displs) free(displs);
    if(NULL != accumbuf_free) free(accumbuf_free);
    if(NULL != inbuf_free[0]) free(inbuf_free[0]);
    if(NULL != inbuf_free[1]) free(inbuf_free[1]);

    return MPI_SUCCESS;
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
    recv_buf_size=msg_size/w_size;
    msg_size_ints=msg_size/sizeof(int);

    send_buf = (int*) malloc_align(send_buf_size);
    recv_buf = (int*) malloc_align(recv_buf_size);
    recv_counts = (int*) malloc_align(w_size*sizeof(int));
    durations = (double*) malloc_align(sizeof(double)*max_samples);
    if(send_buf==NULL || recv_buf==NULL || durations==NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }
    
    /*fill send buffer with dummies*/
    for(i=0;i<msg_size_ints;i++){
        send_buf[i]= (int) (rand()*my_rank % 10);
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
                    MPI_Reduce_scatter(send_buf, recv_buf, recv_counts, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
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

