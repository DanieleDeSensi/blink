/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_NTO1
#include "hrtimer/hrtimer.h"
#include <vector>
#include "statistics.h"
#include <string.h>
#include <algorithm>
#include "ng_tools.hpp"

/* internal function prototypes & extern stuff */
static void Nto1_do_benchmarks(struct ng_module *module);
extern struct ng_options g_options;

/**
 * one-to-many communication pattern registration
 * structure
 */
static struct ng_comm_pattern pattern_Nto1 = {
   pattern_Nto1.name = "Nto1",
   pattern_Nto1.desc = "measures bandwith in a N:1 congestion situation",
   pattern_Nto1.flags = NG_PTRN_NB,
   pattern_Nto1.do_benchmarks = Nto1_do_benchmarks
};

/**
 * registers this comm. pattern with the main prog.,
 * but only if we use MPI
 */
int register_pattern_Nto1() {
  /* there's no use for this mode without MPI */
#if NG_MPI
  ng_register_pattern(&pattern_Nto1);
#endif 
  return 0;
}

void Nto1_do_benchmarks(struct ng_module *module) {
#if NG_MPI
  /** current data size for transmission tests */
  long data_size;

  /** # times to run with current data_size */
  long test_count = g_options.testcount;
  
  /** send/receive buffer */
  void *buffer;

  /** enumerating things is such a fun! */
  int i;

  /* server role - receive from clients */
  NG_Request *reqs = NULL;

  /* measurement data ... we need to send it from rank 1 to rank 0 since
   * rank 0 doe all the I/O */
  double results[2];
    
  /* allocate send / recv buffer */
  ng_info(NG_VLEV2, "Allocating %d bytes data buffer", g_options.max_datasize + module->headerlen);
  NG_MALLOC(module, void*, g_options.max_datasize + module->headerlen, buffer);

  int rank = g_options.mpi_opts->worldrank;

  /* initialize the statistics stuff - rank 1 does the whole measurement */
  if(rank == 0) {
    /* allocate Request structure at server */
    reqs = (NG_Request*)malloc(sizeof(NG_Request) * (g_options.mpi_opts->worldsize-1) );
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
  for (data_size = NG_START_PACKET_SIZE; !g_stop_tests && data_size > 0;
       get_next_testparams(&data_size, &test_count, &g_options, module)) {
    if(data_size == -1) goto shutdown;

    /* TODO: introduce internal dissemination barrier */
    ng_info(NG_VLEV1, "Barrier and testing %d times with %d bytes:", test_count, data_size);
    // if we print dots ...
    if ( (rank==0) && (NG_VLEV1 & g_options.verbose) ) {
      printf("# ");
    }
    
    // the benchmark results
    std::vector<double> tblock, trtt;
    
    for (long test=-1 /* 1 warmup */; (test < test_count) && !g_stop_tests; test++) {
      /* TODO: introduce internal dissemination barrier */
      MPI_Barrier(MPI_COMM_WORLD);

	    if ( rank == 0 && (NG_VLEV1 & g_options.verbose) && ( test_count < NG_DOT_COUNT || !(test % (int)(test_count / NG_DOT_COUNT)) )) {
	      printf(".");
	      fflush(stdout);
	    }

      if (g_options.mpi_opts->worldrank == 0) {
        for(i = 1; i<g_options.mpi_opts->worldsize; i++) {
          /* TODO: all receive in one buffer ... is this a problem? */
          module->irecvfrom(i, buffer, data_size, &reqs[i-1]);
        }
        /* loop until all requests done */
        while(1) {
          int ctr;
          ctr = 0;
          for(i = 1; i<g_options.mpi_opts->worldsize; i++) {
            ctr += module->test(&reqs[i-1]);
          }
          /* all returned 0 :) */
          if(ctr == 0) break;
        }
        /* send ack to rank 1 */
        module->sendto(1, buffer, 1);

        /* receive measurement data from rank 1 */
        MPI_Recv(&results, 2, MPI_DOUBLE, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        /* calculate results */
        if(test >= 0) {
          trtt.push_back(results[1]/2);
          tblock.push_back(results[0]);
        }
          
      } else {
        /* client role - send to server */
        unsigned long long tibl, tirtt;
        HRT_TIMESTAMP_T t[3];
        
        if(g_options.mpi_opts->worldrank == 1) {
          /* get start time - timer is *ONLY* meaningful on rank 1 !! */
          HRT_GET_TIMESTAMP(t[0]);
        }
            
        module->sendto(0, buffer, data_size);

        /* get after-sending time */
        if(g_options.mpi_opts->worldrank == 1) HRT_GET_TIMESTAMP(t[1]);
        
        /* rank 1 receives ack from server */
        if(g_options.mpi_opts->worldrank == 1) {
          module->recvfrom(0,buffer,1);
          /* get after-receiving time */
          HRT_GET_TIMESTAMP(t[2]);

          HRT_GET_ELAPSED_TICKS(t[0],t[1],&tibl);
          HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);

          /* send results to rank 0 */
          results[0] = HRT_GET_USEC(tibl);
          results[1] = HRT_GET_USEC(tirtt);
          MPI_Send(&results, 2, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
        }
      }
    }

    /* add linebreak if we made dots ... */
    if ( (NG_VLEV1 & g_options.verbose) ) {
      ng_info(NG_VLEV1, "\n");
    }

    if(rank==0) {
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

 shutdown:
  /* clean up */
  if (buffer) free(buffer);
#endif
}

#else 
extern "C" {
int register_pattern_Nto1(void) {return 0;}
}
#endif
