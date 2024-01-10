/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include <stdio.h>
#include "cpustat.h"
#include "netgauge.h"

#ifdef HAVE_LIBPAPI
#include <papi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This struct is for managing the statistics - gathering process.
 */
struct ng_statistics {
      int test_count;
      double clock_period;
      unsigned long long *blockcycles;
      unsigned long long *rttcycles;
      
      /** linux kernel cpu usage statistics */
      struct kstat cpustat_before, cpustat_after;
      
      /** counts the # of results in the current round */
      int test_num;
      
      /** the file where the results will be written */
      FILE *outfile;

      /** size of data for currently running round */
      int data_size;
      
      /** a MPI Communicator for all peers collecting statistics */
      MPI_Comm stats_comm;
      int stats_size, stats_rank;

#ifdef HAVE_LIBPAPI
      int papi_event_set;
      long long papi_results[2];
      double *papi_cache_quota_l2;
#endif
};

/**
 * Initializes a statistics struct for later use. This _must_ be
 * called before using the statistics struct with any other
 * functions here.
 */
int ng_create_statistics(int testcount, struct ng_statistics *stats,
			 const char* outfile_name,
			 MPI_Comm stats_comm, int stats_size, int stats_rank);
/**
 * Frees the resources used internally by the statistics object.
 * But the statistics object itself is _not_ freed. Well, we can't
 * even know if it's been allocated dynamically here. So...
 */
void ng_free_statistics(struct ng_statistics *stats);

/**
 * To be called before a test run starts.
 *
 * @param data_size The amount of data transmitted in this test round.
 *   Needed for calulating MBit/s and stuff.
 */
void ng_statistics_start_round(struct ng_statistics *stats, int data_size);

/**
 * Finishes this test round and prints out the gathered data to
 * console, file, wherever wanted. In case of MPI this also collects
 * information from all clients.
 */
void ng_statistics_finish_round(struct ng_statistics *stats);

/**
 * Tell the statistics stuff that a new test is about to start.
 * Currently only needed for the PAPI stuff.
 */ 
void ng_statistics_test_begin(struct ng_statistics *stats);

/**
 * Adds one benchmark result to the currently running round.
 */
void ng_statistics_test_done(struct ng_statistics *stats, 
			     unsigned long long blockcycle,
			     unsigned long long rttcycle);

/* Functions below this line are for internal use only */

int _printresults(FILE *fp,
                  unsigned long long *blockcycles, unsigned long long *rttcycles,
                  struct kstat *stat, double clock_period,
                  int testcount, int packetsize,
#ifdef HAVE_LIBPAPI
double *papi_cache_quota_l2,
#endif
                  int client_rank, int client_size, MPI_Comm client_comm);

int _print_full_results(char *full_output_file,
                        unsigned long long *blockcycles, unsigned long long *rttcycles,
                        double clock_period,
                        int testcount, int packetsize);

#ifdef __cplusplus
}
#endif
#endif
