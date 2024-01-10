/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "statistics.h"
#include <assert.h>
#include "fullresult.h"
#include "netgauge.h"
#include <string.h>

#ifdef HAVE_LIBPAPI
#include <papi.h>
#endif

extern struct ng_options g_options;

void ng_statistics_start_round(struct ng_statistics *stats, int data_size) {
   get_cpustatus(&stats->cpustat_before);
   stats->test_num = 0;
   stats->data_size = data_size;
   
}

void ng_statistics_test_done(struct ng_statistics *stats,
			     unsigned long long blockcycle,
			     unsigned long long rttcycle) {
   
   stats->blockcycles[stats->test_num] = blockcycle;
   stats->rttcycles[stats->test_num] = rttcycle;
   stats->test_num++;
#ifdef HAVE_LIBPAPI
   PAPI_stop(stats->papi_event_set, stats->papi_results);
   stats->papi_cache_quota_l2[stats->test_num] = (double)stats->papi_results[1] / stats->papi_results[0];
#endif
}

void ng_statistics_finish_round(struct ng_statistics *stats) {
   
   get_cpustatus(&stats->cpustat_after);
   calc_cpustatus(&stats->cpustat_before, &stats->cpustat_after);
   
   _printresults(stats->outfile, 
                 stats->blockcycles,
                 stats->rttcycles,
                 &stats->cpustat_before,
                 stats->clock_period,
                 stats->test_num,
                 stats->data_size,
#ifdef HAVE_LIBPAPI
                 stats->papi_cache_quota_l2,
#endif
                 stats->stats_rank,
                 stats->stats_size,
                 stats->stats_comm
      );
/*    if(options->full_output_file) */
/*       print_full_results(options->full_output_file, */
/* 			 blockcycles, */
/* 			 rttcycles, */
/* 			 clock_period, */
/* 			 /\* test_count *\/ test, */
/* 			 data_size */
/* 	 ); */
   
}

/**
 * - if outfile_name is NULL this peer won't write out statistics
 * - only ONE node writes statistics
 */
int ng_create_statistics(int testcount, struct ng_statistics *stats, 
			 const char* outfile_name, MPI_Comm stats_comm, int stats_size, int stats_rank) {
   
#ifdef HAVE_LIBPAPI
   int retval;
#endif
   stats->test_count = testcount;

   /* get result buffer memory */
   ng_info(NG_VLEV2, "Allocating %d bytes result buffers",
	   g_options.testcount * (sizeof(*stats->blockcycles) + 
				  sizeof(*stats->rttcycles)));

   stats->blockcycles = (long long unsigned int*)malloc(testcount * sizeof(*stats->blockcycles));
   stats->rttcycles = (long long unsigned int*)malloc(testcount * sizeof(*stats->rttcycles));

#ifdef HAVE_LIBPAPI
   stats->papi_cache_quota_l2 = malloc(testcount * sizeof(double));
#endif
   
   if (!stats->blockcycles || !stats->rttcycles) {
      ng_perror("Failed to allocate %d bytes for the test results",
		g_options.testcount * (sizeof(*stats->blockcycles) + 
				      sizeof(*stats->rttcycles)));
      
      if (stats->blockcycles) free(stats->blockcycles);
      if (stats->rttcycles) free(stats->rttcycles);
      
      return -1;
   }

   /* we have already usec - this clock_period is legacy and should go
    * away ... */
   stats->clock_period = 1/1e12;
   
   /* open the output file, if needed */
   if (outfile_name != NULL) {
      ng_info(NG_VLEV2, "Opening output file %s", outfile_name);
      stats->outfile = fopen(outfile_name, "w");
      
      if (!stats->outfile) {
	 ng_perror("Failed to open output file %s for writing", outfile_name);
	 return -2;
      }
   } else {
      stats->outfile = NULL;
   }

   stats->data_size = 0;
   stats->stats_comm = stats_comm;
   stats->stats_size = stats_size;
   stats->stats_rank = stats_rank;

#ifdef HAVE_LIBPAPI
   retval = PAPI_library_init(PAPI_VER_CURRENT);

   if (retval != PAPI_VER_CURRENT && retval > 0) {
      fprintf(stderr,"PAPI library version mismatch!\n");
      exit(1);
   }


   stats->papi_event_set = PAPI_NULL;
   PAPI_create_eventset(&(stats->papi_event_set));
   PAPI_add_event(stats->papi_event_set, PAPI_L2_TCA);
   if (PAPI_OK != PAPI_add_event(stats->papi_event_set, PAPI_L2_TCM)) {
      ng_error("There is a problem with PAPI.");
      return 1;
   }
#endif
   
   return 0;
}

void ng_free_statistics(struct ng_statistics *stats) {
   free(stats->blockcycles);
   free(stats->rttcycles);
   if (stats->outfile != NULL) fclose(stats->outfile);
}

void ng_statistics_test_begin(struct ng_statistics *stats) {
#ifdef HAVE_LIBPAPI
  PAPI_start(stats->papi_event_set);
#endif  
}

/**
 * aggregate the results of all clients
 */
int _printresults(FILE *fp,
                  unsigned long long *blockcycles, unsigned long long *rttcycles,
                  struct kstat *stat, double clock_period,
                  int testcount, int packetsize,
#ifdef HAVE_LIBPAPI
                  double *papi_cache_quota_l2,
#endif
                  int client_rank, int client_size, MPI_Comm client_comm) {
   
  unsigned long long minblockcycles = *blockcycles,
    maxblockcycles = 0,
    avgblockcycles = 0,
    medblockcycles = 0,
    minrttcycles   = *rttcycles,
    maxrttcycles   = 0,
    avgrttcycles   = 0,
    medrttcycles   = 0;
  
  double minblocktime   = 0,
    maxblocktime   = 0,
    avgblocktime   = 0,
    medblocktime   = 0,
    minrtttime     = 0,
    maxrtttime     = 0,
    avgrtttime     = 0,
    medrtttime     = 0,
    sys_time       = (double) stat->sy, /*  initialize the cpu statistics */
    io_time        = (double) stat->wa,
    hard_int_time  = (double) stat->hi,
    soft_int_time  = (double) stat->si;
 
  double max_cache_quota_l2 = 0.0f;
  
  double varblock = 0, varrtt   = 0, tmpbuf;
   
  unsigned int rttfail = 0, blockfail = 0;
   
  static int first_time = 1;
  int i;
  char *buffer;
   
  buffer = (char *)calloc(1024, sizeof(char));
  if (buffer == NULL) {
    ng_error("Could not allocate 1024 byte for output buffer");
    return 1;
  }
  
  for(i = 0; i < testcount; i++) {
    minblockcycles  = ng_min(blockcycles[i], minblockcycles);
    maxblockcycles  = ng_max(blockcycles[i], maxblockcycles);
    minrttcycles    = ng_min(rttcycles[i], minrttcycles);
    maxrttcycles    = ng_max(rttcycles[i], maxrttcycles);
    avgblockcycles += blockcycles[i];
    avgrttcycles   += rttcycles[i];
#ifdef HAVE_LIBPAPI
    max_cache_quota_l2 = ng_max(papi_cache_quota_l2[i], max_cache_quota_l2);
#endif
  }
   
  avgblockcycles = avgblockcycles / testcount;
  avgrttcycles   = avgrttcycles   / testcount;
  
  medblockcycles = median(blockcycles, testcount);
  medrttcycles = median(rttcycles, testcount);
   
  /* calculate standard deviation (Standardabweichung :)
   * Varianz: D^2X = 1/(testcount - 1) * sum((x - avg(x))^2)
   * Standardabweichung = sqrt(D^2X)
   */
  for(i = 0; i < testcount; i++) {
    varblock += (blockcycles[i] - avgblockcycles)  * (blockcycles[i] - avgblockcycles) * clock_period * 1000000 * clock_period * 1000000;
    varblock /= packetsize;
    varrtt   += (rttcycles[i] - avgrttcycles) * (rttcycles[i] - avgrttcycles) * clock_period * 1000000 * clock_period * 1000000;
  }
  varblock = sqrt(varblock / (testcount - 1));
  varrtt   = sqrt(varrtt   / (testcount - 1));
   
  /* get amount of testvalues bigger than 2 times standard deviation */
  for(i = 0; i < testcount; i++) {
    if ( blockcycles[i] > (2*avgblockcycles) )
	  blockfail++;
    if ( rttcycles[i] > (2*avgrttcycles) )
	  rttfail++;
  }
   
  /* calculate actual time values from the cpu cycle values
   * (times are then in microseconds [usec])
   */
  minblocktime = (double)(minblockcycles * clock_period * 1000000);
  avgblocktime = (double)(avgblockcycles * clock_period * 1000000);
  medblocktime = (double)(medblockcycles * clock_period * 1000000);
  maxblocktime = (double)(maxblockcycles * clock_period * 1000000);
  minrtttime = (double)(minrttcycles * clock_period * 1000000);
  avgrtttime = (double)(avgrttcycles * clock_period * 1000000);
  medrtttime = (double)(medrttcycles * clock_period * 1000000);
  maxrtttime = (double)(maxrttcycles * clock_period * 1000000);
   
#ifdef NG_MPI
  assert(g_options.mpi>0);
  if(client_size > 1) {

  /* collect minblocktime from all clients */
  if (MPI_Reduce(&minblocktime, &tmpbuf, 1, MPI_DOUBLE, MPI_MIN, 0, client_comm) != MPI_SUCCESS) {
    ng_error("Could not do MPI_Reduce for minblocktime");
	  return 1;
  }
  minblocktime=tmpbuf;

  /* collect avgblocktime from all clients */
  if (MPI_Reduce(&avgblocktime, &tmpbuf, 1, MPI_DOUBLE, MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
    ng_error("Could not do MPI_Reduce for avgblocktime");
    return 1;
  } 
  avgblocktime = tmpbuf/client_size;
  
     
     if (MPI_Reduce(&maxblocktime, &tmpbuf, 1, MPI_DOUBLE, 
	      MPI_MAX, 0, client_comm) != MPI_SUCCESS) {
	ng_error("Could not do MPI_Reduce for maxblocktime");
	return 1;
     } else if (client_rank == 0) {
	maxblocktime = tmpbuf;
     }
     
     if (MPI_Reduce(&varblock, &tmpbuf, 1, MPI_DOUBLE, 
	      MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	ng_error("Could not do MPI_Reduce for varblock");
	return 1;
     } else if (client_rank == 0) {
	varblock = tmpbuf;
	varblock /= client_size;
     }
     
     if (MPI_Reduce(&blockfail, &tmpbuf, 1, MPI_UNSIGNED, 
	      MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	ng_error("Could not do MPI_Reduce for blockfail");
	return 1;
     } else if (client_rank == 0) {
	blockfail = tmpbuf;
	blockfail /= client_size;
     }
     
     /* distribute the round trip times */
     if (MPI_Reduce(&minrtttime, &tmpbuf, 1, MPI_DOUBLE, 
	      MPI_MIN, 0, client_comm) != MPI_SUCCESS) {
	ng_error("Could not do MPI_Reduce for minrtttime");
	 return 1;
      } else if (client_rank == 0) {
	 minrtttime = tmpbuf;
      }
      
      if (MPI_Reduce(&avgrtttime, &tmpbuf, 1, MPI_DOUBLE, 
		     MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	 ng_error("Could not do MPI_Reduce for avgrtttime");
	 return 1;
      } else if (client_rank == 0) {
	 avgrtttime = tmpbuf;
	 avgrtttime /= client_size;
      }
      
      if (MPI_Reduce(&maxrtttime, &tmpbuf, 1, MPI_DOUBLE, 
		     MPI_MAX, 0, client_comm) != MPI_SUCCESS) {
	 ng_error("Could not do MPI_Reduce for maxrtttime");
	 return 1;
      } else if (client_rank == 0) {
	 maxrtttime = tmpbuf;
      }
      
      if (MPI_Reduce(&varrtt, &tmpbuf, 1, MPI_DOUBLE, 
		     MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	 ng_error("Could not do MPI_Reduce for varrtt");
	 return 1;
      } else if (client_rank == 0) {
	 varrtt = tmpbuf;
	 varrtt /= client_size;
      }
      
      if (MPI_Reduce(&rttfail, &tmpbuf, 1, MPI_UNSIGNED, 
		     MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	 ng_error("Could not do MPI_Reduce for rttfail");
	 return 1;
      } else if (client_rank == 0) {
	 rttfail = tmpbuf;
	 rttfail /= client_size;
      }
      
      /* distribute the cpu statistics
       * the variables are promoted to double values because MPI does not know an
       * unsigned long long type (as they are) and because there may be
       * computational errors when doing the division for the average value on
       * "integer" types
       */
      if (MPI_Reduce(&sys_time, &tmpbuf, 1, MPI_DOUBLE, 
		     MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	 ng_error("Could not do MPI_Reduce for sys_time");
	 return 1;
      } else if (client_rank == 0) {
	 sys_time = tmpbuf;
	 sys_time /= client_size;
      }
      
      if (MPI_Reduce(&io_time, &tmpbuf, 1, MPI_DOUBLE, 
		     MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	 ng_error("Could not do MPI_Reduce for io_time");
	 return 1;
      } else if (client_rank == 0) {
	 io_time = tmpbuf;
	 io_time /= client_size;
      }
      
      if (MPI_Reduce(&hard_int_time, &tmpbuf, 1, MPI_DOUBLE, 
		     MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	 ng_error("Could not do MPI_Reduce for hard_int_time");
	 return 1;
      } else if (client_rank == 0) {
	 hard_int_time = tmpbuf;
	 hard_int_time /= client_size;
      }
      
      if (MPI_Reduce(&soft_int_time, &tmpbuf, 1, MPI_DOUBLE, 
		     MPI_SUM, 0, client_comm) != MPI_SUCCESS) {
	 ng_error("Could not do MPI_Reduce for soft_int_time");
	 return 1;
      } else if (client_rank == 0) {
	 soft_int_time = tmpbuf;
	 soft_int_time /= client_size;
      }
      
   } /* end if ( (g_options.mpi > 0) && (client_size > 1) ) */
#endif

   
   if (client_rank != 0) return 0;

   /* prepare output buffer */
    /* only rank 0 deals with the buffer and log file */
    memset(buffer, '\0', 1024);
    
    /* write some explanations of the output to the output file */
    if (first_time == 1) {
      buffer = (char *)realloc(buffer, 2048 * sizeof(char));
      if (buffer == NULL) {
        ng_error("Could not (re)allocate 2048 byte for output buffer");
        return 1;
      }
      memset(buffer, '\0', 2048);
	 
      snprintf(buffer, 2047,
	      "## Netgauge v%s - mode %s - %d processes\n"
	      "##\n"
	      "## time-measurement-1:\n"
        "##    time for the mode's respective send operation to return to its caller [usec]\n"
        "##    (approximate time for a blocking system call)\n"
        "##\n"
        "## time-measurement-2:\n"
        "##    half of the time needed for sending the data and receiving them back [usec]\n"
        "##    (approximate time for sending data in one direction)\n"
        "##\n"
        "## A...packet size [byte]\n"
        "##\n"
        "## B...minimum blocking time\n"
        "## C...average blocking time\n"
        "## S...median blocking time\n"
        "## D...maximum blocking time\n"
        "## E...standard deviation for blocking time\n"
        "## F...number of blocking time values, that were bigger than 2*avg.\n"
        "##\n"
        "## G...minimum RTT/2\n"
        "## H...average RTT/@\n"
        "## T...median RTT/2\n"
        "## I...maximum RTT/2\n"
        "## J...standard deviation for RTT/2\n"
        "## K...number of RTT/2 values, that were bigger than 2*avg.\n"
        "##\n"
        "## L...minimum throughput [Mbit/sec]\n"
        "## M...average throughput [Mbit/sec]\n"
        "## U...median throughput [Mbit/sec]\n"
        "## N...maximum throughput [Mbit/sec]\n"
        "##\n"
        "## O...time spent for calculating (system time) [User Hz]\n"
        "## P...time spent waiting for IO                [User Hz]\n"
        "## Q...time spent in hardware interrupts        [User Hz]\n"
        "## R...time spent in software interrupts        [User Hz]\n"
        "## XX best l2 cache quota\n"
        "##\n"
        "## A  -  B  C  S  D  E  (F) - G  H  T  I  J  (K)  -  L  M U N  -  O  P  Q  R -- XX\n",
        NG_VERSION,
        g_options.mode,
        client_size * 2);
	 
      fputs(buffer, fp);
      fflush(fp);
	 
      printf("## See the output file '%s' for a description of all values.\n", g_options.output_file);
      memset(buffer, '\0', 2048);
      first_time = 0;
    }
    
    snprintf(buffer, 1023,
	     "%d - %.2f %.2f %.2f %.2f %.2f (%d) - %.2f %.2f %.2f %.2f %.2f (%d) - %.2f %.2f %.2f %.2f - %.0f %.0f %.0f %.0f -- %.5f\n",
	     packetsize,     /* packet size [byte] */
	     minblocktime,   /* minimum time spent waiting in blocking send function [usec] */
	     avgblocktime,   /* average time spent waiting in blocking send function [usec] */
	     medblocktime,	/* median time spent waiting in blocking send function [usec] */
	     maxblocktime,   /* maximum time spent waiting in blocking send function [usec] */
	     varblock,       /* standard deviation for the blocking time [usec] */
	     blockfail,      /* number of blocking times bigger than 2 * avg. of blocking time */
	     minrtttime / 2, /* minimum time for sending data in one direction [usec] */
	     avgrtttime / 2, /* average time for sending data in one direction [usec] */
	     medrtttime / 2,		/* median time for sending data in one direction [usec] */
	     maxrtttime / 2, /* maximum time for sending data in one direction [usec] */
	     varrtt,         /* standard deviation for the send time [usec] */
	     rttfail,        /* number of send times bigger than 2 * avg. of send time */
	     packetsize/(maxrtttime / 2)*8, /* minimum throughput [bits/usec == Mbit/sec] */
	     packetsize/(avgrtttime / 2)*8, /* average throughput [bits/usec == Mbit/sec] */
	     packetsize/(medrtttime / 2)*8, /* median throughput [bits/usec == Mbit/sec] */
	     packetsize/(minrtttime / 2)*8, /* maximum throughput [bits/usec == Mbit/sec] */
	     sys_time,       /* time spent for calculating (system time) [User Hz] */
	     io_time,        /* time spent waiting for IO [User Hz] */
	     hard_int_time,  /* time spent in hardware interrupts [User Hz] */
	     soft_int_time, /* time spent in software interrupts [User Hz] */
             max_cache_quota_l2);
    
    /* print results to console */
    printf("%s", buffer);
    
    /* write results to output file */
    fputs(buffer, fp);
    fflush(fp);
   
   /* free allocated string buffer memory */
   if (buffer) free(buffer);
   
   return 0;
}


