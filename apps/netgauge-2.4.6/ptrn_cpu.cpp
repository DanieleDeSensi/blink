/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@illinois.edu>
 *
 */

#include "netgauge.h"
#ifdef NG_PTRN_CPU
#include "hrtimer/hrtimer.h"
#include "MersenneTwister.h"
//#include "ptrn_cpu_cmdline.h"
#include "ng_tools.hpp"
#include <vector>
#include <time.h>
#include <algorithm>
#include <numeric>

#ifdef NG_HPM
#include <libhpc.h>
#endif

//#define TYPE double
//#define TYPE float
//#define TYPE signed int
#define TYPE unsigned int
//#define TYPE unsigned long long

#define REGISTER 

#define TESTS 10e6

extern "C" {

/* this only exists to prevent compiler optimizations that remove the
 * cachewiper ;) */
volatile TYPE NG_CPU_res=0;

extern struct ng_options g_options;

/* internal function prototypes */
static void cpu_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_cpu = {
   pattern_cpu.name = "cpu",
   pattern_cpu.desc = "measures cpu performance",
   pattern_cpu.flags = 0,
   pattern_cpu.do_benchmarks = cpu_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_cpu() {
   ng_register_pattern(&pattern_cpu);
   return 0;
}

static void cpu_do_benchmarks(struct ng_module *module) {

  /** currently tested packet size and maximum */
  long data_elems;

  /** number of times to test the current datasize */
  long test_count = g_options.testcount;

  /** counts up to test_count */
  int test_round = 0;

  std::vector<double> alltimes, unrtimes, vectimes;

  /* Inner test loop
   * - run the requested number of tests for the current data size
   * - but only if testtime does not exceed the max. allowed test time
   *   (only if max. test time is not zero)
   */
  for (int test = -1 /* 1 warmup test */; test < test_count; test++) {
    HRT_TIMESTAMP_T t[2];
    MTRand mt;
    unsigned long long ticks;
    REGISTER TYPE x=(TYPE)mt.rand(), y=(TYPE)mt.rand();
    TYPE x0, x1,x2,x3,x4,x5,x6,x7;
    TYPE x8, x9,x10,x11,x12,x13,x14,x15;
    x0=mt.rand(); x1=mt.rand();x2=mt.rand();x3=mt.rand();x4=mt.rand();x5=mt.rand();x6=mt.rand();x7=mt.rand();
    x8=mt.rand(); x9=mt.rand();x10=mt.rand();x11=mt.rand();x12=mt.rand();x13=mt.rand();x14=mt.rand();x15=mt.rand();
	
    // no unroll
#ifdef NG_HPM
        if(test > -1 ) hpmStart(80,"nounroll");
#endif
    HRT_GET_TIMESTAMP(t[0]);
#if defined(__xlc__) || defined(__xlC__)
#pragma nounroll
#pragma nosimd
    for(int i=0; i < TESTS; ++i) {
#else
    for(int i=0; i < TESTS; ++i) {
#endif
      x = x+y;
    }
    HRT_GET_TIMESTAMP(t[1]);
    HRT_GET_ELAPSED_TICKS(t[0],t[1],&ticks);

#ifdef NG_HPM
        if(test > -1 ) hpmStop(80);
#endif

    if(test >=0) alltimes.push_back(HRT_GET_USEC(ticks));

#ifdef NG_HPM
        if(test > -1 ) hpmStart(90,"unroll8");
#endif
    // 8x unrolled loop
    HRT_GET_TIMESTAMP(t[0]);
#if defined(__xlc__) || defined(__xlC__)
#pragma nounroll
#pragma nosimd
    for(int i=0; i < TESTS; i+=8) {
#else
    for(int i=0; i < TESTS; i+=8) {
#endif
      x0 = x0+y;
      x1 = x1+y;
      x2 = x2+y;
      x3 = x3+y;
      x4 = x4+y;
      x5 = x5+y;
      x6 = x6+y;
      x7 = x7+y;
    }
    x = x + x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7;
    HRT_GET_TIMESTAMP(t[1]);
    HRT_GET_ELAPSED_TICKS(t[0],t[1],&ticks);
#ifdef NG_HPM
        if(test > -1 ) hpmStop(90);
#endif
    if(test >=0) unrtimes.push_back(HRT_GET_USEC(ticks));
    
#ifdef NG_HPM
        if(test > -1 ) hpmStart(100,"vecloop");
#endif
    // vectorized loop
    HRT_GET_TIMESTAMP(t[0]);
#if defined(__xlc__) || defined(__xlC__)
   { 
    vector TYPE xx, xx0, xx1, xx2, xx3, xx4, xx5, xx6, xx7, xx8, xx9, xx10, xx11, xx12, xx13, xx14, xx15;
    vector TYPE yy;

    xx0=vec_splats(x);
    xx1=vec_splats(x);
    xx2=vec_splats(x);
    xx3=vec_splats(x);
    xx4=vec_splats(x);
    xx5=vec_splats(x);
    xx6=vec_splats(x);
    xx7=vec_splats(x);
    xx8=vec_splats(x);
    xx9=vec_splats(x);
    xx10=vec_splats(x);
    xx11=vec_splats(x);
    xx12=vec_splats(x);
    xx13=vec_splats(x);
    xx14=vec_splats(x);
    xx15=vec_splats(x);
    yy=vec_splats(y);
    for(int i=0; i < TESTS; i+=16*(16/sizeof(TYPE))) {
      xx0=vec_add(xx0,yy);
      xx1=vec_add(xx1,yy);
      xx2=vec_add(xx2,yy);
      xx3=vec_add(xx3,yy);
      xx4=vec_add(xx4,yy);
      xx5=vec_add(xx5,yy);
      xx6=vec_add(xx6,yy);
      xx7=vec_add(xx7,yy);
      xx8=vec_add(xx8,yy);
      xx9=vec_add(xx9,yy);
      xx10=vec_add(xx10,yy);
      xx11=vec_add(xx11,yy);
      xx12=vec_add(xx12,yy);
      xx13=vec_add(xx13,yy);
      xx14=vec_add(xx14,yy);
      xx15=vec_add(xx15,yy);
    }
    for (int i=0;i<16/sizeof(TYPE); ++i){
    x = x + xx0[i];
    x = x + xx1[i];
    x = x + xx2[i];
    x = x + xx3[i];
    x = x + xx4[i];
    x = x + xx5[i];
    x = x + xx6[i];
    x = x + xx7[i];
    x = x + xx8[i];
    x = x + xx9[i];
    x = x + xx10[i];
    x = x + xx11[i];
    x = x + xx12[i];
    x = x + xx13[i];
    x = x + xx14[i];
    x = x + xx15[i];
    }
    }
#else
    for(int i=0; i < TESTS; i+=8) {
      x0 = x0+y;
      x1 = x1+y;
      x2 = x2+y;
      x3 = x3+y;
      x4 = x4+y;
      x5 = x5+y;
      x6 = x6+y;
      x7 = x7+y;
    }
    x = x + x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7;
#endif
    HRT_GET_TIMESTAMP(t[1]);
    HRT_GET_ELAPSED_TICKS(t[0],t[1],&ticks);
#ifdef NG_HPM
        if(test > -1 ) hpmStop(100);
#endif
    if(test >=0) vectimes.push_back(HRT_GET_USEC(ticks));

    NG_CPU_res=x;

  }	/* end inner test loop */
  //ng_info(NG_VLEV1, "\n");
  //fflush(stdout);

  double taavg = std::accumulate(alltimes.begin(), alltimes.end(), (double)0)/(double)alltimes.size(); 
  double tamin = *std::min_element(alltimes.begin(), alltimes.end()); 
  double tamax = *std::max_element(alltimes.begin(), alltimes.end()); 
  std::vector<double>::iterator nthrtt = alltimes.begin()+alltimes.size()/2;
  std::nth_element(alltimes.begin(), nthrtt, alltimes.end());
  double tamed = *nthrtt;
  double tavar = standard_deviation(alltimes.begin(), alltimes.end(), taavg);

  double tuavg = std::accumulate(unrtimes.begin(), unrtimes.end(), (double)0)/(double)unrtimes.size(); 
  double tumin = *std::min_element(unrtimes.begin(), unrtimes.end()); 
  double tumax = *std::max_element(unrtimes.begin(), unrtimes.end()); 
  nthrtt = unrtimes.begin()+unrtimes.size()/2;
  std::nth_element(unrtimes.begin(), nthrtt, unrtimes.end());
  double tumed = *nthrtt;
  double tuvar = standard_deviation(unrtimes.begin(), unrtimes.end(), tuavg);

  double tvavg = std::accumulate(vectimes.begin(), vectimes.end(), (double)0)/(double)vectimes.size(); 
  double tvmin = *std::min_element(vectimes.begin(), vectimes.end()); 
  double tvmax = *std::max_element(vectimes.begin(), vectimes.end()); 
  nthrtt = vectimes.begin()+vectimes.size()/2;
  std::nth_element(vectimes.begin(), nthrtt, vectimes.end());
  double tvmed = *nthrtt;
  double tvvar = standard_deviation(vectimes.begin(), vectimes.end(), tuavg);
  
  printf("avg: %.2f us (%.2f MFlop/s); %.2f (%.2f MFlop/s); %.2f (%.2f MFlop/s)\nmin: %.2f us (%.2f MFlop/s); %.2f (%.2f MFlop/s); %.2f (%.2f MFlop/s)\nmax: %.2f us (%.2f MFlop/s); %.2f (%.2f MFlop/s); %.2f (%.2f MFlop/s)\nmed: %.2f us (%.2f MFlop/s); %.2f (%.2f MFlop/s); %.2f (%.2f MFlop/s)\ndev: %.2f us; %.2f us; %.2f us\n", 
      taavg, ((double)TESTS)/taavg, tuavg, ((double)TESTS)/tuavg, tvavg, ((double)TESTS)/tvavg, 
      tamin, (double)TESTS/tamin, tumin, (double)TESTS/tumin, tvmin, (double)TESTS/tvmin, 
      tamax, (double)TESTS/tamax, tumax, (double)TESTS/tumax, tvmax, (double)TESTS/tvmax, 
      tamed, (double)TESTS/tamed, tumed, (double)TESTS/tumed, tvmed, (double)TESTS/tvmed, 
      tavar, tuvar, tvvar);
    
}

} /* extern C */

#else
extern "C" {
int register_pattern_cpu(void) {return 0;};
}
#endif
