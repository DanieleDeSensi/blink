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
  if (input_buffer == NULL || output_buffer == NULL || scount <= 0 || rcount <= 0) {
    return MPI_ERR_UNKNOWN;
  }

  int sdtype_size;
  MPI_Type_size(sdtype, &sdtype_size);
  int rdtype_size;
  MPI_Type_size(rdtype, &rdtype_size);

  size_t s_size = (size_t) sdtype_size * scount;
  size_t r_size = (size_t) rdtype_size * rcount;

  if (r_size < s_size) {
    memcpy(output_buffer, input_buffer, r_size); // Copy as much as possible
    return MPI_ERR_TRUNCATE;      // Indicate truncation
  }

  memcpy(output_buffer, input_buffer, s_size);        // Perform the memory copy

  return MPI_SUCCESS;
}


void allgather_memcpy(const void *sbuf, size_t scount, MPI_Datatype sdtype, void* rbuf, size_t rcount, MPI_Datatype rdtype, MPI_Comm comm){

  int rank, size, sendto, recvfrom, i, recvdatafrom, senddatafrom;
  MPI_Aint rlb, rext;
  char *tmpsend = NULL, *tmprecv = NULL;

  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);

  MPI_Type_get_extent(rdtype, &rlb, &rext);

  tmprecv = (char*) rbuf + rank * rcount * rext;
  if (MPI_IN_PLACE != sbuf) {
    tmpsend = (char*) sbuf;
    copy_buffer_different_dt(tmpsend, scount, sdtype, tmprecv, rcount, rdtype);
  }
}


void allgather_ring(const void *sbuf, size_t scount, MPI_Datatype sdtype,
                   void* rbuf, size_t rcount, MPI_Datatype rdtype, MPI_Comm comm) {

  int rank, size, sendto, recvfrom, i, recvdatafrom, senddatafrom;
  MPI_Aint rlb, rext;
  char *tmpsend = NULL, *tmprecv = NULL;

  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);

  MPI_Type_get_extent(rdtype, &rlb, &rext);

  sendto = (rank + 1) % size;
  recvfrom  = (rank - 1 + size) % size;

  for (i = 0; i < size - 1; i++) {

    recvdatafrom = (rank - i - 1 + size) % size;
    senddatafrom = (rank - i + size) % size;

    tmprecv = (char*)rbuf + recvdatafrom * rcount * rext;
    tmpsend = (char*)rbuf + senddatafrom * rcount * rext;

    MPI_Sendrecv(tmpsend, rcount, rdtype, sendto, 0,
                       tmprecv, rcount, rdtype, recvfrom, 0,
                       comm, MPI_STATUS_IGNORE);

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
    
    size_t msg_size=1024;
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
    size_t msg_size_ints;
    size_t send_buf_size, recv_buf_size;
    int *send_buf;
    int *recv_buf;
    
    if(msg_size%sizeof(int)!=0){
        if(my_rank==master_rank){
                fprintf(stderr, "Msg-size (%zu) must be divisible by size of int (%ld)",msg_size,sizeof(int));
                exit(-1);
        }
    }
    
    send_buf_size=msg_size/w_size;
    msg_size_ints=send_buf_size/sizeof(int);
    recv_buf_size=msg_size;
    
    send_buf=(int*)malloc_align(send_buf_size);
    recv_buf=(int*)malloc_align(recv_buf_size);
    durations=(double *)malloc_align(sizeof(double)*max_samples);
    
    if(send_buf==NULL || recv_buf==NULL || durations==NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }
    
    /*fill send buffer with dummies*/
    for(i=0;i<msg_size_ints;i++){
        send_buf[i]=1;
    }

    
    /*print basic info to stdout*/
    if(my_rank==master_rank){
        if(endless){
            printf("All-reduce with %d processes, receiver rank: %d, msg-size: %zu, test iterations: endless.\n"
                    ,w_size,master_rank,msg_size);
        }else{
            printf("All-reduce with %d processes, receiver rank: %d, msg-size: %zu, test iterations: %d.\n"
                    ,w_size,master_rank,msg_size,max_iters);
        }
    }
    
    /*measured iterations*/
    double burst_start_time;
    double measure_start_time;
    double measure_total_time;
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
                measure_total_time=0.0;
                for(i=0;i<measure_granularity;i++){
                    allgather_memcpy(send_buf, msg_size_ints, MPI_INT, recv_buf, msg_size_ints, MPI_INT, MPI_COMM_WORLD);
                    measure_start_time=MPI_Wtime();
                    allgather_ring(send_buf, msg_size_ints, MPI_INT, recv_buf, msg_size_ints, MPI_INT, MPI_COMM_WORLD);
                    measure_total_time+=MPI_Wtime()-measure_start_time;
                }
                durations[curr_iters%max_samples]=measure_total_time; /*write result to buffer (lru space)*/
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
    
    /*exit MPI library*/
    MPI_Finalize();
}

