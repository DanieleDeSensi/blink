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

int main(int argc, char** argv){

    /*init MPI world*/
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &w_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    /*register signal handler*/
    signal(SIGUSR1,sig_handler); //or SIGUSR1 here

    /*parse command line*/
    int i, j, k;
    argc = parse_common_args(argc, argv);
    for (i = 1; i < argc; i++) {
        if (my_rank == master_rank) {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            exit(-1);
        }
    }

    /*pin to core*/
    /*cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);*/
    
    /*allocate buffers*/
    int send_buf_size, recv_buf_size;
    unsigned char *send_buf;
    unsigned char *recv_buf;
    MPI_Win rma_win;
    
    send_buf_size=msg_size;
    recv_buf_size=w_size*msg_size*measure_granularity;
    
    send_buf=(unsigned char*)malloc_align(send_buf_size);
    recv_buf=(unsigned char*)malloc_align(recv_buf_size);
    durations=(double *)malloc_align(sizeof(double)*max_samples);
    
    MPI_Win_create(send_buf,send_buf_size,1,MPI_INFO_NULL,MPI_COMM_WORLD,&rma_win);
    
    if(send_buf==NULL || recv_buf==NULL || durations==NULL || rma_win==NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }
    
    /*fill send buffer with dummies*/
    if(my_rank!=master_rank){
        for(i=0;i<send_buf_size;i++){
            send_buf[i]='a';
        }
    }
    
    /*print basic info to stdout*/
    if(my_rank==master_rank){
        if(endless){
            printf("Incast with %d processes, receiver rank: %d, msg-size: %d, test iterations: endless.\n"
                    ,w_size,master_rank,msg_size);
        }else{
            printf("Incast with %d processes, receiver rank: %d, msg-size: %d, test iterations: %d.\n"
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
                burst_length=sample_burst_length(burst_length_mean);
            }        
            burst_start_time=MPI_Wtime();
            do{
                MPI_Barrier(MPI_COMM_WORLD);
                measure_start_time=MPI_Wtime();
                MPI_Win_fence(0,rma_win);
                if(my_rank==master_rank){
                    for(i=0;i<measure_granularity;i++){    
                        for(j=0;j<w_size;j++){
                                if(j!=master_rank){
                                    MPI_Get(&recv_buf[j*msg_size*measure_granularity+i*msg_size], 
                                        msg_size, MPI_BYTE, j, 0, msg_size, MPI_BYTE, rma_win);
                            }
                        }
                    }
                }
                MPI_Win_fence(0,rma_win);
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
                    burst_pause=sample_pause_length(burst_pause_mean);
                }
                dsleep(burst_pause);
            }
        }
    }while(endless);

    /*write results to file*/
    MPI_Barrier(MPI_COMM_WORLD);
    write_results();
    
    /*free allocated buffers*/
    MPI_Win_free(&rma_win);
    free(durations);
    free(recv_buf);
    free(send_buf);
    
    /*exit MPI library*/
    MPI_Finalize();
}

