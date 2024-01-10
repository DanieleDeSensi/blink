/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_EBB && defined NG_MPI
#include "hrtimer/hrtimer.h"
#include "mersenne/MersenneTwister.h"
#include "ptrn_ebb_cmdline.h"
#include <stdint.h>
#include <assert.h>
#include <vector>
#include <numeric>
#include <algorithm>

typedef std::pair<int, int> pair_t;
typedef std::vector<pair_t> ptrn_t;

extern "C" {
#include "ng_sync.h"

extern struct ng_options g_options;


/* internal function prototypes */
static void ebb_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_ebb = {
   pattern_ebb.name = "ebb",
   pattern_ebb.desc = "benchmarks effective bisection bandwidth",
   pattern_ebb.flags = 0,
   pattern_ebb.do_benchmarks = ebb_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_ebb() {
   ng_register_pattern(&pattern_ebb);
   return 0;
}

void genptrn_bisect_fb(int p, ptrn_t *ptrn, int print) {
  std::vector<int> mixer(p);

  if(p%2) ng_abort("number of nodes must be divisible by 2!\n");

  int invalid = 1;
  MTRand mtrand;
  do {
    int next=0;
    /* select p random numbers and the first p/2 communicate with the second p/2 */
    while(next < p) {
      int myrand = mtrand.randInt(p-1);
      /* look if this is already in mixer */
      int found = 0;
      for(int i = 0; i<next; i++) {
        if(mixer[i] == myrand) {
          found=1;
        }
      }
      if(!found) {
        mixer[next] = myrand;
        next++;
      }
    }
    invalid = 0;
  } while(invalid);
  for(int i = 0; i<p/2; i++) {
    std::pair<int,int> mypair;
    mypair.first = mixer[i+p/2];
    mypair.second = mixer[i];
    ptrn->push_back(mypair);
    mypair.first = mixer[i];
    mypair.second = mixer[i+p/2];
    ptrn->push_back(mypair);
    if(print) printf("%i <-> %i\n", mypair.first, mypair.second);
  }
}


static void ebb_do_benchmarks(struct ng_module *module) {
	//parse cmdline arguments
	struct ptrn_ebb_cmd_struct args_info;
	//printf("The string I got: %s\n", g_options.ptrnopts);
	if (ptrn_ebb_parser_string(g_options.ptrnopts, &args_info, "netgauge") != 0) {
		exit(EXIT_FAILURE);
	}

  long max_data_size = ng_min(g_options.max_datasize + module->headerlen, module->max_datasize);
  long data_size, test_count;
  int r=g_options.mpi_opts->worldrank;
  int p=g_options.mpi_opts->worldsize;

  /* get needed data buffer memory */
  ng_info(NG_VLEV1, "Allocating 2 %d bytes data buffers", max_data_size);
  char *rbuf; // = (char*)module->malloc(max_data_size);
  NG_MALLOC(module, char*, max_data_size, rbuf);
  char *sbuf; // = (char*)module->malloc(max_data_size);
  NG_MALLOC(module, char*, max_data_size, sbuf);

  ng_info(NG_VLEV2, "Initializing data buffer (make sure it's really allocated)");
  for (int i = 0; i < max_data_size; i++) rbuf[i] = 0xff;
  for (int i = 0; i < max_data_size; i++) sbuf[i] = 0xff;

  int *peers = new int[2*p];
  std::fill(peers,peers+2*p,MPI_PROC_NULL);

  for (data_size = g_options.min_datasize; data_size > 0;
       get_next_testparams(&data_size, &test_count, &g_options, module)) {
    if(data_size == -1) break;

    int num_buckets = args_info.buckets_arg;
    std::vector<int> buckets(num_buckets);
    std::fill(buckets.begin(), buckets.end(), 0);
    std::vector<double> times(p*args_info.rounds_arg);

    HRT_TIMESTAMP_T t1, t2;

    ptrn_t ptrn;
    for(int round=0; round<args_info.rounds_arg;round++) {
      if(r ==0 && (NG_VLEV1 & g_options.verbose)) {
        printf("# size: %lu round %i/%i\n", data_size, round+1, args_info.rounds_arg);
      }

      /* rank 0 generates the random bisection pattern ... */
      if(r == 0) {
        genptrn_bisect_fb(p, &ptrn, args_info.print_ptrn_flag);
 
        /* pack the pattern for scatter */
        for(ptrn_t::iterator iter = ptrn.begin(); iter != ptrn.end(); iter++) {
          //printf("%i -> %i\n", iter->first, iter->second);
          peers[2*iter->first] = iter->second;
          peers[2*iter->second+1] = iter->first;
        }
      }
 
      /* communicate the pattern to all ranks */
      int peer[2];
      MPI_Scatter(peers, 2, MPI_INT, &peer, 2, MPI_INT, 0, MPI_COMM_WORLD);
      //printf("[%i] sending to %i, receiving from %i\n", r, peer[0], peer[1]);
 
      MPI_Barrier(MPI_COMM_WORLD); // this is just to be extra-sure
      /*ng_sync_init_stage1(MPI_COMM_WORLD);
      ng_sync_init_stage2(MPI_COMM_WORLD, &win);
      long err = ng_sync(win);*/
       
      HRT_GET_TIMESTAMP(t1);
      for(int i=0; i<args_info.reps_arg; i++) {
        MPI_Request req[2];
        MPI_Irecv(rbuf, data_size, MPI_BYTE, peer[1], 0, MPI_COMM_WORLD, &req[0]);
        MPI_Isend(sbuf, data_size, MPI_BYTE, peer[0], 0, MPI_COMM_WORLD, &req[1]);
        MPI_Waitall(2, &req[0], MPI_STATUSES_IGNORE);
      }
      HRT_GET_TIMESTAMP(t2);
      
      uint64_t num_ticks;
      HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks);
      double us = 1e6 * num_ticks / (double) g_timerfreq;

      MPI_Gather(&us, 1, MPI_DOUBLE, &times[round*p], 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

      double sum=0;
      /* sum up */
      for(int i = round*p; i<(round+1)*p; i++) {
        sum += times[i];
      }
      if(r == 0) printf("size: %i, round %i: num: %i average: %lf us (%lf MiB/s)\n", data_size, round, p, sum/p, (data_size*args_info.reps_arg)/(sum/p));

    }

    if((r == 0) && (num_buckets)) {
      ng_info(NG_VNORM, "---- bucket data ----");

      /* last round */
      std::vector<double>::iterator miter = std::min_element(times.begin(), times.end());
      double min = *miter;
      miter = std::max_element(times.begin(), times.end());
      double max = *miter;
      double width = (max-min)/num_buckets;

      double sum=0;
      /* add values to buckets */
      for(int i = 0; i<times.size(); i++) {
        sum += times[i];
        int bucket = (int)floor((times[i]-min) / width);
        //printf("-- %i, %lf, %lf, %lf\n", bucket, times[i], min, width);
        if(bucket >= num_buckets) bucket=num_buckets-1;
        buckets[bucket]++;
      }

      /* print buckets */
      int num=0;
      for(int i=0; i<num_buckets; i++) {
        if(buckets[i] != 0) printf("size: %lu %lf (%lf MiB/s): %i\n", data_size, min+i*width, (data_size*args_info.reps_arg)/(min+i*width), buckets[i]);
        num+=buckets[i];
         }
      printf("size: %lu num: %i average: %lf (%lf MiB/s)\n", data_size, num, sum/num, (data_size*args_info.reps_arg)/(sum/num));
    }
  }
}

} /* extern C */

#else
extern "C" {
int register_pattern_ebb(void) {return 0;};
} /* extern C */
#endif
