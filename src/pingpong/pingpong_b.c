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
    int i, k;
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
    
    send_buf_size=msg_size;
    recv_buf_size=msg_size;
    send_buf=(unsigned char*)malloc_align(send_buf_size);
    recv_buf=(unsigned char*)malloc_align(recv_buf_size);
    durations=(double *)malloc_align(sizeof(double)*max_samples);
    
    if(send_buf==NULL || recv_buf==NULL ||  durations==NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }
    
    if(w_size!=2){
        fprintf(stderr,"Needs two processes to ping-pong. %d\n",my_rank);
        exit(-1);
    }
    
    /*fill send buffer with dummies*/
    for(i=0;i<send_buf_size;i++){
        send_buf[i]='a';
    }
    
    
    /*print basic info to stdout*/
    if(my_rank==master_rank){
        if(endless){
            printf("Ping-pong with %d processes, msg-size: %d, test iterations: endless.\n"
                    ,w_size,msg_size);
        }else{
            printf("Ping-pong with %d processes, msg-size: %d, test iterations: %d.\n"
                    ,w_size,msg_size,max_iters);
        }
    }
    /*measured iterations*/
    double burst_start_time;
    double measure_start_time;
    double burst_length_mean=burst_length;
    double burst_pause_mean=burst_pause;
    bool burst_cont=false;
    int receiver_rank;
    curr_iters=0;
    
    if(master_rank==0){
        receiver_rank=1;
    }else{
        receiver_rank=0;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    do{
        for(k=0;k<max_iters+warm_up_iters;k++){
            if(burst_length_rand){ /*randomized burst length*/
                burst_length=sample_burst_length(burst_length_mean);
            }        
            burst_start_time=MPI_Wtime();
            do{
                if(burst_length){ // If no bursts, no need to do the barrier before (received will be most likely already waiting on the recv)
                    MPI_Barrier(MPI_COMM_WORLD);
                }
                measure_start_time=MPI_Wtime();
                for(i=0;i<measure_granularity;i++){
                    if(my_rank==master_rank){
                        MPI_Send(send_buf,msg_size,MPI_BYTE,receiver_rank,my_rank,MPI_COMM_WORLD);
                        MPI_Recv(recv_buf,msg_size,MPI_BYTE, MPI_ANY_SOURCE
                                    ,MPI_ANY_TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE); // TODO: Any tag is not good!
                    }else{
                        MPI_Recv(recv_buf,msg_size,MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,
                                MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                        MPI_Send(send_buf,msg_size,MPI_BYTE,master_rank,my_rank,MPI_COMM_WORLD);
                    }
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
                    burst_pause=sample_pause_length(burst_pause_mean);
                }
                dsleep(burst_pause);
            }
        }
    }while(endless);
    /*write results to file*/
    MPI_Barrier(MPI_COMM_WORLD);
    

    // For pingpong we can avoid doing allgather etc from common.h (we just need to report rank 0 time)
    if(my_rank==master_rank){
        int num_samples;
        int start_index;
        if (curr_iters - warm_up_iters > max_samples)
        {
            num_samples = max_samples;
            start_index = curr_iters % max_samples;        
        }
        else
        {
            num_samples = curr_iters - warm_up_iters;
            start_index = warm_up_iters;
        }
        printf("Time,Bandwidth\n");
        for(i = 0; i < num_samples; i++){
            float time = durations[(start_index + i) % max_samples]/2;
            float bandwidth = ((msg_size * 8.0) / 1000000000.0) / time;
            printf("%.9f,%.9f\n", time, bandwidth);
        }
        printf("Ran %d iterations. Measured %d iterations.\n", curr_iters, num_samples);
        fflush(stdout);
    }

    
    /*free allocated buffers*/
    free(durations);
    free(send_buf);
    free(recv_buf);
    
    /*exit MPI library*/
    MPI_Finalize();
}

