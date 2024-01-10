/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_DISTRTT
#include "statistics.h"
#include "hrtimer/hrtimer.h"
#include <string.h>

#define NUM_TESTS 500
#define SLEEPTIME 100000


/* internal function prototypes & extern stuff */
static void fdistrtt_do_benchmarks(struct ng_module *module);
extern struct ng_options g_options;

/**
 * one-to-many communication pattern registration
 * structure
 */
static struct ng_comm_pattern pattern_distrtt = {
   .name = "distrtt",
   .flags = '\0', /* we need a reliable transport and channel semantics */
   .desc = "measures latency distribution",
   .do_benchmarks = fdistrtt_do_benchmarks
};

/**
 * registers this comm. pattern with the main prog.,
 * but only if we use MPI
 */
int register_pattern_distrtt() {
  /* there's no use for this mode without MPI 
   * TODO: there coule be :) */
#if NG_MPI
  ng_register_pattern(&pattern_distrtt);
#endif 
  return 0;
}

void fdistrtt_do_benchmarks(struct ng_module *module) {
#if NG_MPI
	int myproc, other_proc, nprocs;
	MPI_Status status;
	double timestart;
     	unsigned int __a, __d;
	u_int64_t i;
	HRT_TIMESTAMP_T t1, t2, t3;
	u_int64_t roundtrips[NUM_TESTS]; /* roundtrip time, minim. newtwork jitter */
	u_int64_t clockdiffs[NUM_TESTS]; /* clock "difference", see expl. below */
	double localtimes[NUM_TESTS]; /* time of measurement, relative to start */

	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myproc);

	if (nprocs != 2) {
		printf("Exactly two nodes please!\n");
        MPI_Abort(MPI_COMM_WORLD,1);
		exit (1);
	}
	
	other_proc = (myproc + 1) % 2;

	MPI_Barrier(MPI_COMM_WORLD);
	timestart = MPI_Wtime();

	/* measurement loop */
	for (i=0; i<NUM_TESTS; i++) {
		/* CLIENT */
		if (myproc == 0) {
			usleep(SLEEPTIME); /* wait for some time so that node 2 is in recv*/
			HRT_GET_TIMESTAMP(t1);
            module->sendto(other_proc, &t1, sizeof(HRT_TIMESTAMP_T));
            module->recvfrom(other_proc, &t2, sizeof(HRT_TIMESTAMP_T));
			/* MPI_Send(&t1, 8, MPI_BYTE, other_proc, 0, MPI_COMM_WORLD);
			MPI_Recv(&t2, 8, MPI_BYTE, other_proc, 0, MPI_COMM_WORLD, &status);*/
			HRT_GET_TIMESTAMP(t3);
			localtimes[i] = MPI_Wtime() - timestart;
			HRT_GET_ELAPSED_TICKS(t1, t3, &roundtrips[i]);
			//if (t1 > t2) {clockdiffs[i] = t1 - t2;}
			//else {clockdiffs[i] = t2 - t1;}
			HRT_GET_ELAPSED_TICKS(t1, t2, &clockdiffs[i]);
			/* of course you would have to substract the inital offset to
			 * get the real difference between the two clocks but we will
			 * never ever know that offset. 
			 * Despite of that we can show that diff + offset changes over
			 * time, which proves that the two clocks run at different speeds
			 * because offset is a constant. So diff must have changed. But 
			 * that can only happen if the clocks have different speeds, which
			 * was what we wanted to show*/
			printf("%llu %llu %llu %f\n", i, roundtrips[i], clockdiffs[i], localtimes[i]);
		}
		else {
			//MPI_Recv(&t1, 8, MPI_BYTE, other_proc, 0, MPI_COMM_WORLD, &status);
            module->recvfrom(other_proc, &t1, sizeof(HRT_TIMESTAMP_T));
			HRT_GET_TIMESTAMP(t2);
            module->sendto(other_proc, &t2, sizeof(HRT_TIMESTAMP_T));
			//MPI_Send(&t2, 8, MPI_BYTE, other_proc, 0, MPI_COMM_WORLD);
		}
	}

#endif
}

#else 
int register_pattern_distrtt(void) {return 0;}
#endif
