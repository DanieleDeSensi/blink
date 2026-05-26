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

    char *comm_mode="offpair";
    int target_offset=1;

    /*parse command line*/
    int i, k;
    argc = parse_common_args(argc, argv);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-offset") == 0) {
            target_offset = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-mode") == 0) {
            comm_mode = argv[++i];
        } else {
            if (my_rank == master_rank) {
                fprintf(stderr, "Unknown argument: %s\n", argv[i]);
                exit(-1);
            }
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
    int *targets;
    MPI_Request *recv_requests;
    
    send_buf_size=msg_size;
    recv_buf_size=measure_granularity*msg_size;
    
    send_buf=(unsigned char*)malloc_align(send_buf_size);
    recv_buf=(unsigned char*)malloc_align(recv_buf_size);
    targets=(int*)malloc_align(sizeof(int)*w_size);
    durations=(double *)malloc_align(sizeof(double)*max_samples);
    recv_requests=(MPI_Request*)malloc_align(sizeof(MPI_Request)*measure_granularity);
    
    if(send_buf==NULL || recv_buf==NULL || recv_requests==NULL || targets==NULL || durations==NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }
    
    /*fill send buffer with dummies*/
    for(i=0;i<send_buf_size;i++){
        send_buf[i]='a';
    }
    
    /*setup target mode*/
    if(strcmp(comm_mode,"perm")==0){
        for(i=0;i<w_size;i++){
            targets[i]=i;
        }
        permute(targets,w_size);
    }else if(strcmp(comm_mode,"rot")==0){
        for(i=0;i<w_size;i++){
            targets[i]=mod(i+target_offset,w_size);
        }
    }else if(strcmp(comm_mode,"offpair")==0){
        offset_pairs(targets,w_size,target_offset);
    }else if(strcmp(comm_mode,"rpair")==0){
        random_pairs(targets,w_size);
    }else{
        if(my_rank==master_rank){
            fprintf(stderr,"Unknown communication mode: %s\n",comm_mode);
            exit(-1);
        }
    }
    
    /* //print for target mode debugging
    if(my_rank==master_rank){
        printf("Targets:");
        for(int i=0;i<w_size;i++){
            printf(" %d",targets[i]);
        }
        printf("\n");   
    }*/
    
    /*print basic info to stdout*/
    if(my_rank==master_rank){
        if(endless){
            printf("Pairwise with %d processes, mode: %s, msg-size: %d, test iterations: endless.\n"
                    ,w_size,comm_mode,msg_size);
        }else{
            printf("Pairwise with %d processes, mode: %s, msg-size: %d, test iterations: %d.\n"
                    ,w_size,comm_mode,msg_size,max_iters);
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
                for(i=0;i<measure_granularity;i++){
                    MPI_Irecv(&recv_buf[i*msg_size],recv_buf_size,MPI_BYTE,MPI_ANY_SOURCE
                        ,MPI_ANY_TAG, MPI_COMM_WORLD,&recv_requests[i]);
                    MPI_Send(send_buf,msg_size,MPI_BYTE,targets[my_rank]
                        ,my_rank,MPI_COMM_WORLD);
                }
                MPI_Waitall(measure_granularity,recv_requests,MPI_STATUSES_IGNORE);
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
    free(targets);
    free(durations);
    free(send_buf);
    free(recv_buf);
    free(recv_requests);
    
    /*exit MPI library*/
    MPI_Finalize();
}

