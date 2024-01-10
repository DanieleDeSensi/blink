/*
 * Copyright (c) 2010 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Abhishek Kulkarni <adkulkar@osl.iu.edu>
 *
 */

#include "netgauge.h"

#ifdef NG_PTRN_ONE_ONE_ALL
#include "hrtimer/hrtimer.h"
#include <time.h>
#include <numeric>
#include <vector>
#include <algorithm>
#include "fullresult.h"
#include "statistics.h"
#include "ng_tools.hpp"

#define MAX_ROUNDS 1024

extern "C" {

extern struct ng_options g_options;

static void one_one_all_do_benchmarks(struct ng_module *module);

static struct ng_comm_pattern pattern_one_one_all = {
  pattern_one_one_all.name = "one_one_all",
  pattern_one_one_all.desc = "measures ping-pong latency between all nodes",
  pattern_one_one_all.flags = 0,
  pattern_one_one_all.do_benchmarks = one_one_all_do_benchmarks
};

int register_pattern_one_one_all() {
  ng_register_pattern(&pattern_one_one_all);
  return 0;
}

static void one_one_all_do_benchmarks(struct ng_module *module) {
  /** for collecting statistics */
  struct ng_statistics statistics;

  /** number of times to test with invariant results */
  long test_count = g_options.testcount;

  /** counts the no. of tests for which the op hasn't fallen below min  */
  int test_round = 0;

  /** how long does the test run? */
  time_t test_time, cur_test_time;

  /** number of tests run */
  int ovr_tests=0, ovr_bytes=0;

  /* get needed data buffer memory */
  char buf = '\0';

  /** currently tested packet size */
  long data_size = sizeof(buf);

  if (data_size != 1)
    ng_info(NG_VNORM, "using data size of %d byte(s)", sizeof(buf));

  int rank = g_options.mpi_opts->worldrank;
  int size = g_options.mpi_opts->worldsize; 

  /* buffer for header ... */
  char* txtbuf = (char *)calloc(2048, sizeof(char));
  if (!txtbuf) {
    ng_error("Could not (re)allocate 2048 bytes for output buffer");
    ng_exit(10);
  }

  /* file to write results in */
  FILE *outputfd;

  for (int boss = 0; boss < size; boss++) {
    if(rank == boss) {
      char fname[1024];
      strncpy(fname, g_options.output_file, 1023);
      if(size > 2) {
        char suffix[32];
        snprintf(suffix, 31, ".%i", rank);
        strncat(fname, suffix, 1023);
      }
      ng_info(NG_VNORM, "writing data to %s", fname);

      outputfd = open_output_file(fname);
      write_host_information(outputfd);
  
      /* header printing */
      // if very verbose - long output
      if (NG_VLEV2 & g_options.verbose) {
        snprintf(txtbuf, 2047,
                 "## Netgauge v%s - mode %s - rank %i: %i processes\n"
                 "##\n"
                 "## R...rank\n"
                 "## T...tests\n"
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
                 "## R T A  -  B  C  D  E - F  G  H  I - J  K  L  M\n",
                 NG_VERSION, g_options.mode, rank, size);
      } else {
        // if verbose (and by default) - short output
        snprintf(txtbuf, 2047,
                 "## Netgauge v%s - mode %s - rank %i: %i processes\n"
                 "##\n"
                 "## R...rank\n"
                 "## T...tests\n"
                 "##\n"
                 "## B...minimum RTT/2\n"
                 "## C...average RTT/2\n"
                 "## D...median RTT/2\n"
                 "## E...maximum RTT/2\n"
                 "## F...standard deviation for RTT/2 (stddev)\n"
                 "## G...number of RTT/2 values, that were bigger than avg + 2 * stddev.\n"
                 "##\n"
                 "## R T A  -  B  C  D  E  (F G)\n",
                 NG_VERSION, g_options.mode, rank, size);
      }
      // write to the output file
      fprintf(outputfd, "%s", txtbuf);
      fflush(outputfd);
    }

    for (int slave = 0; slave < size; slave++) {
      if (boss == slave) continue;

      // TODO: should be done globally in netgauge.c
      if (rank == 0) {
        float percent = (float)((size*boss)+slave)/(size*size);
        printf("\r"); 
        printf("# Info:\t  (0): %.2f percent done.", percent * 100);
        ng_info(NG_VLEV1, "latency test between %d and %d.", boss, slave);
      }

      if (rank == boss || rank == slave) {
        // the benchmark results
        std::vector<double> tblock, trtt;
        unsigned long long tibl, tirtt;

        /* Test loop
         * - run the tests for data size 1
         * - but only if testtime does not exceed the max. allowed test time
         *   (only if max. test time is not zero)
         */
        test_time = 0;
        test_count = g_options.testcount;
        for (test_round = -1 ; /* 1 warmup test */ test_round < test_count; test_round++) {
          /* first statement to prevent floating exception */

          /* call the appropriate client or server function */
          if (rank == slave) {
            g_options.mpi_opts->partner = boss;
            cur_test_time = time(NULL);
            /* do the server stuff basically a simple receive and send to
             * mirror the data in a ping-pong fashion ... this is not in a
             * subfunction since this may be performance critical. The
             * send and receive functions are also macros */

            // Phase 1: receive data (wait for data - blocking)
            NG_RECV(g_options.mpi_opts->partner, &buf, data_size, module);

            // Phase 2: send data back
            NG_SEND(g_options.mpi_opts->partner, &buf, data_size, module);

            test_time += time(NULL) - cur_test_time;

            NG_RECV(g_options.mpi_opts->partner, &test_count, sizeof(test_count), module);
          } else {
            /* wait some time for the server to get ready */
            usleep(10);
            cur_test_time = time(NULL);
            g_options.mpi_opts->partner = slave;

            /* do the client stuff ... take time, send message, wait for
             * reply and take time  ... simple ping-pong scheme */
            HRT_TIMESTAMP_T t[3];
  
            /* init statistics (TODO: what does this do?) */
            ng_statistics_test_begin(&statistics);

            HRT_GET_TIMESTAMP(t[0]);

            NG_SEND(g_options.mpi_opts->partner, &buf, data_size, module);

            // get after-sending time
            HRT_GET_TIMESTAMP(t[1]);
   
            // phase 2: receive returned data
            NG_RECV(g_options.mpi_opts->partner, &buf, data_size, module);

            HRT_GET_TIMESTAMP(t[2]);
            HRT_GET_ELAPSED_TICKS(t[0],t[1],&tibl);
            HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);

            /* TODO: check received data */
  
            /* calculate results */
  
            if(test_round >= 0) {
              double cur_rtt = (HRT_GET_USEC(tirtt)/2);
              double trtt_min = 0;
              if (trtt.size() > 0) {
                trtt_min = *min_element(trtt.begin(), trtt.end());
              }
              if (cur_rtt < trtt_min) {
                test_count += g_options.testcount;
                ng_info(NG_VLEV2, "Test count increased to %d", test_count);
              }
              tblock.push_back(HRT_GET_USEC(tibl));
              trtt.push_back(cur_rtt);
            }
            test_time += time(NULL) - cur_test_time;

            NG_SEND(g_options.mpi_opts->partner, &test_count, sizeof(test_count), module);
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
            ng_info(NG_VLEV2, "Test stopped at %d tests", test_round);
            break;
          }
        }   /* end test loop */

        if (rank==boss) {
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

          // generate long output for output file
          memset(txtbuf, '\0', 2048);
          if (NG_VLEV2 & g_options.verbose) {
            snprintf(txtbuf, 2047,
                     "%d %d %ld - %.2lf %.2lf %.2lf %.2lf (%.2lf %i) - %.4lf %.2lf %.2lf %.2lf (%.2lf %i)\n",
                     slave,
                     test_round, /* num tests */
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
                     trtt_fail /* how many are bigger than twice the standard deviation? */
                     );
          } else {
            snprintf(txtbuf, 2047,
                     "%d %d - %.4lf %.2lf %.2lf %.2lf (%.2lf %i)\n",
                     slave,
                     test_round, /* num tests */

                     trtt_min, /* minimum RTT time */
                     trtt_avg, /* average RTT time */
                     trtt_med, /* median RTT time */
                     trtt_max, /* maximum RTT time */
        
                     trtt_var, /* standard deviation */
                     trtt_fail /* how many are bigger than twice the standard deviation? */
                     );
          }
          
          fprintf(outputfd, "%s", txtbuf);
          fflush(outputfd);
        }

        /* only a client does the stats stuff */
        //if (!g_options.server)
        //  ng_statistics_finish_round(&statistics);
      }	/* end outer test loop */

      if (size > 2) MPI_Barrier(MPI_COMM_WORLD);
    }

    /* close the output file */
    if (rank==boss) fclose(outputfd);
    if (size > 2) MPI_Barrier(MPI_COMM_WORLD);
  }

  if (txtbuf) free(txtbuf);
}

} /* extern C */

#else
extern "C" {
int register_pattern_one_one_all(void) {return 0;};
}
#endif
