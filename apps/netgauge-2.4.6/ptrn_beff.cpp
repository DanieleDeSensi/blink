/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_BEFF && defined NG_MPI
#include "hrtimer/hrtimer.h"
#include "mersenne/MersenneTwister.h"
#include "ptrn_beff_cmdline.h"
#include <stdint.h>
#include <vector>
#include <numeric>

extern "C" {
#include "ng_sync.h"

extern struct ng_options g_options;


/* internal function prototypes */
static void beff_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_beff = {
   pattern_beff.name = "beff",
   pattern_beff.desc = "assesses the influence of network jitter on collective MPI operations",
   pattern_beff.flags = 0,
   pattern_beff.do_benchmarks = beff_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_beff() {
   ng_register_pattern(&pattern_beff);
   return 0;
}

static void beff_do_benchmarks(struct ng_module *module) {

}

} /* extern C */

#else
extern "C" {
int register_pattern_beff(void) {return 0;};
} /* extern C */
#endif
