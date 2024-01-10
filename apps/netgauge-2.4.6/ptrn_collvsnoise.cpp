/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_COLLVSNOISE && defined NG_MPI
#include "hrtimer/hrtimer.h"
#include "mersenne/MersenneTwister.h"
#include "ptrn_collvsnoise_cmdline.h"
#include <stdint.h>
#include <vector>
#include <numeric>
#include <algorithm>

extern "C" {
#include "ng_sync.h"

extern struct ng_options g_options;


/* internal function prototypes */
static void collvsnoise_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_collvsnoise = {
   pattern_collvsnoise.name = "collvsnoise",
   pattern_collvsnoise.desc = "assesses the influence of network jitter on collective MPI operations",
   pattern_collvsnoise.flags = 0,
   pattern_collvsnoise.do_benchmarks = collvsnoise_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_collvsnoise() {
   ng_register_pattern(&pattern_collvsnoise);
   return 0;
}

static void collvsnoise_do_benchmarks(struct ng_module *module) {

  int comm_rank;
  int comm_size;

	//parse cmdline arguments
	struct ptrn_collvsnoise_cmd_struct args_info;
	//printf("The string I got: %s\n", g_options.ptrnopts);
	if (ptrn_collvsnoise_parser_string(g_options.ptrnopts, &args_info, "netgauge") != 0) {
		exit(EXIT_FAILURE);
	}

  int change_comm = !args_info.nochangecomm_given; /* create new comm for every try or not */
  unsigned long minsize, maxsize;
  //printf("min: %li, max: %li", minsize, maxsize);
  ng_readminmax(args_info.datasize_arg, &minsize, &maxsize);
  unsigned long commminsize, commmaxsize;
  ng_readminmax(args_info.commsize_arg, &commminsize, &commmaxsize);


  static const int size=1*sizeof(double); // size of the "data" that will be transmitted in the benchmarks - should be divisible by sizeof(double)
  static const long pertbufsize=1024*1024*10; /* size of the perturbation messages 10 MiB */

  /* select collective to benchmark */
//#define COLLCALL(comm) MPI_Alltoall(sendbuf, size, MPI_BYTE, recvbuf, size, MPI_BYTE, comm);
#define COLLCALL(comm) MPI_Allreduce(sendbuf, recvbuf, size/sizeof(double), MPI_DOUBLE, MPI_SUM, comm);
//#define COLLCALL(comm) MPI_Reduce(sendbuf, recvbuf, size/sizeof(double), MPI_DOUBLE, MPI_SUM, 0, comm);
//#define COLLCALL(comm) MPI_Bcast(sendbuf, size/sizeof(double), MPI_DOUBLE, 0, comm);
//#define COLLCALL(comm) {NBC_Handle h; NBC_Iallreduce(sendbuf, recvbuf, size/sizeof(double), MPI_DOUBLE, MPI_SUM, comm, &h); NBC_Wait(&h);}

  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  MPI_Comm mpi_comm_coll;
  MPI_Comm mpi_comm_perturb;

  int rank_in_comm_coll, rank_in_comm_perturb;
  int comm_perturb_size, comm_coll_size;
  char *sendbuf = (char *) malloc(size * comm_size);
  char *recvbuf = (char *) malloc(size * comm_size);
  char *spertbuf = (char*)malloc(pertbufsize); 
  char *rpertbuf = (char*)malloc(pertbufsize); 
  if (!sendbuf || !recvbuf || !rpertbuf || !spertbuf) ng_abort("Couldn't allocate buffers!\n"); 
  for(int i=0; i<size * comm_size; i++) sendbuf[i] = recvbuf[i] = 'F';
  for(int i=0; i<pertbufsize; i++) spertbuf[i] = rpertbuf[i] = 'F';
  
  // warmup buffers and COMM_WORLD
  //MPI_Alltoall(sendbuf, size, MPI_INT, recvbuf, size, MPI_INT, MPI_COMM_WORLD);
  
  MPI_Request sreq, rreq[2];

  HRT_TIMESTAMP_T t1, t2;
  unsigned long pertrounds=1; /* how many perturbation messages have to be sent - this is adapted during the benchmark */

  int rand_mapping[comm_size];  // mapping to decide if a rank goes into the collective group or the perturbation group

  for (int ratio=commminsize; ratio <= std::min(comm_size,(int)commmaxsize); ratio+=2) {
    std::vector<double> colltime[2]; // usecs coll comm without and with perturbation
    std::vector<double> perttime[2]; // usecs coll comm without and with perturbation
    for (long iter=0; (iter < g_options.testcount) && !g_stop_tests; iter++) {
	    if ( comm_rank == 0  && (NG_VLEV1 > g_options.verbose) && (g_options.testcount < NG_DOT_COUNT || !(iter % (int)(g_options.testcount / NG_DOT_COUNT)) )) {
	      printf(".");
	      fflush(stdout);
	    }

      if(change_comm || iter==0) {
        // split all ranks in two communicators:
        // - one for the collective operation
        // - one for the noise generation
        
        // Rank 0 will figure out who belongs to which group so that it is
        // consistent among all ranks
        if (comm_rank == 0) {
          // Create an array with comm_size elements, with the numbers
          // 0 .. comm_size-1 elements in random order
          
          //initialize rand_mapping[i] with -1 for "unassigned"
          for (int i = 0; i < comm_size; i++) rand_mapping[i] = -1;

          for (int i = 0; i < comm_size; i++) {
            // get a random number m in [0 .. free_spaces]
            MTRand mtrand;
            int m = mtrand.randInt(comm_size-i-1);
            int ok=0;
            // assign i to the m'th free space
            int free_counter = -1;
            for (int j=0; j < comm_size; j++) {
              if (rand_mapping[j]<0) {
                free_counter++;
                if (free_counter == m) {rand_mapping[j] = i; ok=1;}
              }
            }
            if (ok == 0) printf("something is broken in the mapping function...\n");
          }	
        
          if((g_options.verbose >= NG_VLEV2)) {
            // print the mapping array
            printf("# collective comm: [ ");
            for (int i=0; i<ratio; i++) {
              printf("%i ", rand_mapping[i]);		
            }
            printf("]\n");
            // print the mapping array
            printf("# perturbation comm: [ ");
            for (int i=ratio; i<comm_size; i++) {
              printf("%i ", rand_mapping[i]);		
            }
            printf("]\n");
          }
        }

        // Broadcast the mapping
        MPI_Bcast(rand_mapping, comm_size, MPI_INT, 0, MPI_COMM_WORLD);

        // split MPI_COMM_WORLD, the first ranks (< ratio) go into mpi_comm_coll
        // all others go into mpi_comm_perturb
        for (int i=0; i<comm_size; i++) {
          if (rand_mapping[i] == comm_rank) {
            if (i<ratio) {
              //printf("#  Rank %i will go into comm_coll.\n", comm_rank);
              MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &mpi_comm_coll);
              MPI_Comm_rank(mpi_comm_coll, &rank_in_comm_coll);
              rank_in_comm_perturb=-1;
              MPI_Comm_size(mpi_comm_coll, &comm_coll_size);
              comm_perturb_size=-1;
              // establish all connections + warmup collective 
              //for(int t=0; t<2;t++) MPI_Alltoall(sendbuf, size, MPI_BYTE, recvbuf, size, MPI_BYTE, mpi_comm_coll);
              //for(int t=0; t<2;t++) MPI_Allreduce(sendbuf, recvbuf, size/sizeof(double), MPI_DOUBLE, MPI_SUM, mpi_comm_coll);
              for(int t=0; t<2;t++) COLLCALL(mpi_comm_coll);
            } else {
              //printf("#  +++ Rank %i will go into comm_perturb.\n", comm_rank);
              MPI_Comm_split(MPI_COMM_WORLD, 1, 0, &mpi_comm_perturb);
              MPI_Comm_rank(mpi_comm_perturb, &rank_in_comm_perturb);
              rank_in_comm_coll=-1;
              MPI_Comm_size(mpi_comm_perturb, &comm_perturb_size);
              comm_coll_size=-1;
              // establish all connections + warmup collective 
              //for(int t=0; t<2;t++) MPI_Alltoall(sendbuf, size, MPI_BYTE, recvbuf, size, MPI_BYTE, mpi_comm_perturb);
              //for(int t=0; t<2;t++) MPI_Allreduce(sendbuf, recvbuf, size/sizeof(double), MPI_DOUBLE, MPI_SUM, mpi_comm_perturb);
              for(int t=0; t<2;t++) COLLCALL(mpi_comm_perturb);
            }
          }
        }
      }

      for (int do_perturb=1; do_perturb>=0; do_perturb--) {
      
        // synchronize all ranks
        MPI_Barrier(MPI_COMM_WORLD); // this is just to be extra-sure
        /*ng_sync_init_stage1(MPI_COMM_WORLD);
        ng_sync_init_stage2(MPI_COMM_WORLD, &win);
        long err = ng_sync(win);*/

        if (rank_in_comm_perturb >= 0) {
          HRT_GET_TIMESTAMP(t1);
         
          if (do_perturb) {
            for (int k = 0; k < pertrounds; k++) { 
              int peer;
              if (rank_in_comm_perturb%2 == 0) {
                peer = rank_in_comm_perturb+1;
              } else {
                peer = rank_in_comm_perturb-1;
              }
              //MPI_Sendrecv_replace(pertbuf, pertbufsize, MPI_BYTE, peer, 0, peer, 0, mpi_comm_perturb, MPI_STATUS_IGNORE);
              MPI_Request pertreqs[2];
              MPI_Isend(spertbuf, pertbufsize, MPI_BYTE, peer, 0, mpi_comm_perturb,&pertreqs[0]);
              MPI_Irecv(rpertbuf, pertbufsize, MPI_BYTE, peer, 0, mpi_comm_perturb,&pertreqs[1]);
              MPI_Waitall(2, pertreqs, MPI_STATUSES_IGNORE);
            }
          }

          HRT_GET_TIMESTAMP(t2);
          uint64_t num_ticks;
          HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks);
          double num_ticks_d = (double) num_ticks;
          double num_ticks_r;
          MPI_Reduce(&num_ticks_d, &num_ticks_r, 1, MPI_DOUBLE, MPI_SUM, 0, mpi_comm_perturb);
          double us = 1e6 * num_ticks_r / (double) g_timerfreq/comm_perturb_size;
          if (rank_in_comm_perturb == 0) {
            //printf("%i of %i processes are doing the collective operation\n", ratio, comm_size);
            MPI_Isend(&us, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &sreq);
            //printf("Perturb: %f\n", us);
          }
        }

        if (rank_in_comm_coll >= 0) {
          // the size as well as the collective should be configureable later
          //printf("Rank %i does Collective\n", comm_rank);
          //MPI_Alltoall(sendbuf, size, MPI_BYTE, recvbuf, size, MPI_BYTE, mpi_comm_coll);
          COLLCALL(mpi_comm_coll);

          HRT_GET_TIMESTAMP(t1);
          
          // The collective we want to meassure...
          //MPI_Barrier(mpi_comm_coll);
          //for(int num=0; num<10;num++) MPI_Alltoall(sendbuf, size, MPI_BYTE, recvbuf, size, MPI_BYTE, mpi_comm_coll);
          //MPI_Allreduce(sendbuf, recvbuf, size/sizeof(double), MPI_DOUBLE, MPI_SUM, mpi_comm_coll);
          for(int num=0; num<10;num++) COLLCALL(mpi_comm_coll);
  
          HRT_GET_TIMESTAMP(t2);
          uint64_t num_ticks;
          HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks);
          double num_ticks_d = (double) num_ticks;
          double num_ticks_r;
          MPI_Reduce(&num_ticks_d, &num_ticks_r, 1, MPI_DOUBLE, MPI_SUM, 0, mpi_comm_coll);
          double us = 1e6 * num_ticks_r / (double) g_timerfreq/comm_coll_size;
          int comm_coll_rank;
          MPI_Comm_rank(mpi_comm_coll, &comm_coll_rank);
          if (comm_coll_rank == 0) {
            //printf("Coll: %f\n", us);
            MPI_Isend(&us, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &sreq);
          }
        }

        if (comm_rank == 0) {
          double us_c, us_p;
 
          MPI_Irecv(&us_c, 1, MPI_DOUBLE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &rreq[0]);
          if(comm_size > ratio) {
            MPI_Irecv(&us_p, 1, MPI_DOUBLE, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &rreq[1]);
            MPI_Waitall(2, rreq, MPI_STATUSES_IGNORE);
            perttime[do_perturb].push_back(us_p);
          } else {
            MPI_Waitall(1, rreq, MPI_STATUSES_IGNORE);
            perttime[do_perturb].push_back(0.0);
          }
          colltime[do_perturb].push_back(us_c);
        }
        if((rank_in_comm_coll == 0) || (rank_in_comm_perturb == 0)) MPI_Wait(&sreq, MPI_STATUS_IGNORE);
      } // end of loop do_perturb
      if(!comm_rank && (g_options.verbose >= NG_VLEV1)) printf("%i %i %li %i :: %lf (%lf) %lf (%lf) \n", comm_size, ratio, iter, size, colltime[0][iter], perttime[0][iter], colltime[1][iter], perttime[1][iter]);

      // check if we still have enough perturbation time
      if(!comm_rank && (perttime[1][iter] < 2*colltime[1][iter])) {
        pertrounds*=2;
      }
      MPI_Bcast(&pertrounds, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

      if(change_comm || iter == (g_options.testcount-1)) {
        /* free the communicators */
        if (rank_in_comm_perturb >= 0) MPI_Comm_free(&mpi_comm_perturb);
        if (rank_in_comm_coll >= 0) MPI_Comm_free(&mpi_comm_coll);
      }
    } // end of loop over tries
    double sumnopert = std::accumulate(colltime[0].begin(), colltime[0].end(), 0.0);
    double sumpert = std::accumulate(colltime[1].begin(), colltime[1].end(), 0.0);
    std::sort(colltime[0].begin(), colltime[0].end());
    std::sort(colltime[1].begin(), colltime[1].end());
    if(!comm_rank && (g_options.verbose < NG_VLEV1)) printf("\n");
    if(!comm_rank) printf("# acc: %i %i %i :: %lf (%lf), %lf (%lf)\n", comm_size, ratio, size, sumnopert/g_options.testcount, colltime[0][g_options.testcount/2], sumpert/g_options.testcount, colltime[1][g_options.testcount/2]);
  } // end of loop over "ratio"
}

} /* extern C */

#else
extern "C" {
int register_pattern_collvsnoise(void) {return 0;};
} /* extern C */
#endif
