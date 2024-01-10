/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "cpustat.h"
#include "netgauge.h"
#include <stdio.h>
#include <stdlib.h>

/* this is too imprecise, it measures in USER_HZ, usually 1/100 HZ ... */

int get_cpustatus(struct kstat *stat) {
   static FILE *fp = NULL;
   char buf[1024];
   unsigned long long us, sy, ni, id, wa, hi, si; 
   
   if (!(fp = fopen("/proc/stat", "r") )) {
      ng_perror("cannot open /proc/stat!");
      exit(1);
   }	
   
   if (!fgets(buf, sizeof(buf)-1, fp)) {
      ng_perror("cannot read /proc/stat!");
      exit(1);
   }	
   
   /*  empty in 2.4 kernel ... */
   hi = 0;
   si = 0;
   
   sscanf(buf, "cpu %llu %llu %llu %llu %llu %llu %llu", &us, &sy,
	  &ni, &id, &wa, &hi, &si);
   
   stat->us=us;
   stat->sy=sy;
   stat->ni=ni;
   stat->id=id;
   stat->wa=wa;
   stat->hi=hi;
   stat->si=si;
   
   fclose(fp);
   return 0;
}

int calc_cpustatus(struct kstat *stat1, struct kstat *stat2) {
   stat1->us = stat2->us - stat1->us;
   stat1->sy = stat2->sy - stat1->sy;
   stat1->ni = stat2->ni - stat1->ni;
   stat1->id = stat2->id - stat1->id;
   stat1->wa = stat2->wa - stat1->wa;
   stat1->hi = stat2->hi - stat1->hi;
   stat1->si = stat2->si - stat1->si;
   
   return 0;
}

