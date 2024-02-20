/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include <stdio.h>
#include <stdarg.h>

/******** WORKLOAD STUFF *******/

#define WORKLOAD   a^=a+a; \
				   a^=a+a+a; \
				   a>>=b; \
				   a>>=a+a; \
				   a^=a<<b; \
				   a^=a+b; \
				   a+=(a+b)&07; \
				   a^=n; \
				   b^=a; \
				   a|=b;

#define TEN(A)  A A A A A A A A A A 

#define HUNDRED(A)  TEN(A) TEN(A) TEN(A) TEN(A) TEN(A) \
				    TEN(A) TEN(A) TEN(A) TEN(A) TEN(A)

#define THOUSAND(A)  HUNDRED(A) HUNDRED(A) HUNDRED(A) HUNDRED(A) HUNDRED(A) \
					 HUNDRED(A) HUNDRED(A) HUNDRED(A) HUNDRED(A) HUNDRED(A)

#define INIT_WORKLOAD  int a=rand(); int b=rand(); int n=rand();

#define FINALIZE_WORKLOAD printf("# random output (to prevent compiler optimizations): %i\n", a+b);


/********** TIMER STUFF ********/

#include "hrtimer/hrtimer.h"

/******** PROTOTYPES *********/

/* This function returns the number of timerticks that happen in an interval of
 * one second, when the timer-macro "HRT_GET_TIMESTAMP( HRT_TIMESTAMP_T t1 )" is used. */

double get_ticks_per_second();

/* This function returns the number of times the workload, which is defined
 * through the macro "WORKLOAD", has to be executed in order to get an
 * approximate execution time of "usecs" microseconds */

double tune_workload(int usecs);

/* This function performs a basic tests to ensure that the timer, available
 * through the "HRT_GET_TIMESTAMP( HRT_TIMESTAMP_T t1 )" macro is usable. A positive return
 * value indicates that it can be assumed that the timer is usable, otherwise a
 * negative return value is returned. Note that this function does not check
 * the accuracy of the timer, it just identifies completely unusable timers. */

int sanity_check();

/* This function performs the fixed work quantum benchmark "num_runs" times
 * with a "computation-phase" of length "usecs". The results (actual duration
 * of the computation phase) are written in an buffer pointed to by the
 * "*results" parameter */

void perform_fwq(int usecs, int num_runs, uint64_t *results);

/* write some system information into the output file, due to
 * portability reasons only uname() is used right now */

void write_host_information(FILE *fd);

/* write the results into the output file */

void write_results(FILE *fd, HRT_TIMESTAMP_T *results, int num_results);

/* A printf wrapper for log messages */

void log_message(char *fmt, ...);

