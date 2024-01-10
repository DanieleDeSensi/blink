/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"

extern "C" {

#ifdef NG_PTRN_FUNC_ARGS
#include "hrtimer/hrtimer.h"
#include "ptrn_func_args_callee.h"
#include <stdlib.h>


extern struct ng_options g_options;

/* internal function prototypes */
static void func_args_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_func_args = {
   pattern_func_args.name = "func_args",
   pattern_func_args.desc = "measures function call overhead with different argument counts",
   pattern_func_args.flags = 0,
   pattern_func_args.do_benchmarks = func_args_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_func_args() {
   ng_register_pattern(&pattern_func_args);
   return 0;
}

static void func_args_do_benchmarks(struct ng_module *module) {
  register int testcount = g_options.testcount;
  HRT_TIMESTAMP_T t1, t2;
  int arg1=rand(),arg2=rand(),arg3=rand(),arg4=rand(),arg5=rand(),arg6=rand(),
      arg7=rand(),arg8=rand(),arg9=rand(),arg10=rand(),arg11=rand(),arg12=rand();
  

  ng_info(NG_VNORM,"to run this test, is recommended that Netgauge is configured with: ./configure CFLAGS=\"-O0\" CXXFLAGS=\"-O0\"!");
  ng_info(NG_VNORM,"starting runs ... ");

  ng_test_args0();
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args0();
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks0;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks0);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args1(&arg1);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args1(&arg1);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks1;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks1);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args2(&arg1, &arg2);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args2(&arg1, &arg2);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks2;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks2);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args3(&arg1, &arg2, &arg3);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args3(&arg1, &arg2, &arg3);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks3;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks3);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args4(&arg1, &arg2, &arg3, &arg4);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args4(&arg1, &arg2, &arg3, &arg4);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks4;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks4);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args5(&arg1, &arg2, &arg3, &arg4, &arg5);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args5(&arg1, &arg2, &arg3, &arg4, &arg5);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks5;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks5);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args6(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args6(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks6;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks6);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args7(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args7(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks7;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks7);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args8(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args8(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks8;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks8);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args9(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args9(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks9;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks9);
  
  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args10(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args10(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks10;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks10);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args11(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args11(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks11;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks11);

  // warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_args12(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11, &arg12);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_args12(&arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11, &arg12);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks12;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks12);

  // comparison between pointers and direct types -- warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_pointers(&arg1, &arg2, &arg3);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_pointers(&arg1, &arg2, &arg3);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks_pointers;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks_pointers);

  // comparison between pointers and direct types -- warmup
  for(rint i=0; i<testcount; ++i) {
    ng_test_direct(arg1, arg2, arg3);
  }
  HRT_GET_TIMESTAMP(t1);
  for(rint i=0; i<testcount; ++i) {
    ng_test_direct(arg1, arg2, arg3);
  }
  HRT_GET_TIMESTAMP(t2);
  uint64_t num_ticks_direct;
  HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks_direct);

  printf("# nargs time per call [ns]\n");
  printf("0 %f \n", 1e9 * (num_ticks0/(double) g_timerfreq/testcount));
  printf("1 %f \n", 1e9 * (num_ticks1/(double) g_timerfreq/testcount));
  printf("2 %f \n", 1e9 * (num_ticks2/(double) g_timerfreq/testcount));
  printf("3 %f \n", 1e9 * (num_ticks3/(double) g_timerfreq/testcount));
  printf("4 %f \n", 1e9 * (num_ticks4/(double) g_timerfreq/testcount));
  printf("5 %f \n", 1e9 * (num_ticks5/(double) g_timerfreq/testcount));
  printf("6 %f \n", 1e9 * (num_ticks6/(double) g_timerfreq/testcount));
  printf("7 %f \n", 1e9 * (num_ticks7/(double) g_timerfreq/testcount));
  printf("8 %f \n", 1e9 * (num_ticks8/(double) g_timerfreq/testcount));
  printf("9 %f \n", 1e9 * (num_ticks9/(double) g_timerfreq/testcount));
  printf("10 %f \n", 1e9 * (num_ticks10/(double) g_timerfreq/testcount));
  printf("11 %f \n", 1e9 * (num_ticks11/(double) g_timerfreq/testcount));
  printf("12 %f \n", 1e9 * (num_ticks12/(double) g_timerfreq/testcount));
  printf("pointers %f vs. direct %f (3 args)\n", 1e9 * (num_ticks_pointers/(double) g_timerfreq/testcount), 1e9 * (num_ticks_direct/(double) g_timerfreq/testcount));
}


#else
int register_pattern_func_args(void) {return 0;};
#endif

} /* extern C */
