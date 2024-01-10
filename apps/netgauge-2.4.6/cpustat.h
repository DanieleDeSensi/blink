/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef CPUSTAT_H_
#define CPUSTAT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CPU status information
 */
struct kstat {
      /** user */
      unsigned long long us;
      /** system */
      unsigned long long sy;
      /** nice */
      unsigned long long ni;
      /** idle */
      unsigned long long id;
      /** wait (IO) */
      unsigned long long wa;
      /** hardware interrupts */
      unsigned long long hi;
      /** software interrupts */
      unsigned long long si;
};

/**
 * reads cpu status information from "proc" special
 * filesystem and fills the supplied kstat struct
 */
int get_cpustatus(struct kstat *stat);

/**
 * calculates the difference of stat1 and stat2
 * and stores the result in stat1
 */
int calc_cpustatus(struct kstat *stat1, struct kstat *stat2);


#ifdef __cplusplus
}
#endif
#endif
