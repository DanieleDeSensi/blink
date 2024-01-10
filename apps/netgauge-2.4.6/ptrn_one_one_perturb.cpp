/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#ifdef NG_PTRN_ONE_ONE_PERTURB
#include "hrtimer/hrtimer.h"
#include <vector>
#include <time.h>
#include <algorithm>
#include "fullresult.h"
#include "statistics.h"
#include "ng_tools.hpp"


extern "C" {

extern struct ng_options g_options;

/* internal function prototypes */
static void one_one_perturb_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_one_one_perturb = {
   pattern_one_one_perturb.name = "one_one_perturb",
   pattern_one_one_perturb.desc = "measures ping-pong latency&bandwidth",
   pattern_one_one_perturb.flags = 0,
   pattern_one_one_perturb.do_benchmarks = one_one_perturb_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_one_one_perturb() {
   ng_register_pattern(&pattern_one_one_perturb);
   return 0;
}

static void one_one_perturb_do_benchmarks(struct ng_module *module) {
  /** for collecting statistics */
  struct ng_statistics statistics;

  /** currently tested packet size and maximum */
  long data_size;

  /** number of times to test the current datasize */
  long test_count = g_options.testcount;

  /** counts up to test_count */
  int test_round = 0;

  /** how long does the test run? */
  time_t test_time, cur_test_time;

  /** number of tests run */
  int ovr_tests=0, ovr_bytes=0;

  long max_data_size = ng_min(g_options.max_datasize + module->headerlen, module->max_datasize);

  /* get needed data buffer memory */
  ng_info(NG_VLEV1, "Allocating %d bytes data buffer", max_data_size);
  char *buffer; // = (char*)module->malloc(max_data_size);
  NG_MALLOC(module, char*, max_data_size, buffer);

  ng_info(NG_VLEV2, "Initializing data buffer (make sure it's really allocated)");
  for (int i = 0; i < max_data_size; i++) buffer[i] = 0xff;

  int rank = g_options.mpi_opts->worldrank;
  g_options.mpi_opts->partner = (rank+1)%2;
  
  /* buffer for header ... */
  char* txtbuf = (char *)malloc(2048 * sizeof(char));
  if (txtbuf == NULL) {
    ng_error("Could not (re)allocate 2048 byte for output buffer");
    ng_exit(10);
  }
  memset(txtbuf, '\0', 2048);
  
  /* Outer test loop
   * - geometrically increments data_size (i.e. data_size = data_size * 2)
   *  (- geometrically decrements test_count) not yet implemented
   */
  for(int compute=0; compute<2; compute++) for(int communicate=0; communicate<2; communicate++) {
    printf("### compute=%i, communicate=%i\n", compute, communicate);
  for (data_size = g_options.min_datasize; data_size > 0;
	    get_next_testparams(&data_size, &test_count, &g_options, module)) {
    if(data_size == -1) goto shutdown;
    
    ++test_round;

    // the benchmark results
    std::vector<double> tcomp, tpost, twait;
    
    ng_info(NG_VLEV1, "Round %d: testing %d times with %d bytes:", test_round, test_count, data_size);
    // if we print dots ...
    if ( (rank==0) && (NG_VLEV1 & g_options.verbose) ) {
      printf("# ");
    }
 
    //if (!g_options.server) ng_statistics_start_round(&statistics, data_size);

    /* Inner test loop
     * - run the requested number of tests for the current data size
     * - but only if testtime does not exceed the max. allowed test time
     *   (only if max. test time is not zero)
     */
    test_time = 0;
    for (int test = -1 /* 1 warmup test */; test < test_count; test++) {
	
	    /* first statement to prevent floating exception */
      /* TODO: add cool abstract dot interface ;) */
	    if ( rank == 0 && (NG_VLEV1 & g_options.verbose) && ( test_count < NG_DOT_COUNT || !(test % (int)(test_count / NG_DOT_COUNT)) )) {
	      printf(".");
	      fflush(stdout);
	    }
	
	    /* call the appropriate client or server function */
	    if (rank==1) {
	        cur_test_time = time(NULL);
          /* do the server stuff basically a simple receive and send to
           * mirror the data in a ping-pong fashion ... this is not in a
           * subfunction since this may be performance critical. The
           * send and receive functions are also macros */

          /* Phase 1: receive data (wait for data - blocking) */
          MPI_Recv(buffer, data_size, MPI_BYTE, 0 /*src*/, 0 /*tag*/, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

          /* Phase 2: send data back */
          MPI_Send(buffer, data_size, MPI_BYTE, 0 /*dst*/, 0 /*tag*/, MPI_COMM_WORLD);
  
	        test_time += time(NULL) - cur_test_time;
	    } else {
          MPI_Request req[2];
	        /* wait some time for the server to get ready */
	        usleep(10);
	        cur_test_time = time(NULL);

          /* do the client stuff ... take time, send message, wait for
           * reply and take time  ... simple ping-pong scheme */
          HRT_TIMESTAMP_T t[4];
          unsigned long long tipost, ticomp, tiwait;
  
          /* init statistics (TODO: what does this do?) */
          ng_statistics_test_begin(&statistics);

          HRT_GET_TIMESTAMP(t[0]);

          if(communicate) {
            MPI_Isend(buffer, data_size, MPI_BYTE, 0 /*dst*/, 0 /*tag*/, MPI_COMM_WORLD, &req[0]);
            MPI_Irecv(buffer, data_size, MPI_BYTE, 0 /*src*/, 0 /*tag*/, MPI_COMM_WORLD, &req[1]);
          }

          /* get after-posting time */
          HRT_GET_TIMESTAMP(t[1]);

          if(compute) {
            register int x=0;
            for(int i=0; i<1e6; i++) {x++;}
          }

          /* get after-computation time */
          HRT_GET_TIMESTAMP(t[2]);
   
          /* phase 2: receive returned data */
          if(communicate) MPI_Waitall(2, &req[0], MPI_STATUSES_IGNORE);
  
          /* after wait time */
          HRT_GET_TIMESTAMP(t[3]);
          HRT_GET_ELAPSED_TICKS(t[0],t[1],&tipost);
          HRT_GET_ELAPSED_TICKS(t[1],t[2],&ticomp);
          HRT_GET_ELAPSED_TICKS(t[2],t[3],&tiwait);
                      
          /* calculate results */
  
          if(test >= 0) {
            tpost.push_back(HRT_GET_USEC(tipost));
            tcomp.push_back(HRT_GET_USEC(ticomp));
            twait.push_back(HRT_GET_USEC(tiwait));
          }
	        test_time += time(NULL) - cur_test_time;
	    }

	    /* calculate overall statistics */
	    ovr_tests++;
	    ovr_bytes += data_size;
	 
	    /* measure test time and quit test if 
	     * test time exceeds max. test time
	     * but not if the max. test time is zero
	    */
	    if ( (g_options.max_testtime > 0) && 
	         (test_time > g_options.max_testtime) ) {
	      ng_info(NG_VLEV2, "Round %d exceeds %d seconds (duration %d seconds)", test_round, g_options.max_testtime, test_time);
	      ng_info(NG_VLEV2, "Test stopped at %d tests", test);
	      break;
	    }
	 
    }	/* end inner test loop */
    
	  if (rank==0) {
      /* add linebreak if we made dots ... */
      if ( (NG_VLEV1 & g_options.verbose) ) {
        ng_info(NG_VLEV1, "\n");
      }

      /* output statistics - compute time */
      double tcomp_avg = std::accumulate(tcomp.begin(), tcomp.end(), (double)0)/(double)tcomp.size(); 
      double tcomp_min = *min_element(tcomp.begin(), tcomp.end()); 
      double tcomp_max = *max_element(tcomp.begin(), tcomp.end()); 
      std::vector<double>::iterator nthblock = tcomp.begin()+tcomp.size()/2;
      nth_element(tcomp.begin(), nthblock, tcomp.end());
      double tcomp_med = *nthblock;
      double tcomp_var = standard_deviation(tcomp.begin(), tcomp.end(), tcomp_avg);
      int tcomp_fail = count_range(tcomp.begin(), tcomp.end(), tcomp_avg-tcomp_var*2, tcomp_avg+tcomp_var*2);

      /* output statistics - wait time */
      double twait_avg = std::accumulate(twait.begin(), twait.end(), (double)0)/(double)twait.size(); 
      double twait_min = *min_element(twait.begin(), twait.end()); 
      double twait_max = *max_element(twait.begin(), twait.end()); 
      nthblock = twait.begin()+twait.size()/2;
      nth_element(twait.begin(), nthblock, twait.end());
      double twait_med = *nthblock;
      double twait_var = standard_deviation(twait.begin(), twait.end(), twait_avg);
      int twait_fail = count_range(twait.begin(), twait.end(), twait_avg-twait_var*2, twait_avg+twait_var*2);

      /* output statistics - post time */
      double tpost_avg = std::accumulate(tpost.begin(), tpost.end(), (double)0)/(double)tpost.size(); 
      double tpost_min = *min_element(tpost.begin(), tpost.end()); 
      double tpost_max = *max_element(tpost.begin(), tpost.end()); 
      std::vector<double>::iterator nthrtt = tpost.begin()+tpost.size()/2;
      std::nth_element(tpost.begin(), nthrtt, tpost.end());
      double tpost_med = *nthrtt;
      double tpost_var = standard_deviation(tpost.begin(), tpost.end(), tpost_avg);
      int tpost_fail = count_range(tpost.begin(), tpost.end(), tpost_avg-tpost_var*2, tpost_avg+tpost_var*2);

      memset(txtbuf, '\0', 2048);
      snprintf(txtbuf, 2047,
	      "%ld bytes \t -> %.2lf us, %.2lf us. %.2lf us\n",
        data_size, /* packet size */
        tpost_med, /* minimum RTT time */
        tcomp_med, /* minimum RTT time */
        twait_med /* minimum RTT time */
        );
      printf("%s", txtbuf);
    }

  ng_info(NG_VLEV1, "\n");
  fflush(stdout);

    /* only a client does the stats stuff */
    //if (!g_options.server)
	  //  ng_statistics_finish_round(&statistics);
      
  } }	/* end outer test loop */

   
 shutdown:
   if(txtbuf) free(txtbuf);
    
}

} /* extern C */

#else
extern "C" {
int register_pattern_one_one_perturb(void) {return 0;};
}
#endif
