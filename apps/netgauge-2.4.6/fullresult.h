/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef FULLRESULTS_H_
#define FULLRESULTS_H_

#include "netgauge.h"

unsigned long long median(unsigned long long *array, int count);
int print_full_results(char *full_output_file,
			unsigned long long *blockcycles, unsigned long long *rttcycles,
			double clock_period,
			int testcount, int packetsize);

#endif /*FULLRESULTS_H_*/
