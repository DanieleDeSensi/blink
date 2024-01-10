/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_ONE_ONE_DTYPE && defined NG_MPI
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
static void one_one_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_one_one_dtype = {
   pattern_one_one_dtype.name = "one_one_dtype",
   pattern_one_one_dtype.desc = "measures latency&bandwidth with varying strides in a datatype",
   pattern_one_one_dtype.flags = 0,
   pattern_one_one_dtype.do_benchmarks = one_one_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_one_one_dtype() {
   ng_register_pattern(&pattern_one_one_dtype);
   return 0;
}

static void one_one_do_benchmarks(struct ng_module *module) {
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
  max_data_size = std::max((long)16777216, max_data_size); // at least 16 MiB

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

  /* header printing */
  if(rank == 0) {
    // if very verbose - long output
    if (NG_VLEV2 & g_options.verbose) {
      snprintf(txtbuf, 2047,
	      "## Netgauge v%s - mode %s - 2 processes\n"
	      "##\n"
        "## A...message size [byte]\n"
        "##\n"
        "## B...minimum send blocking time\n"
        "## C...average send blocking time\n"
        "## D...median send blocking time\n"
        "## E...maximum send blocking time\n"
        "## F...standard deviation for send blocking time (stddev)\n"
        "## G...number of send blocking time values, that were bigger than avg + 2 * stddev.\n"
        "##\n"
        "## H...minimum RTT/2\n"
        "## I...average RTT/2\n"
        "## J...median RTT/2\n"
        "## K...maximum RTT/2\n"
        "## L...standard deviation for RTT/2 (stddev)\n"
        "## M...number of RTT/2 values, that were bigger than avg + 2 * stddev.\n"
        "##\n"
        "## N...minimum throughput [Mbit/sec]\n"
        "## O...average throughput [Mbit/sec]\n"
        "## P...median throughput [Mbit/sec]\n"
        "## Q...maximum throughput [Mbit/sec]\n"
        "##\n"
        "## A  -  B  C  D  E  (F G) - H  I  J  K  (L M)  -  N  O  P  Q\n",
        NG_VERSION,
        g_options.mode);

      printf("%s", txtbuf);
    } else
    // if verbose - short output
    if (NG_VLEV1 & g_options.verbose) {
      snprintf(txtbuf, 2047,
	      "## Netgauge v%s - mode %s - 2 processes\n"
	      "##\n"
        "## A...message size [byte]\n"
        "##\n"
        "## B...minimum send blocking time\n"
        "## C...average send blocking time\n"
        "## D...median send blocking time\n"
        "## E...maximum send blocking time\n"
        "##\n"
        "## F...minimum RTT/2\n"
        "## G...average RTT/2\n"
        "## H...median RTT/2\n"
        "## I...maximum RTT/2\n"
        "##\n"
        "## J...minimum throughput [Mbit/sec]\n"
        "## K...average throughput [Mbit/sec]\n"
        "## L...median throughput [Mbit/sec]\n"
        "## M...maximum throughput [Mbit/sec]\n"
        "##\n"
        "## A  -  B  C  D  E - F  G  H  I - J  K  L  M\n",
        NG_VERSION,
        g_options.mode);

      printf("%s", txtbuf);
      
    } else
    // if not verbose - short output
    {
      // no header ...
    }

  }
  
  /* Outer test loop
   * - geometrically increments data_size (i.e. data_size = data_size * 2)
   *  (- geometrically decrements test_count) not yet implemented
   */
  /*for (data_size = g_options.min_datasize; data_size > 0;
	    get_next_testparams(&data_size, &test_count, &g_options, module)) {*/
  
  // CHANGE PARAMETERS HERE
  data_size=max_data_size;
  int stride_inc=1;
  int stride_max = 1000;
  int count = data_size/(stride_max+1);  
  
  for (int stride=1; stride < max_data_size && stride < stride_max; stride+=stride_inc) {
    ++test_round;

    // the benchmark results
    std::vector<double> tblock, trtt;
    
    ng_info(NG_VLEV1, "Round %d: testing %d times with %d bytes:", test_round, test_count, data_size);
    // if we print dots ...
    if ( (rank==0) && (NG_VLEV1 & g_options.verbose) ) {
      printf("# ");
    }
 
    //if (!g_options.server) ng_statistics_start_round(&statistics, data_size);

    /* create new strided datatype :) */
    MPI_Datatype str_dt;
    MPI_Type_vector(count, 1, stride, MPI_CHAR, &str_dt);
    MPI_Type_commit(&str_dt);


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
          //NG_RECV(g_options.mpi_opts->partner, buffer, data_size, module);
          MPI_Recv(buffer, 1, str_dt, 0 /*src*/, 0 /*tag*/, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

          /* Phase 2: send data back */
          //NG_SEND(g_options.mpi_opts->partner, buffer, data_size, module);
          MPI_Send(buffer, 1, str_dt, 0 /*dst*/, 0 /*tag*/, MPI_COMM_WORLD);
  
	        test_time += time(NULL) - cur_test_time;
	    } else {
	        /* wait some time for the server to get ready */
	        usleep(10);
	        cur_test_time = time(NULL);

          /* do the client stuff ... take time, send message, wait for
           * reply and take time  ... simple ping-pong scheme */
          HRT_TIMESTAMP_T t[3];
          unsigned long long tibl, tirtt;
  
          /* init statistics (TODO: what does this do?) */
          //ng_statistics_test_begin(&statistics);

          HRT_GET_TIMESTAMP(t[0]);

          //NG_SEND(g_options.mpi_opts->partner, buffer, data_size, module);
          MPI_Send(buffer, 1, str_dt, 1 /*src*/, 0 /*tag*/, MPI_COMM_WORLD);

          /* get after-sending time */
          HRT_GET_TIMESTAMP(t[1]);
   
          /* phase 2: receive returned data */
          //NG_RECV(g_options.mpi_opts->partner, buffer, data_size, module);
          MPI_Recv(buffer, 1, str_dt, 1 /*dst*/, 0 /*tag*/, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  
          HRT_GET_TIMESTAMP(t[2]);
          HRT_GET_ELAPSED_TICKS(t[0],t[1],&tibl);
          HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
                      
          /* TODO: check received data */
  
          /* calculate results */
  
          if(test >= 0) {
            trtt.push_back(HRT_GET_USEC(tirtt)/2);
            tblock.push_back(HRT_GET_USEC(tibl));
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

    MPI_Type_free(&str_dt);
    
	  if (rank==0) {
      /* add linebreak if we made dots ... */
      if ( (NG_VLEV1 & g_options.verbose) ) {
        ng_info(NG_VLEV1, "\n");
      }

      /* output statistics - blocking time */
      double tblock_avg = std::accumulate(tblock.begin(), tblock.end(), (double)0)/(double)tblock.size(); 
      double tblock_min = *min_element(tblock.begin(), tblock.end()); 
      double tblock_max = *max_element(tblock.begin(), tblock.end()); 
      std::vector<double>::iterator nthblock = tblock.begin()+tblock.size()/2;
      nth_element(tblock.begin(), nthblock, tblock.end());
      double tblock_med = *nthblock;
      double tblock_var = standard_deviation(tblock.begin(), tblock.end(), tblock_avg);
      int tblock_fail = count_range(tblock.begin(), tblock.end(), tblock_avg-tblock_var*2, tblock_avg+tblock_var*2);

      /* output statistics - rtt time */
      double trtt_avg = std::accumulate(trtt.begin(), trtt.end(), (double)0)/(double)trtt.size(); 
      double trtt_min = *min_element(trtt.begin(), trtt.end()); 
      double trtt_max = *max_element(trtt.begin(), trtt.end()); 
      std::vector<double>::iterator nthrtt = trtt.begin()+trtt.size()/2;
      std::nth_element(trtt.begin(), nthrtt, trtt.end());
      double trtt_med = *nthrtt;
      double trtt_var = standard_deviation(trtt.begin(), trtt.end(), trtt_avg);
      int trtt_fail = count_range(trtt.begin(), trtt.end(), trtt_avg-trtt_var*2, trtt_avg+trtt_var*2);

      // if very verbose - long output
      if (NG_VLEV2 & g_options.verbose) {
        memset(txtbuf, '\0', 2048);
        snprintf(txtbuf, 2047,
	        "%ld %ld - %.2lf %.2lf %.2lf %.2lf (%.2lf %i) - %.2lf %.2lf %.2lf %.2lf (%.2lf %i) - %.2lf %.2lf %.2lf %.2lf\n",
          stride, count,

          tblock_min, /* minimum send blocking time */
          tblock_avg, /* average send blocking time */
          tblock_med, /* median send blocking time */
          tblock_max, /* maximum send blocking time */ 

          tblock_var, /* standard deviation */
          tblock_fail, /* how many are bigger than twice the standard deviation? */
          
          trtt_min, /* minimum RTT time */
          trtt_avg, /* average RTT time */
          trtt_med, /* median RTT time */
          trtt_max, /* maximum RTT time */
          
          trtt_var, /* standard deviation */
          trtt_fail, /* how many are bigger than twice the standard deviation? */

          count/trtt_max*8, /* minimum bandwidth */
          count/trtt_avg*8, /* average bandwidth */
          count/trtt_med*8, /* median bandwidth */
          count/trtt_min*8 /* maximum bandwidth */
          );
        printf("%s", txtbuf);
        
      } else
      // if verbose - short output
      if (NG_VLEV1 & g_options.verbose) {
        memset(txtbuf, '\0', 2048);
        snprintf(txtbuf, 2047,
	        "%ld %ld - %.2lf %.2lf %.2lf %.2lf - %.2lf %.2lf %.2lf %.2lf - %.2lf %.2lf %.2lf %.2lf\n",
          stride, count, 

          tblock_min, /* minimum send blocking time */
          tblock_avg, /* average send blocking time */
          tblock_med, /* median send blocking time */
          tblock_max, /* maximum send blocking time */ 
          
          trtt_min, /* minimum RTT time */
          trtt_avg, /* average RTT time */
          trtt_med, /* median RTT time */
          trtt_max, /* maximum RTT time */
          
          count/trtt_max*8, /* minimum bandwidth */
          count/trtt_avg*8, /* average bandwidth */
          count/trtt_med*8, /* median bandwidth */
          count/trtt_min*8 /* maximum bandwidth */
          );
        printf("%s", txtbuf);
        
      } else
      // if not verbose - short output
      {
        memset(txtbuf, '\0', 2048);
        snprintf(txtbuf, 2047,
	        "stride %ld \t %ld x %ld bytes \t -> %.2lf us \t == %.2lf Mbit/s\n",
          stride, count, sizeof(char),
          //trtt_med, /* median RTT time */
          trtt_min, /* minimum RTT time */
          count/trtt_med*8 /* median bandwidth */
          );
        printf("%s", txtbuf);
        
      }
    }

  ng_info(NG_VLEV1, "\n");
  fflush(stdout);

    /* only a client does the stats stuff */
    //if (!g_options.server)
	  //  ng_statistics_finish_round(&statistics);
      
  }	/* end outer test loop */

   
 shutdown:
   if(txtbuf) free(txtbuf);
    
}

} /* extern C */

#else
extern "C" {
int register_pattern_one_one_dtype(void) {return 0;};
}
#endif
