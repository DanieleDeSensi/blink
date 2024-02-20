/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_SYNCTEST && defined NG_MPI
#include "hrtimer/hrtimer.h"
#include <stdint.h>

extern "C" {
#include "ng_sync.h"

extern struct ng_options g_options;

/* internal function prototypes */
static void synctest_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_synctest = {
   pattern_synctest.name = "synctest",
   pattern_synctest.desc = "tests synchronization precision",
   pattern_synctest.flags = 0,
   pattern_synctest.do_benchmarks = synctest_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_synctest() {
   ng_register_pattern(&pattern_synctest);
   return 0;
}

static void synctest_do_benchmarks(struct ng_module *module) {

  int rank;

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  HRT_TIMESTAMP_T t1;
  unsigned long long time, roottime;
  long long timediff;


  unsigned long long win=0;
  const int tries=100;
  double *diffs = (double*)malloc(tries*sizeof(double));
  double *errs = (double*)malloc(tries*sizeof(double));

  for(int i = 0; i < tries; i++) {
    ng_sync_init_stage1(MPI_COMM_WORLD);
    ng_sync_init_stage2(MPI_COMM_WORLD, &win);
    long err = ng_sync(win);

    HRT_GET_TIMESTAMP(t1);
    HRT_GET_TIME(t1,time);

    roottime = time;
    MPI_Bcast(&roottime, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    timediff = roottime - time;

    diffs[i] = 1e6*(double)timediff/(double)g_timerfreq;
    errs[i] = 1e6*(double)err/(double)g_timerfreq;

  }
  for(int i = 0; i < tries; i++) {
    if(rank) printf("[%i] %i diff: %lf us (base: %lf, err: %lf)\n", rank, i, diffs[i]-diffs[0], diffs[0], errs[i]);
  }
}

} /* extern C */

#else
extern "C" {
int register_pattern_synctest(void) {return 0;};
} /* extern C */
#endif
