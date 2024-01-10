/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_1TON
#include "hrtimer/hrtimer.h"
#include <vector>
#include "statistics.h"
#include <string.h>
#include <algorithm>
#include "ng_tools.hpp"

/* internal function prototypes & extern stuff */
static void f1toN_do_benchmarks(struct ng_module *module);
extern struct ng_options g_options;

/**
 * one-to-many communication pattern registration
 * structure
 */
static struct ng_comm_pattern pattern_1toN = {
   pattern_1toN.name = "1toN",
   pattern_1toN.desc = "measures bandwith in a 1:N congestion situation",
   pattern_1toN.flags = NG_PTRN_NB,
   pattern_1toN.do_benchmarks = f1toN_do_benchmarks
};

/**
 * registers this comm. pattern with the main prog.,
 * but only if we use MPI
 */
int register_pattern_1toN() {
  /* there's no use for this mode without MPI */
#if NG_MPI
  ng_register_pattern(&pattern_1toN);
#endif 
  return 0;
}

void f1toN_do_benchmarks(struct ng_module *module) {
#if NG_MPI
  /** current data size for transmission tests */
  long data_size;

  /** # times to run with current data_size */
  long test_count = g_options.testcount;

  /** send/receive buffer */
  void *buffer;

  /** enumerating things is such a fun! */
  int i;
  
  /* server role - send to and receive from clients */
  NG_Request *rreqs=0, *sreqs=0;
    
  /* allocate send / recv buffer */
  ng_info(NG_VLEV2, "Allocating %d bytes data buffer", g_options.max_datasize + module->headerlen);
  NG_MALLOC(module, void*, g_options.max_datasize + module->headerlen, buffer);

  int rank = g_options.mpi_opts->worldrank;

  /* initialize the statistics stuff - rank 1 does the whole measurement */
  if(rank == 0) {
    /* allocate Request structure at server */
    sreqs = (NG_Request*)malloc(sizeof(NG_Request) * (g_options.mpi_opts->worldsize-1) );
    rreqs = (NG_Request*)malloc(sizeof(NG_Request) * (g_options.mpi_opts->worldsize-1) );
  }

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
        "## A...packet size [byte]\n"
        "##\n"
        "## B...minimum send blocking time\n"
        "## C...average send blocking time\n"
        "## D...median send blocking time\n"
        "## E...maximum send blocking time\n"
        "## F...standard deviation for send blocking time\n"
        "## G...number of send blocking time values, that were bigger than 2*avg.\n"
        "##\n"
        "## H...minimum RTT/2\n"
        "## I...average RTT/2\n"
        "## J...median RTT/2\n"
        "## K...maximum RTT/2\n"
        "## L...standard deviation for RTT/2\n"
        "## M...number of RTT/2 values, that were bigger than 2*avg.\n"
        "##\n"
        "## N...minimum throughput [Mbit/sec]\n"
        "## O...average throughput [Mbit/sec]\n"
        "## P...median throughput [Mbit/sec]\n"
        "## Q...maximum throughput [Mbit/sec]\n"
        "##\n"
        "## A  -  B  C  S  D  (E F) - G  H  T  I  (J K)  -  L  M  U  N\n",
        NG_VERSION,
        g_options.mode);

      printf("%s", txtbuf);
    } else
    // if verbose - short output
    if (NG_VLEV1 & g_options.verbose) {
      snprintf(txtbuf, 2047,
	      "## Netgauge v%s - mode %s - 2 processes\n"
	      "##\n"
        "## A...packet size [byte]\n"
        "##\n"
        "## B...minimum send blocking time\n"
        "## C...average send blocking time\n"
        "## D...median send blocking time\n"
        "## E...maximum send blocking time\n"
        "##\n"
        "## H...minimum RTT/2\n"
        "## I...average RTT/2\n"
        "## J...median RTT/2\n"
        "## K...maximum RTT/2\n"
        "##\n"
        "## N...minimum throughput [Mbit/sec]\n"
        "## O...average throughput [Mbit/sec]\n"
        "## P...median throughput [Mbit/sec]\n"
        "## Q...maximum throughput [Mbit/sec]\n"
        "##\n"
        "## A  -  B  C  S  D - G  H  T  I - L  M  U  N\n",
        NG_VERSION,
        g_options.mode);

      printf("%s", txtbuf);
      
    } else
    // if not verbose - short output
    {
      // no header ...
    }

  }

  /* outer test loop, sets data_size and test_count */
  for (data_size = g_options.min_datasize; !g_stop_tests && data_size > 0;
       get_next_testparams(&data_size, &test_count, &g_options, module)) {

    // the benchmark results
    std::vector<double> tblock, trtt;

    ng_info(NG_VLEV1, "Testing %d times with %d bytes:", test_count, data_size);
    // if we print dots ...
    if ( (rank==0) && (NG_VLEV1 & g_options.verbose) ) {
      printf("# ");
    }
    
    for (long test=-1 /* 1 warmup */; (test < test_count) && !g_stop_tests; test++) {

	    if ( rank == 0 && (NG_VLEV1 & g_options.verbose) && ( test_count < NG_DOT_COUNT || !(test % (int)(test_count / NG_DOT_COUNT)) )) {
	      printf(".");
	      fflush(stdout);
	    }

      if (g_options.mpi_opts->worldrank == 0) {
        /* client role - send to server */
        unsigned long long tibl, tirtt;
        HRT_TIMESTAMP_T t[3];
        
        HRT_GET_TIMESTAMP(t[0]);
        
        for(i = 1; i<g_options.mpi_opts->worldsize; i++) {
          /* TODO: send from one buffer ... is this a problem? */
          module->isendto(i, buffer, data_size, &sreqs[i-1]);
        }
        
        HRT_GET_TIMESTAMP(t[1]);

        
        for(i = 1; i<g_options.mpi_opts->worldsize; i++) {
          /* TODO: recv to one buffer ... is this a problem? */
          module->irecvfrom(i, buffer, data_size, &rreqs[i-1]);
        }
        /* loop until all requests done */
        while(1) {
          int ctr = 0;
          for(i = 1; i<g_options.mpi_opts->worldsize; i++) {
            ctr += module->test(&sreqs[i-1]);
            ctr += module->test(&rreqs[i-1]);
          }
          /* all returned 0 :) */
          if(ctr == 0) break;
        }

        HRT_GET_TIMESTAMP(t[2]);

        HRT_GET_ELAPSED_TICKS(t[0],t[1],&tibl);
        HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);

        /* store results */
        if(test >= 0) {
          trtt.push_back(HRT_GET_USEC(tirtt)/2);
          tblock.push_back(HRT_GET_USEC(tibl));
        }
          
      } else {
        /* do the ping pong game */
        module->recvfrom(0, buffer, data_size);
          
        module->sendto(0, buffer, data_size);
        
      }

    /* TODO: introduce internal dissemination barrier */
    MPI_Barrier(MPI_COMM_WORLD);
    }
	  if (rank==0) {
      /* add linebreak if we made dots ... */
      if ( (NG_VLEV1 & g_options.verbose) ) {
        ng_info(NG_VLEV1, "\n");
      }

      /* output statistics - blocking time */
      double tblock_avg = std::accumulate(tblock.begin(), tblock.end(), (double)0)/(double)tblock.size(); 
      double tblock_min = *std::min_element(tblock.begin(), tblock.end()); 
      double tblock_max = *std::max_element(tblock.begin(), tblock.end()); 
      std::vector<double>::iterator nthblock = tblock.begin()+tblock.size()/2;
      std::nth_element(tblock.begin(), nthblock, tblock.end());
      double tblock_med = *nthblock;
      double tblock_var = standard_deviation(tblock.begin(), tblock.end(), tblock_avg);
      int tblock_fail = count_range(tblock.begin(), tblock.end(), tblock_avg-tblock_var*2, tblock_avg+tblock_var*2);

      /* output statistics - rtt time */
      double trtt_avg = std::accumulate(trtt.begin(), trtt.end(), (double)0)/(double)trtt.size(); 
      double trtt_min = *std::min_element(trtt.begin(), trtt.end()); 
      double trtt_max = *std::max_element(trtt.begin(), trtt.end()); 
      std::vector<double>::iterator nthrtt = trtt.begin()+trtt.size()/2;
      std::nth_element(trtt.begin(), nthrtt, trtt.end());
      double trtt_med = *nthrtt;
      double trtt_var = standard_deviation(trtt.begin(), trtt.end(), trtt_avg);
      int trtt_fail = count_range(trtt.begin(), trtt.end(), trtt_avg-trtt_var*2, trtt_avg+trtt_var*2);

      // if very verbose - long output
      if (NG_VLEV2 & g_options.verbose) {
        memset(txtbuf, '\0', 2048);
        snprintf(txtbuf, 2047,
	        "%li - %.2lf %.2lf %.2lf %.2lf (%.2lf %i) - %.2lf %.2lf %.2lf %.2lf (%.2lf %i) - %.2lf %.2lf %.2lf %.2lf\n",
          data_size, /* packet size */
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
          data_size/trtt_max*8, /* minimum bandwidth */
          data_size/trtt_avg*8, /* average bandwidth */
          data_size/trtt_med*8, /* median bandwidth */
          data_size/trtt_min*8 /* maximum bandwidth */
          );
        printf("%s", txtbuf);
        
      } else
      // if verbose - short output
      if (NG_VLEV1 & g_options.verbose) {
        memset(txtbuf, '\0', 2048);
        snprintf(txtbuf, 2047,
	        "%li - %.2lf %.2lf %.2lf %.2lf - %.2lf %.2lf %.2lf %.2lf - %.2lf %.2lf %.2lf %.2lf\n",
          data_size, /* packet size */
          tblock_min, /* minimum send blocking time */
          tblock_avg, /* average send blocking time */
          tblock_med, /* median send blocking time */
          tblock_max, /* maximum send blocking time */ 
          trtt_min, /* minimum RTT time */
          trtt_avg, /* average RTT time */
          trtt_med, /* median RTT time */
          trtt_max, /* maximum RTT time */
          data_size/trtt_max*8, /* minimum bandwidth */
          data_size/trtt_avg*8, /* average bandwidth */
          data_size/trtt_med*8, /* median bandwidth */
          data_size/trtt_min*8 /* maximum bandwidth */
          );
        printf("%s", txtbuf);
        
      } else
      // if not verbose - short output
      {
        memset(txtbuf, '\0', 2048);
        snprintf(txtbuf, 2047,
	        "%li bytes \t -> %.2lf us \t == %.2lf Mbit/s\n",
          data_size, /* packet size */
          trtt_med, /* median RTT time */
          data_size/trtt_med*8 /* median bandwidth */
          );
        printf("%s", txtbuf);
        
      }
    }
  } /* outer test loop */

  /* clean up */
  free(sreqs);
  free(rreqs);
  free(buffer);
  free(txtbuf);
#endif
}

#else 
extern "C" {
int register_pattern_1toN(void) {return 0;}
}
#endif
