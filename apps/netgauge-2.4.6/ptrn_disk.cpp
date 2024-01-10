/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@illinois.edu>
 *
 */

#include "netgauge.h"
#ifdef NG_PTRN_DISK
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "MersenneTwister.h"
#include "hrtimer/hrtimer.h"
#include "ptrn_disk_cmdline.h"
#include "ng_tools.hpp"
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h> // nasty linux header needes for ioctl to get device size ...

#include <vector>
#include <time.h>
#include <algorithm>
#include <numeric>


extern "C" {

extern struct ng_options g_options;

/* internal function prototypes */
static void disk_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_disk = {
   pattern_disk.name = "disk",
   pattern_disk.desc = "measures disk performance",
   pattern_disk.flags = 0,
   pattern_disk.do_benchmarks = disk_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_disk() {
   ng_register_pattern(&pattern_disk);
   return 0;
}


static void disk_do_benchmarks(struct ng_module *module) {

  int size = g_options.mpi_opts->worldsize;
  if(size > 1) ng_abort("this pattern only supports a single rank!\n");

  std::vector<double> ts;
  std::vector<unsigned long> bnum;

  /** number of times to test the current datasize */
  long test_count = g_options.testcount;

  /** counts up to test_count */
  int test_round = 0;

  char fname[1024];
  strncpy(fname, g_options.output_file, 1023);
	FILE* outputfd = open_output_file(fname);
  write_host_information(outputfd);

  //parse cmdline arguments
  struct ptrn_disk_cmd_struct args_info;
  //printf("The string I got: %s\n", g_options.ptrnopts);
  if (ptrn_disk_parser_string(g_options.ptrnopts, &args_info, "netgauge") != 0) {
    exit(EXIT_FAILURE);
  }

  if(!args_info.device_given) {
    printf("need device, e.g., -d /dev/sda to test! Aborting.\n");
    exit(1);
  }
  printf("# Opening device %s\n", args_info.device_arg);

  mode_t mode = O_RDONLY;
  if(args_info.write_given) {
    printf("\n# ***** ATTENTION, I AM GOING TO ERASE ALL DATA ON %s \n", args_info.device_arg);
    printf("# ***** I will wait 20 seconds now for you to decide \n");
    printf("# ***** if you *really want to erase* %s \n", args_info.device_arg);
    fflush(stdout);

    for(int i=20; i>=0; --i) {
      sleep(1);
      printf(" %i", i);
      fflush(stdout);
    }

    printf("\n# You wanted it so ... \n\n");
    mode = O_WRONLY;
  }

  int fd = open(args_info.device_arg, mode);
  if(fd < 0) {
    perror("open fd");
    ng_abort("");
  }
  printf("# got fd=%i\n", fd);

  unsigned long blks;
  int ret = ioctl(fd, BLKGETSIZE, &blks);
  printf("# %s has %i blocks (%i MiB)\n", args_info.device_arg, blks /* sectors of 512 bytes */, blks/2048);
  fprintf(outputfd, "# %s has %i blocks (%i MiB)\n", args_info.device_arg, blks /* sectors of 512 bytes */, blks/2048);

  int bs = args_info.bs_arg*1024;
  // this cast seems a bit dumb but some weird Linux on Power6 generates
  // an overflow if we don't do it (runs in 32-bit mode :-/)
  int totblks = (unsigned long long)blks*512/bs;

  if(args_info.write_given) {
    printf("# writing %i random blocks of size and granularity %i kiB (%i total)\n", test_count, bs/1024, totblks);
    fprintf(outputfd, "# writing %i random blocks of size and granularity %i kiB (%i total)\n", test_count, bs/1024, totblks);
  } else { 
    printf("# reading %i random blocks of size and granularity %i kiB (%i total)\n", test_count, bs/1024, totblks);
    fprintf(outputfd, "# reading %i random blocks of size and granularity %i kiB (%i total)\n", test_count, bs/1024, totblks);
  }

  char *buffer = (char*)malloc(bs);
  if(!buffer) printf("could not allocate buffer\n");

  MTRand rand;

  /* Inner test loop */
  for (int test = 1; test < test_count; test++) {
	
    /* TODO: add cool abstract dot interface ;) */
	  if( (NG_VLEV1 & g_options.verbose) && ( test_count < NG_DOT_COUNT || !(test % (int)(test_count / NG_DOT_COUNT)) )) {
	    printf(".");
	    fflush(stdout);
	  }

    unsigned long randblock = (unsigned long)floor(rand.randDblExc(totblks));
    bnum.push_back(randblock);

    /* do the client stuff ... take time, send message, wait for
     * reply and take time  ... simple ping-pong scheme */
    HRT_TIMESTAMP_T t[3];  
    unsigned long long tirtt;
  
    HRT_GET_TIMESTAMP(t[0]);
    
    // seek to random block!
    //printf("seeking to block %lu (offset: %lu)\n", block, bs * block);
    off64_t off = lseek64(fd, bs * randblock, SEEK_SET);
    if(off == (off64_t)-1) {
      perror("lseek64");
      ng_abort("");
    }

    if(args_info.write_given) {
      // write block
      ret = write(fd, buffer, bs);
      if(ret < 0) {
        perror("write");
        ng_abort("");
      }
    } else { 
      // read block
      ret = read(fd, buffer, bs);
      if(ret < 0) {
        perror("read");
        ng_abort("");
      }
    }

    HRT_GET_TIMESTAMP(t[2]);

    HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
    if(test >= 0) ts.push_back(HRT_GET_USEC(tirtt));

  }	/* end inner test loop */

	if(NG_VLEV1 & g_options.verbose) printf("\n");
    
  /* compute statistics - read time */
  double ts_avg = std::accumulate(ts.begin(), ts.end(), (double)0)/(double)ts.size(); 
  double ts_min = *min_element(ts.begin(), ts.end()); 
  double ts_max = *max_element(ts.begin(), ts.end()); 
  std::vector<double>::iterator nthr = ts.begin()+ts.size()/2;
  std::nth_element(ts.begin(), nthr, ts.end());
  double ts_med = *nthr;
  double ts_var = standard_deviation(ts.begin(), ts.end(), ts_avg);
  int ts_fail = count_range(ts.begin(), ts.end(), ts_avg-ts_var*2, ts_avg+ts_var*2);


  fprintf(outputfd, "# \n# sample latency [us] blockno\n");
  for(int i=0; i<ts.size(); ++i) {
    fprintf(outputfd, "%i %f %lu\n", i, ts[i], bnum[i]);
  }

  printf("bs: %i kiB min: %.2f ms avg: %.2f med: %.2f max: %.2f std-var: %.2f outliers: %i\n", bs/1024, ts_min/1000, ts_avg/1000, ts_med/1000, ts_max/1000, ts_var/1000, ts_fail);
  fprintf(outputfd, "# bs: %i kiB min: %.2f ms avg: %.2f med: %.2f max: %.2f std-var: %.2f outliers: %i\n", bs/1024, ts_min/1000, ts_avg/1000, ts_med/1000, ts_max/1000, ts_var/1000, ts_fail);
  
  fclose(outputfd);

// shutdown:
    
}

} /* extern C */

#else
extern "C" {
int register_pattern_disk(void) {return 0;};
}
#endif
