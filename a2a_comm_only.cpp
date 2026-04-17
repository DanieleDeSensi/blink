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
#include <vector>
#include "common.h"



void all2all_memcpy(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm){

    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    
    int datatype_size;
    MPI_Type_size(sendtype, &datatype_size);

    const char* sbuf = static_cast<const char*>(sendbuf);
    char* rbuf = static_cast<char*>(recvbuf);

    double mem_time = MPI_Wtime(); 
    // Copy local data directly (self-send)
    memcpy(rbuf + rank * datatype_size * recvcount,
                sbuf + rank * datatype_size * sendcount,
                sendcount * datatype_size);

}

void custom_alltoall(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                     void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int datatype_size;
    MPI_Type_size(sendtype, &datatype_size);

    const char* sbuf = static_cast<const char*>(sendbuf);
    char* rbuf = static_cast<char*>(recvbuf);

    std::vector<MPI_Request> requests;
    for (int i = 0; i < size; ++i) {
        if (i == rank) continue;

        MPI_Request req_recv;
        MPI_Request req_send;

        MPI_Isend(sbuf + i * datatype_size * sendcount, sendcount, sendtype, i, 0, comm, &req_send);
        MPI_Irecv(rbuf + i * datatype_size * recvcount, recvcount, recvtype, i, 0, comm, &req_recv);
        
        requests.push_back(req_send);
        requests.push_back(req_recv);
    }

    MPI_Waitall(static_cast<int>(requests.size()), requests.data(), MPI_STATUSES_IGNORE);
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
    
    int i,k;

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
    size_t send_buf_size, recv_buf_size;
    unsigned char *send_buf;
    unsigned char *recv_buf;

    send_buf_size=msg_size*w_size;
    recv_buf_size=msg_size*w_size;
    
    send_buf=(unsigned char*)malloc_align(send_buf_size);
    recv_buf=(unsigned char*)malloc_align(recv_buf_size);
    durations=(double *)malloc_align(sizeof(double)*max_samples);
    
    if(send_buf==NULL){
        fprintf(stderr,"Failed to allocate send_buf on rank %d\n",my_rank);
        exit(-1);
    } else if (recv_buf==NULL){
        fprintf(stderr,"Failed to allocate recv_buf on rank %d\n",my_rank);
        exit(-1);
    } else if (durations==NULL){
        fprintf(stderr,"Failed to allocate durations buffer on rank %d\n",my_rank);
        exit(-1);
    }
    
    /*fill send buffer with dummies*/
    for(i=0;i<send_buf_size;i++){
        send_buf[i]='a';
    }
    
    /*print basic info to stdout*/
    if(my_rank==master_rank){
        if(endless){
            printf("All-to-all with %d processes, msg-size: %zu, test iterations: endless.\n"
                    ,w_size,msg_size);
        }else{
            printf("All-to-all with %d processes, msg-size: %zu, test iterations: %d.\n"
                    ,w_size,msg_size,max_iters);
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

    size_t large_count = 0;
    if(msg_size >= 8 && msg_size % 8 == 0){ // Check if I can use 64-bit data types
        large_count = msg_size / 8;
        if (large_count >= ((u_int64_t) (1UL << 32)) - 1) { // If large_count can't be represented on 32 bits
            if(my_rank == 0){
                printf("\tTransfer size (B): -1, Transfer Time (s): -1, Bandwidth (GB/s): -1, Iteration -1\n");
            }
            return -1;
        }
    }else{
        if (msg_size >= ((u_int64_t) (1UL << 32)) - 1) { // If msg_size can't be represented on 32 bits
            if(my_rank == 0){
                printf("\tTransfer size (B): -1, Transfer Time (s): -1, Bandwidth (GB/s): -1, Iteration -1\n");
            }
            return -1;
        }
    }

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
                    if(large_count){
                        all2all_memcpy(send_buf, large_count, MPI_UINT64_T, recv_buf, large_count, MPI_UINT64_T, MPI_COMM_WORLD);
                        measure_start_time=MPI_Wtime();
                        custom_alltoall(send_buf, large_count, MPI_UINT64_T, recv_buf, large_count, MPI_UINT64_T, MPI_COMM_WORLD);
                        measure_total_time+=MPI_Wtime()-measure_start_time;
                    }else{
                        all2all_memcpy(send_buf, msg_size, MPI_BYTE, recv_buf, msg_size, MPI_BYTE, MPI_COMM_WORLD);
                        measure_start_time=MPI_Wtime();
                        custom_alltoall(send_buf, msg_size, MPI_BYTE, recv_buf, msg_size, MPI_BYTE, MPI_COMM_WORLD);
                        measure_total_time+=MPI_Wtime()-measure_start_time;
                    }
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
    free(send_buf);
    free(recv_buf);
    
    /*exit MPI library*/
    MPI_Finalize();
}

