/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"

#ifdef NG_MOD_LIBOF

-> this module is manually disabled (this message will thrown a compile error which disables the module)

/* from of.h to avoid C++ problems and include hell! */
typedef void* OF_Request; 
int OF_Testall(int count, OF_Request *requests, int *flag);
int OF_Test(OF_Request *request);
int OF_Wait(OF_Request *request);
int OF_Waitall(int count, OF_Request *requests);
int OF_Isend(void *buf, int count, MPI_Datatype type, int dst, int tag, MPI_Comm comm, OF_Request *request);
int OF_Irecv(void *buf, int count, MPI_Datatype type, int src, int tag, MPI_Comm comm, OF_Request *request);
#define OF_OK 0


#define MSG_DONTWAIT 1

/* module function prototypes */
static int  libof_getopt(int argc, char **argv, struct ng_options *global_opts);
static int  libof_init(struct ng_options *global_opts);
static void libof_shutdown(struct ng_options *global_opts);
void libof_usage(void);
void libof_writemanpage(void);
static int libof_sendto(int dst, void *buffer, int size);
static int libof_recvfrom(int src, void *buffer, int size);
static int libof_isendto(int dst, void *buffer, int size, NG_Request *req);
static int libof_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static int libof_test(NG_Request *req);

extern struct ng_options g_options;

/* module registration data structure */
static struct ng_module libof_module = {
   .name         = "libof",
   .desc         = "Mode libof uses OF_Isend/OF_Irecv for data transmission.",
   .max_datasize = -1,			/* can send data of arbitrary size */
   .headerlen    = 0,			/* no extra space needed for header */
   .getopt       = libof_getopt,
   .flags        = NG_MOD_RELIABLE | NG_MOD_CHANNEL, /* memreg for libof_Alloc_mem? */
   .malloc       = NULL, /* libof_Alloc_mem? */
   .init         = libof_init,
   .shutdown     = libof_shutdown,
   .usage        = libof_usage,
   .sendto       = libof_sendto,
   .recvfrom     = libof_recvfrom,
   .isendto      = libof_isendto,
   .irecvfrom    = libof_irecvfrom,
   .test         = libof_test,
   .writemanpage = libof_writemanpage
};



/* module private data */
static struct libof_private_data {
      int          send_buf;
      int          recv_buf;
      int          send_flags;
      int          recv_flags;
      MPI_Comm     comm;
      int          partner;
/*       int          comm_size; */
/*       int          comm_rank; */
} module_data;


static int libof_sendto(int dst, void *buffer, int size) {
  OF_Request req;
  OF_Isend(buffer, size, MPI_BYTE, dst, 0, MPI_COMM_WORLD, &req);
  OF_Wait(&req);
  return size;
}

static int libof_isendto(int dst, void *buffer, int size, NG_Request *req) {
  OF_Request *ofreq;
  
  ofreq = malloc(sizeof(OF_Request));
  /* the NG_Request itself is a pointer to point to arbitrary module
   * data */
  *req = (NG_Request*)ofreq;
  
  OF_Isend(buffer, size, MPI_BYTE, dst, 0, MPI_COMM_WORLD, ofreq);
  return size;
}

static int libof_irecvfrom(int src, void *buffer, int size, NG_Request *req) {
  OF_Request *ofreq;
  
  ofreq = malloc(sizeof(OF_Request));
  /* the NG_Request itself is a pointer to point to arbitrary module
   * data */
  *req = (NG_Request*)ofreq;
  
  OF_Irecv(buffer, size, MPI_BYTE, src, 0, MPI_COMM_WORLD, ofreq);
  return size;
}

static int libof_test(NG_Request *req) {
  OF_Request *ofreq;
  int flag=0;
  
  /* req == NULL indicates that it is finished */
  if(*req == NULL) return 0;
  
  ofreq = (OF_Request*)*req;
  if (OF_Test(ofreq) == OF_OK) flag = 1;

  if(flag) {
    free(*req);
    *req = NULL;
    return 0;
  }
  return 1;
}

static int libof_recvfrom(int src, void *buffer, int size) {
  OF_Request req;
  OF_Irecv(buffer, size, MPI_BYTE, src, 0, MPI_COMM_WORLD, &req);
  OF_Wait(&req);

  return size;
}


/** module registration */
int register_libof(void) {
   ng_register_module(&libof_module);
   return 0;
}


/* parse command line parameters for module-specific options */
static int libof_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;
   char *optchars = NG_OPTION_STRING "N:";	// additional module options
   extern char *optarg;
   extern int optind, opterr, optopt;
   
   int failure = 0;
   

   if (!global_opts->mpi) {
      ng_error("Mode libof requires the global option '-p' (execution via libof)");
      failure = 1;
   }
   
   memset(&module_data, 0, sizeof(module_data));

   while ( (!failure) && ((c = getopt( argc, argv, optchars)) >= 0) ) {
      switch (c) {
	 case '?':	// unrecognized or badly used option
	    if (!strchr(optchars, optopt))
	       continue;	// unrecognized
	    ng_error("Option %c requires an argument", optopt);
	    failure = 1;
	    break;
	 case 'N':	// non-blocking send/receive mode
	    if (strchr(optarg, 's')) {
	       module_data.send_flags |= MSG_DONTWAIT;
	       ng_info(NG_VLEV2, "Using nonblocking send mode");
				}
	    if (strchr(optarg, 'r')) {
	       module_data.recv_flags |= MSG_DONTWAIT;
	       ng_info(NG_VLEV2, "Using nonblocking receive mode");
	    }
	    break;
      }
   }
   
   // check for necessary parameters, apply default values where possible
/*	if (!failure) {
	}
*/
   // report success or failure
   return failure;
}


/* module specific benchmark initialization */
static int libof_init(struct ng_options *global_opts) {
   
   // get some supplied options from the main program for further use
   // when "global_opts" is not available
//   module_data.comm = global_opts->libof_opts->comm;
//   module_data.partner = global_opts->libof_opts->partner;
//	module_data.comm_size = global_opts->libof_opts->comm_size;
//	module_data.comm_rank = global_opts->libof_opts->comm_rank;
   
   /* report success */
   return 0;
}

/* module specific shutdown */
void libof_shutdown(struct ng_options *global_opts) {
   ng_info(NG_VLEV2, "Nothing to do for mode libof");
}


/* module specific usage information */
void libof_usage(void) {
//	ng_option_usage("-N", "nonblocking [s]end and/or [r]eceive mode", "[s][r]");
}

/* module specific manpage information */
void libof_writemanpage(void) {
}

#else

/* module registration */
int register_libof(void) {
   /* do nothing if libof is not colibofled in */
   return 0;
}


#endif  /* #ifdef NG_ARMCI */

