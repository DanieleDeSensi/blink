/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"

/* module function prototypes */
static int  dummy_getopt(int argc, char **argv, struct ng_options *global_opts);
static int  dummy_init(struct ng_options *global_opts);
static void dummy_shutdown(struct ng_options *global_opts);
void dummy_usage(void);
void dummy_writemanpage(void);
static int dummy_sendto(int dst, void *buffer, int size);
static int dummy_recvfrom(int src, void *buffer, int size);
static int dummy_isendto(int dst, void *buffer, int size, NG_Request *req);
static int dummy_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static int dummy_test(NG_Request *req);

extern struct ng_options g_options;

/* module registration data structure */
static struct ng_module dummy_module = {
   .name         = "dummy",
   .desc         = "Mode dummy doesn't actually do anything.",
   .max_datasize = -1,			/* can send data of arbitrary size */
   .headerlen    = 0,			/* no extra space needed for header */
   .getopt       = dummy_getopt,
   .flags        = NG_MOD_RELIABLE | NG_MOD_CHANNEL, /* memreg for dummy_Alloc_mem? */
   .malloc       = NULL, /* MPI_Alloc_mem? */
   .init         = dummy_init,
   .shutdown     = dummy_shutdown,
   .usage        = dummy_usage,
   .sendto       = dummy_sendto,
   .recvfrom     = dummy_recvfrom,
   .isendto      = dummy_isendto,
   .irecvfrom    = dummy_irecvfrom,
   .test         = dummy_test,
   .writemanpage = dummy_writemanpage
};


static int dummy_sendto(int dst, void *buffer, int size) {
  printf("not supported in dummy module\n");
  return 0;
}

static int dummy_isendto(int dst, void *buffer, int size, NG_Request *req) {
  printf("not supported in dummy module\n");
  return 0;
}

static int dummy_irecvfrom(int src, void *buffer, int size, NG_Request *req) {
  printf("not supported in dummy module\n");
  return 0;
}

static int dummy_test(NG_Request *req) {
  printf("not supported in dummy module\n");
  return 0;
}

static int dummy_recvfrom(int src, void *buffer, int size) {
  printf("not supported in dummy module\n");
  return 0;
}


/** module registration */
int register_dummy(void) {
   ng_register_module(&dummy_module);
   return 0;
}


/* parse command line parameters for module-specific options */
static int dummy_getopt(int argc, char **argv, struct ng_options *global_opts) {
  return 0;
}


/* module specific benchmark initialization */
static int dummy_init(struct ng_options *global_opts) {
   return 0;
}

/* module specific shutdown */
void dummy_shutdown(struct ng_options *global_opts) {
  return;
}


/* module specific usage information */
void dummy_usage(void) {
//	ng_option_usage("-N", "nonblocking [s]end and/or [r]eceive mode", "[s][r]");
}

/* module specific manpage information */
void dummy_writemanpage(void) {
}

