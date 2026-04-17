#include <mpi.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <time.h>
#include <cstdlib> 
#include <cmath>  
#include "common.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const size_t BUFFER_SIZE = 2 * 1024 * 1024;  // bytes per peer 2MiB

    // Each process will send a chunk to every other process
    unsigned char *buffer = (unsigned char*) malloc_align(BUFFER_SIZE); 
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return -1;
    }

    srand(time(NULL)*rank); 
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = rand()*rank % size; 
    }

    double burst_pause;
    double burst_length;
    if(argc >= 2){
        burst_pause = atof(argv[1]);
    }else{
        std::cerr << "Not enough arguments. Usage: ./bursty_noise_a2a <burst_pause> <burst_length>" << std::endl;
        return 1;
    }
    if(argc >= 3){
        burst_length = atof(argv[2]);
    } else {
        std::cerr << "Not enough arguments. Usage: ./bursty_noise_a2a <burst_pause> <burst_length>" << std::endl;
        return 1;
    }


    bool burst_pause_rand = false;
    double burst_start_time;
    double measure_start_time;
    double burst_length_mean=burst_length;
    double burst_pause_mean=burst_pause;
    int burst_cont=0;

    while (1) {
        std::vector<MPI_Request> requests;
        burst_start_time=MPI_Wtime();
        do {
            // INCAST START
            requests.clear();

            if (rank == 0) {

                for (int sender = 1; sender < size; ++sender) {
                    MPI_Request req;
                    MPI_Irecv(buffer, BUFFER_SIZE, MPI_BYTE, sender, 0, MPI_COMM_WORLD, &req);
                    requests.push_back(req);
                }

            } else {
                MPI_Request req;
                MPI_Isend(buffer, BUFFER_SIZE, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &req);
                requests.push_back(req);
            }

            MPI_Waitall(requests.size(), requests.data(), MPI_STATUSES_IGNORE);
            // INCAST END

            if(burst_length!=0){ /*bcast needed for synch if bursts timed*/
                if(rank == 0){ /*master decides if burst should be continued*/
                    burst_cont=((MPI_Wtime()-burst_start_time)<burst_length);
                }
                MPI_Bcast(&burst_cont,1, MPI_INT, 0, MPI_COMM_WORLD); /*bcast the masters decision*/
            }
        } while (burst_cont);

        if(burst_pause!=0){
            dsleep(burst_pause);
        }
    }


    MPI_Finalize();
    return 0;
}