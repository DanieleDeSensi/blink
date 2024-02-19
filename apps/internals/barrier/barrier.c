#define _GNU_SOURCE
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
static double rand_expo(double mean){
    double lambda=1.0/mean;
    double u=rand()/(RAND_MAX+1.0);
    return -log(1-u)/lambda;
}

/*sleep seconds given as double*/
static int dsleep(double t){
    struct timespec t1, t2;
    t1.tv_sec=(long)t;
    t1.tv_nsec=(t-t1.tv_sec)*1000000000L;
    return nanosleep(&t1,&t2);
}

/*double comparison function for quicksort*/
int compare_doubles(const void *p1, const void *p2){
    if(*(double*)p1<*(double*)p2) return -1;
    else if(*(double*)p1>*(double*)p2) return 1;
    else return 0;
}

/*global variables because of signal handling*/
int my_rank;
int w_size;
int master_rank;
int curr_iters;
int warm_up_iters;
int max_samples;
double *durations;
double *results;

static void write_results(){
    double duration_sum;
    double duration_median;
    int num_samples; 
    int i,j;
    int start_index;
    
    if(curr_iters-warm_up_iters>max_samples){
        num_samples=max_samples;
        start_index=curr_iters%max_samples;
    }else{
        num_samples=curr_iters-warm_up_iters;
        start_index=warm_up_iters;
    }
    /*print file header*/
    if(my_rank==master_rank){
        printf("Average,Minimum,Maximum,Median\n");
    }
    
    /*gather+sort to get avg,min,max,median for all every saved iteration*/
    for(i=start_index;i<start_index+num_samples;i++){
        MPI_Gather(&durations[i%max_samples],1,MPI_DOUBLE,results,1,MPI_DOUBLE,master_rank,MPI_COMM_WORLD);
        if(my_rank==master_rank){
            qsort(results,w_size,sizeof(double),compare_doubles);
            duration_sum=0;
            for(j=0;j<w_size;j++){
                duration_sum+=results[j];
            }
            if(w_size%2==0){ /*even: then median as mean of middle values*/
                duration_median=(results[(w_size-1)/2]+results[w_size/2])/2;
            }else{ /*odd: else median as middle value*/
                duration_median=results[(w_size-1)/2];
            }
            printf("%.9f,%.9f,%.9f,%.9f\n"
                ,duration_sum/w_size,results[0],results[w_size-1],duration_median);
        }
    }
    
    if(my_rank==master_rank){
        printf("Ran %d iterations. Measured %d iterations.\n",curr_iters,num_samples);
        fflush(stdout);
    }
}

/*signal handler*/
void sig_handler(int sig){
    write_results();
    MPI_Finalize();
    exit(0);
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
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    
    /*allocate buffers*/
    durations=(double *)malloc(sizeof(double)*max_samples);
    results=(double *)malloc(sizeof(double)*w_size);
    
    if(durations==NULL || results==NULL){
        fprintf(stderr,"Failed to allocate a buffer on rank %d\n",my_rank);
        exit(-1);
    }

    /*print basic info to stdout*/
    if(my_rank==master_rank){
        if(endless){
            printf("Barrier with %d processes, test iterations: endless.\n"
                    ,w_size);
        }else{
            printf("Barrier with %d processes, test iterations: %d.\n"
                    ,w_size,max_iters);
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
                    MPI_Barrier(MPI_COMM_WORLD);
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
    free(results);
    
    /*exit MPI library*/
    MPI_Finalize();
}

