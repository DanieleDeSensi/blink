/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_NBOV
#include "hrtimer/hrtimer.h"
#include "statistics.h"
#include <string.h>

/* internal function prototypes & extern stuff */
static void fnbov_do_benchmarks(struct ng_module *module);
extern struct ng_options g_options;

/**
 * one-to-many communication pattern registration
 * structure
 */
static struct ng_comm_pattern pattern_nbov = {
   .name = "nbov",
   .flags = '\0', /* we need a reliable transport and channel semantics */
   .desc = "measures overheads for non-blocking send and receive",
   .do_benchmarks = fnbov_do_benchmarks
};

/**
 * registers this comm. pattern with the main prog.,
 * but only if we use MPI
 */
int register_pattern_nbov() {
  /* there's no use for this mode without MPI 
   * TODO: there coule be :) */
#if NG_MPI
  ng_register_pattern(&pattern_nbov);
#endif 
  return 0;
}

static volatile int ctr;

static __inline__ void my_wait(int time /* usecs */) {
  double t;

  t = MPI_Wtime()*1e6;
  t += (double)time;
  while(t > MPI_Wtime()*1e6) ctr++;
}

void fnbov_do_benchmarks(struct ng_module *module) {
#if NG_MPI
	NG_Request reqs[2];
  void *buffer;
  int nprocs, time;
  long data_size, test_num;
  
  /** # times to run with current data_size */
  unsigned long test_count = g_options.testcount;

  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	if (nprocs != 2) {
		printf("Exactly two nodes please!\n");
        MPI_Abort(MPI_COMM_WORLD,1);
		exit (1);
	}

  /* allocate send / recv buffer */
  ng_info(NG_VLEV2, "Allocating %d bytes data buffer", g_options.max_datasize + module->headerlen);
  NG_MALLOC(module, void*, g_options.max_datasize + module->headerlen, buffer);
  if (!buffer) {
    ng_perror("Failed to allocate data buffer");
    goto shutdown;
  }

  if(!g_options.mpi_opts->worldrank)
    printf("# datasize wait o(send) o(recv) | o(send test) o(recv test) | rtt\n");
  /* outer test loop, sets data_size and test_count */
  for (data_size = NG_START_PACKET_SIZE; !g_stop_tests && data_size > 0;
       get_next_testparams(&data_size, &test_count, &g_options, module)) {
    
    ng_info(NG_VLEV1, "Testing %d times with %d bytes:", test_count, data_size);

#define NG_WARMUP test_count
    for (test_num=0; test_num < NG_WARMUP; test_num++) {
      if (g_options.mpi_opts->worldrank == 0) {
        module->isendto(1, buffer, data_size, &reqs[0]);
        while(0 != module->test(&reqs[0]));
        module->irecvfrom(1, buffer, data_size, &reqs[1]);
        while(0 != module->test(&reqs[1]));
      } else {
        /* act as packet mirror */
        module->irecvfrom(0, buffer, data_size, &reqs[1]);
        while(0 != module->test(&reqs[1]));
        module->isendto(0, buffer, data_size, &reqs[0]);
        while(0 != module->test(&reqs[0]));
      }
    }
    
    for (time=0; time < 1; time++) {
      double send=0, recv=0, stest=0, rtest=0, pingpong=0; /* the times */
      HRT_TIMESTAMP_T t[5];
      unsigned long long tsend, trecv, tstest, trtest, tpp;
      for (test_num=0; test_num < test_count; test_num++) {

        if ((test_count < NG_DOT_COUNT || !(test_num % (int)(test_count / NG_DOT_COUNT)))) {
          ng_info(NG_VLEV1, ".");
          fflush(stdout);
        }

        if (g_options.mpi_opts->worldrank == 0) {
          int sctr=1, rctr=1;

          HRT_GET_TIMESTAMP(t[0]);
          module->isendto(1, buffer, data_size, &reqs[0]);
          HRT_GET_TIMESTAMP(t[1]);
          while(0 != module->test(&reqs[0])) { sctr++; /*wait(time);*/ }
          HRT_GET_TIMESTAMP(t[2]);
          module->irecvfrom(1, buffer, data_size, &reqs[1]);
          HRT_GET_TIMESTAMP(t[3]);
          while(0 != module->test(&reqs[1])) { rctr++; /*wait(time);*/ }
          HRT_GET_TIMESTAMP(t[4]);
        
          HRT_GET_ELAPSED_TICKS(t[0],t[1],&tsend);
          send += HRT_GET_USEC(tsend);
          HRT_GET_ELAPSED_TICKS(t[2],t[3],&trecv);
          recv += HRT_GET_USEC(trecv);
          HRT_GET_ELAPSED_TICKS(t[1],t[2],&tstest);
          stest += HRT_GET_USEC(tstest)/sctr;
          HRT_GET_ELAPSED_TICKS(t[3],t[4],&trtest);
          rtest += HRT_GET_USEC(trtest)/rctr;
          HRT_GET_ELAPSED_TICKS(t[0],t[4],&tpp);
          pingpong += HRT_GET_USEC(tpp);
          
        } else {
          /* act as packet mirror */
          module->irecvfrom(0, buffer, data_size, &reqs[1]);
          while(0 != module->test(&reqs[1]));
          module->isendto(0, buffer, data_size, &reqs[0]);
          while(0 != module->test(&reqs[0]));
        }
      }
      send = send/test_count;
      recv = recv/test_count;
      stest = stest/test_count;
      rtest = rtest/test_count;
      pingpong = pingpong/test_count;
      if(!g_options.mpi_opts->worldrank)
        printf("\n%li %i %lf %lf | %lf %lf | %lf\n", data_size, time, send, recv, stest, rtest, pingpong);
    }
  }
 shutdown:
   /* clean up */
   free(buffer);
#endif
}

#else
int register_pattern_nbov(void) {return 0;}
#endif
