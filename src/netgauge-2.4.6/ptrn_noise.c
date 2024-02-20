/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_NOISE 
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#ifdef HAVE_CPUAFFINITY
#define __USE_GNU
#include <sched.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ptrn_noise_cmdline.h"
#include "ptrn_noise.h"

/* internal function prototypes & extern stuff */
static void noise_do_benchmarks(struct ng_module *module);
//static void noise_getopt(int argc, char **argv);
extern struct ng_options g_options;

static struct ptrn_noise_cmd_struct args_info;

/**
 * one-to-many communication pattern registration
 * structure
 */
static struct ng_comm_pattern pattern_noise = {
   .name = "noise",
   .flags = '\0', /* according to htor this is useless */
   .desc = "measures system noise parameters",
   .do_benchmarks = noise_do_benchmarks
};

/**
 * registers this comm. pattern with the main prog.,
 * but only if we use MPI
 */
int register_pattern_noise() {
  /* currently I am using MPI_COMM_SIZE/RANK, are there netgauge equivalents? */
  ng_register_pattern(&pattern_noise);
  return 0;
}

double get_ticks_per_second() {
	#define NUM_TESTS 10

	HRT_TIMESTAMP_T t1, t2;
	uint64_t res[NUM_TESTS];
	static uint64_t min=0; 
	int count;

	if (min > 0) {return ((double) min);}

	ng_info(NG_VLEV1, "Calibrating timer, this might take some time: ");
	for (count=0; count<NUM_TESTS; count++) {
		HRT_GET_TIMESTAMP(t1);
		sleep(1);
		HRT_GET_TIMESTAMP(t2);
		HRT_GET_ELAPSED_TICKS(t1, t2, &res[count])
	}

	min = res[0];
	for (count=0; count<NUM_TESTS; count++) {
		if (min > res[count]) min = res[count];
	}

	return ((double) min);
}

double tune_workload(int usecs) {

	double desired_ticks;
	double workcount;
	uint64_t min, lmin;

	desired_ticks = (get_ticks_per_second() / pow(10, 6)) * usecs;
	workcount = 1000;
	min = 0;
	INIT_WORKLOAD
	
	ng_info(NG_VLEV1, "# Workload tuning for the desired duration of %i us.", usecs);
	ng_info(NG_VLEV1, "# This should be %f ticks on this system.", desired_ticks);

	while ((min == 0) || ((abs(desired_ticks - min) / desired_ticks) > 0.005 )) {
		HRT_TIMESTAMP_T t1, t2;
		int i, j;
		min = 0;
		
		/* perform test 20 times, use minimum to eliminate spikes */
		for (i=0; i<200; i++) {
			HRT_GET_TIMESTAMP(t1);
			for (j=0; j<workcount; j++) {
  		 		TEN(WORKLOAD);
  			}
			HRT_GET_TIMESTAMP(t2);
			HRT_GET_ELAPSED_TICKS(t1, t2, &lmin)

			if ((min == 0) || (min > lmin)) {
				min = lmin;
			}
		}
		workcount = ceil((desired_ticks / (double) min) * workcount);
	}

	ng_info(NG_VLEV1, "# Finished workload tuning.");
	ng_info(NG_VLEV1, "# It should use %f ticks with a workcount of %f.", (double) min, workcount);

	FINALIZE_WORKLOAD;

	return workcount;
}

void write_benchmark_information(FILE *fd) {
	
	fprintf(fd, "# Benchmark Method: %s\n", args_info.method_arg);
	fprintf(fd, "# Compute-Cycle Duration: %i usec\n", args_info.duration_arg);
	fprintf(fd, "# Number of Cycles/Detours: %i\n", args_info.samples_arg);
}

void perform_fwq(int usecs, int num_runs, uint64_t *results) {
	
	unsigned int i, run;
	double workload_runs;
	HRT_TIMESTAMP_T t1, t2;
	INIT_WORKLOAD;

	// initialize result buffer, so we can make sure we are not
	// using a copy on write page

	for (i=0; i<num_runs; i++) {
		results[i] = (~0x0);
	}
	
	// how often do we have to call the workload macro in order to
	// get a minimal (noisless case) phase duration of "usecs" usec

	workload_runs = tune_workload(usecs);

	// now execute the benchmark, num_run times

	for (run=0; run<num_runs; run++) {
		HRT_GET_TIMESTAMP(t1);
		for (i=0; i<workload_runs; i++) {
			TEN(WORKLOAD);
		}
		HRT_GET_TIMESTAMP(t2);
		HRT_GET_ELAPSED_TICKS(t1, t2, &results[run]);
	}
	FINALIZE_WORKLOAD;
}

void perform_ftq(int usecs, int num_runs, uint64_t *results) {
	
	unsigned int i, run;
	double workload_runs;
	HRT_TIMESTAMP_T t1, t2;
	uint64_t tps;
	INIT_WORKLOAD;

	// initialize result buffer, so we can make sure we are not
	// using a copy on write page

	for (i=0; i<num_runs; i++) {
		results[i] = 0xffffffff;
	}
	
	// how often do we have to call the workload macro in order to
	// get a minimal (noisless case) phase duration of "usecs" usec

	workload_runs = tune_workload(usecs);
	workload_runs /= 1000;
	
	// how many ticks do we have for one cycle
	tps = ceil(((double) get_ticks_per_second() / pow(10, 6)) * usecs);



	// now execute the benchmark, num_run times

	for (run=0; run<num_runs; run++) {
		int iters = 0;
		uint64_t elapsed;
		HRT_GET_TIMESTAMP(t1);
		do {
			iters++;
			for (i=0; i<workload_runs; i++) {
				TEN(WORKLOAD);
			}
			HRT_GET_TIMESTAMP(t2);
			HRT_GET_ELAPSED_TICKS(t1, t2, &elapsed)
		}
		while (elapsed < tps);
		results[run] = iters*workload_runs;
	}
	FINALIZE_WORKLOAD;
}

void perform_selfish(int num_runs, int threshold, uint64_t *results) {
	
	int cnt=0, num_not_smaller = 0;
	HRT_TIMESTAMP_T current, prev, start;
	uint64_t sample = 0;
	uint64_t elapsed, thr, min=(uint64_t)~0;
  int i;

  int p = g_options.mpi_opts->worldsize;
  int r = g_options.mpi_opts->worldrank;

	// we will do a "calibration run" of the detour benchmark to
	// get a reasonable value for the minimal detour time
	// just perform the benchmark and record the minimal detour time until
	// this minimal detour time does not get smaller for 1000 (as defined by NOT_SMALLER) 
	// consecutive runs
	
	#define NOT_SMALLER 100
  #define INNER_TRIES 50

  thr = min*(threshold/100.0);
	while (num_not_smaller < NOT_SMALLER) {
		cnt = 0;

		HRT_GET_TIMESTAMP(start);
		HRT_GET_TIMESTAMP(current);

    // this is exactly the same loop as below for measurement
    while (cnt < INNER_TRIES) {
      prev = current;
      HRT_GET_TIMESTAMP(current);

      sample++;

      HRT_GET_ELAPSED_TICKS(prev, current, &elapsed);
      // != instead of < in the benchmark loop in order to make the
      // notsmaller principle useful
      if ( elapsed != thr ) { 
        HRT_GET_ELAPSED_TICKS(start, prev, &results[cnt++]);
        HRT_GET_ELAPSED_TICKS(start, current, &results[cnt++]);
      }
    }

    // find minimum in results array - this is outside the
    // calibration/measurement loop!
    {
      if(min == 0) {
          printf("# [%i] the initialization reached 0 clock cycles - the clock accuracy seems too low (setting min=1 and exiting calibration)\n", r);
          min = 1;
          break;
      }
      int smaller=0;
      for(i = 0; i < INNER_TRIES; i+=2) {
        if(results[i+1]-results[i] < min) {
          min = results[i+1]-results[i];
          smaller=1;
          //printf("[%i] min: %lu\n", r, min);
      } } 
      if (!smaller) num_not_smaller++;
      else num_not_smaller = 0;
    }
	}
  
  // synchronize all processes
  {
    unsigned long smin = min;
    unsigned long *rmins = malloc(p*sizeof(unsigned long));
#ifdef NG_MPI
    MPI_Allgather(&smin, 1, MPI_UNSIGNED_LONG, rmins, 1, MPI_UNSIGNED_LONG, MPI_COMM_WORLD);
#else
    rmins[0]=smin;
#endif

    if(!r) {
    printf("# min clock cycles per rank: ");
    for(i=0;i<p;++i) printf("%li ", rmins[i]);
    printf("\n");}
  }

	// now we perform the actual benchmark: Read a time-stamp-counter in a tight
	// loop ignore the results if the timestamps are close to each other, as we can assume
	// that nobody interrupted us. If the difference of the timestamps exceeds a certain
	// threshold, we assume that we have been "hit" by a "noise event" and record the
	// time difference for later analysis

	cnt = 2;
	sample = 0;

	HRT_GET_TIMESTAMP(start);
	HRT_GET_TIMESTAMP(current);
  
  // perform this outside measurement loop in order to save
  // time/increase measurement frequency
  thr = min*(threshold/100.0);
	while (cnt < num_runs) {
		prev = current;
		HRT_GET_TIMESTAMP(current);

		sample++;

		HRT_GET_ELAPSED_TICKS(prev, current, &elapsed);
		if ( elapsed > thr ) {
			HRT_GET_ELAPSED_TICKS(start, prev, &results[cnt++]);
			HRT_GET_ELAPSED_TICKS(start, current, &results[cnt++]);
		}
	}

	results[0] = min;
	results[1] = sample;
}

void write_results_fwq(FILE *fd, uint64_t *results, int num_results) {
	int i;
	double tps;
	unsigned long long int tpus;

	tps = get_ticks_per_second();
	tpus = ceil((double) tps / pow(10,6));

  fprintf(fd, "# Fixed Work Quantum Benchmark\n");
  fprintf(fd, "# cycle number    duration [us]\n");
  for (i=0; i<num_results; i++) {
    fprintf(fd, "%i             %llu\n", i,  results[i]/tpus);
  }
}

void write_results_ftq(FILE *fd, uint64_t *results, int num_results) {
	int i;

  fprintf(fd, "# Fixed Time Quantum Benchmark\n");
  fprintf(fd, "# cycle number    workload iterations\n");
  for (i=0; i<num_results; i++) {
    fprintf(fd, " %i                %llu\n", i,  results[i]);
  }
}

void write_results_selfish(FILE *fd, uint64_t *results, int num_results, int threshold) {
	int i;
	double tpns, sum=0;
  uint64_t cycles=results[1];
  double cycletime;

	tpns = get_ticks_per_second()/1e9;
  cycletime = results[0] / tpns;

	fprintf(fd, "# Selfish Benchmark\n");
	fprintf(fd, "# Minimal cycle length [ns]: %f \n", cycletime);
	if(!g_options.mpi_opts->worldrank) printf("Minimal cycle length [ns]: %f \n", cycletime);
	fprintf(fd, "# Number of iterations (recorded+unrecorded): %llu \n", cycles);
	if(!g_options.mpi_opts->worldrank) printf("Number of iterations (recorded+unrecorded): %llu \n", cycles);
	fprintf(fd, "# Threshold: [%% minimal cycle length]: %i \n", threshold);
  if(!g_options.mpi_opts->worldrank) printf("Threshold: [%% minimal cycle length]: %i \n", threshold);
	fprintf(fd, "# \n");
	fprintf(fd, "# Time [ns]\tselfish duration [ns]\n");

	for (i=2; i<num_results; i+=2) {
		fprintf(fd, "%.2f\t%.2f\n", (results[i] - results[2])/tpns, (results[i+1]-results[i]-results[0])/tpns);
    sum += (results[i+1]-results[i]-results[0])/tpns;
	}

  if(!g_options.mpi_opts->worldrank || args_info.write_all_given) {
    double duration=(results[num_results-1]-results[2])/tpns;
    printf("[%i] CPU overhead due to noise: %.2f%%\n", g_options.mpi_opts->worldrank, 100*sum/duration);
	  fprintf(fd, "# CPU overhead due to noise: %.2f%%\n", 100*sum/duration);
    printf("[%i] Measurement period: %.2f s\n", g_options.mpi_opts->worldrank, duration/1e9);
	  fprintf(fd, "# Measurement period: %.2f s\n", duration/1e9);
    if(duration/1e9 < 1.0) printf("WARNING: the measurement period is less than a second, the results might be inaccurate and indicate too much noise! Increase the number of samples or the threshold to avoid this!\n");
  }

}

void noise_do_benchmarks(struct ng_module *module) {

	char *buffer;
	int size;
	FILE *outputfd;
	double workload;
	uint64_t *results;

#ifdef HAVE_CPUAFFINITY
	cpu_set_t mask;
#endif

  size = g_options.mpi_opts->worldsize;

#ifdef HAVE_CPUAFFINITY
	CPU_ZERO(&mask);
	CPU_SET(g_options.mpi_opts->worldrank, &mask); 
	if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
		ng_abort("Couldn't set CPU affinity\n");
	}
#endif

	//parse cmdline arguments
	if (ptrn_noise_parser_string(g_options.ptrnopts, &args_info, "netgauge") != 0) {
		ng_abort("commandline parser error");
	}

	// we should at least have room for 100 samples
	if (args_info.samples_arg < 100) {
    ng_info(NG_VLEV1, "# to small sample set supplied, increasing it to 100!");
		args_info.samples_arg = 100; 
	}

  char fname[1024];
  strncpy(fname, g_options.output_file, 1023);
  if(args_info.write_all_given) { 
    char suffix[512];
    snprintf(suffix, 511, ".%i", g_options.mpi_opts->worldrank);
    strncat(fname, suffix, 1023);
  }

  // TODO: should be done globally in netgauge.c
  ng_info(NG_VNORM, "writing data to %s", fname);
  if(!g_options.mpi_opts->worldrank || args_info.write_all_given) {
	  outputfd = open_output_file(fname);
    write_benchmark_information(outputfd);
    write_host_information(outputfd);
  }

	// allocate memory for the results
	results = malloc(2+args_info.samples_arg * sizeof(uint64_t));
	
	// synchronize as good as possible, so we can correlate the results later
#ifdef NG_MPI
	MPI_Barrier(MPI_COMM_WORLD);
#endif
	// carry out the measurements
	if (strcmp(args_info.method_arg, "fwq") == 0) {
    ng_info(NG_VNORM, "performing Fixed Work Quantum benchmark");
		perform_fwq(args_info.duration_arg, args_info.samples_arg, results);
		if(!g_options.mpi_opts->worldrank || args_info.write_all_given) write_results_fwq(outputfd, results, args_info.samples_arg);
	}
	if (strcmp(args_info.method_arg, "ftq") == 0) {
    ng_info(NG_VNORM, "performing Fixed Time Quantum benchmark");
		perform_ftq(args_info.duration_arg, args_info.samples_arg, results);
		if(!g_options.mpi_opts->worldrank || args_info.write_all_given) write_results_ftq(outputfd, results, args_info.samples_arg);
	}
	if (strcmp(args_info.method_arg, "selfish") == 0) {
    ng_info(NG_VNORM, "performing Selfish benchmark");
		perform_selfish(args_info.samples_arg, args_info.threshold_arg, results);
		if(!g_options.mpi_opts->worldrank || args_info.write_all_given) write_results_selfish(outputfd, results, args_info.samples_arg, args_info.threshold_arg);
	}

#ifdef NG_MPI
  // wait spinning to keep finished CPUs busy (to not allow them to
  // consume the noise events) -- this hopes that barrier spins,
  // actually, we should use a nonblocking barrier here to enforce
  // spinning on MPI_Wait() (as soon as we have an NB Barrier)
	MPI_Barrier(MPI_COMM_WORLD);
#endif

  if(!g_options.mpi_opts->worldrank || args_info.write_all_given) fclose(outputfd);
}

#else 
int register_pattern_noise(void) {return 0;}
#endif
