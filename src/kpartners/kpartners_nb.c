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
    signal(SIGUSR1,sig_handler);

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

    int npartners=1; /*number of random communication partners per rank (-k flag)*/

    int i,k,p;

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
        }else if(strcmp(argv[i],"-k")==0){
            ++i;
            npartners=atoi(argv[i]);
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

    /*validate npartners*/
    if(npartners<1){
        if(my_rank==master_rank){
            fprintf(stderr,"k (%d) must be >= 1\n",npartners);
            exit(-1);
        }
    }
    if(npartners>w_size-1){
        if(my_rank==master_rank){
            fprintf(stderr,"Warning: k (%d) capped to w_size-1 (%d)\n",npartners,w_size-1);
        }
        npartners=w_size-1;
    }

    /*
     * Build npartners random permutations of [0, w_size-1].
     * For permutation p, rank i sends to perms[p*w_size + i].
     * Each rank appears as a target exactly once per permutation,
     * so every rank sends npartners messages and receives npartners messages.
     * Partners are fixed for the entire run (generated from rand_seed).
     */
    int *perms;
    perms=(int*)malloc_align(sizeof(int)*npartners*w_size);
    if(perms==NULL){
        fprintf(stderr,"Failed to allocate permutation table on rank %d\n",my_rank);
        exit(-1);
    }
    for(p=0;p<npartners;p++){
        for(i=0;i<w_size;i++){
            perms[p*w_size+i]=i;
        }
        permute(&perms[p*w_size],w_size);
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
    MPI_Request *send_requests;
    MPI_Request *recv_requests;

    send_buf_size=msg_size;
    recv_buf_size=npartners*measure_granularity*msg_size;

    send_buf=(unsigned char*)malloc_align(send_buf_size);
    recv_buf=(unsigned char*)malloc_align(recv_buf_size);
    durations=(double *)malloc_align(sizeof(double)*max_samples);
    send_requests=(MPI_Request*)malloc_align(sizeof(MPI_Request)*npartners*measure_granularity);
    recv_requests=(MPI_Request*)malloc_align(sizeof(MPI_Request)*npartners*measure_granularity);

    if(send_buf==NULL || recv_buf==NULL || durations==NULL || send_requests==NULL || recv_requests==NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }

    /*fill send buffer with dummies*/
    for(i=0;i<send_buf_size;i++){
        send_buf[i]='a';
    }

    /*print basic info to stdout*/
    if(my_rank==master_rank){
        if(endless){
            printf("k-random-partners with %d processes, k=%d, msg-size: %d, test iterations: endless.\n"
                    ,w_size,npartners,msg_size);
        }else{
            printf("k-random-partners with %d processes, k=%d, msg-size: %d, test iterations: %d.\n"
                    ,w_size,npartners,msg_size,max_iters);
        }
    }

    /*measured iterations*/
    double burst_start_time;
    double measure_start_time;
    double burst_length_mean=burst_length;
    double burst_pause_mean=burst_pause;
    bool burst_cont=false;
    curr_iters=0;

    int antideadlock_tag=0;

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
                    /*post npartners receives (MPI_ANY_SOURCE: each perm maps exactly one sender to this rank)*/
                    /*post npartners sends to this rank's target in each permutation*/
                    for(p=0;p<npartners;p++){
                        MPI_Irecv(&recv_buf[(i*npartners+p)*msg_size],msg_size,MPI_BYTE,
                                  MPI_ANY_SOURCE,antideadlock_tag,MPI_COMM_WORLD,
                                  &recv_requests[i*npartners+p]);
                        MPI_Isend(send_buf,msg_size,MPI_BYTE,
                                  perms[p*w_size+my_rank],antideadlock_tag,MPI_COMM_WORLD,
                                  &send_requests[i*npartners+p]);
                    }
                    antideadlock_tag++;
                }
                MPI_Waitall(npartners*measure_granularity,send_requests,MPI_STATUSES_IGNORE);
                MPI_Waitall(npartners*measure_granularity,recv_requests,MPI_STATUSES_IGNORE);
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
    free(perms);
    free(durations);
    free(send_buf);
    free(recv_buf);
    free(send_requests);
    free(recv_requests);

    /*exit MPI library*/
    MPI_Finalize();
}
