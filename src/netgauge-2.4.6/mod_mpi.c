/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_MPI && defined NG_MOD_MPI

#define MSG_DONTWAIT 1

/* module function prototypes */
static int  mpi_getopt(int argc, char **argv, struct ng_options *global_opts);
static int  mpi_init(struct ng_options *global_opts);
static void mpi_shutdown(struct ng_options *global_opts);
void mpi_usage(void);
void mpi_writemanpage(void);
static int mpi_sendto(int dst, void *buffer, int size);
static int mpi_recvfrom(int src, void *buffer, int size);
static int mpi_isendto(int dst, void *buffer, int size, NG_Request *req);
static int mpi_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static int mpi_test(NG_Request *req);

extern struct ng_options g_options;

/* module registration data structure */
static struct ng_module mpi_module = {
   .name         = "mpi",
   .desc         = "Mode mpi uses MPI_Send/MPI_Recv for data transmission.",
   .max_datasize = -1,			/* can send data of arbitrary size */
   .headerlen    = 0,			/* no extra space needed for header */
   .getopt       = mpi_getopt,
   .flags        = NG_MOD_RELIABLE | NG_MOD_CHANNEL, /* memreg for MPI_Alloc_mem? */
   .malloc       = NULL, /* MPI_Alloc_mem? */
   .init         = mpi_init,
   .shutdown     = mpi_shutdown,
   .usage        = mpi_usage,
   .sendto       = mpi_sendto,
   .recvfrom     = mpi_recvfrom,
   .isendto      = mpi_isendto,
   .irecvfrom    = mpi_irecvfrom,
   .test         = mpi_test,
   .writemanpage = mpi_writemanpage
};



/* module private data */
static struct mpi_private_data {
      int          send_buf;
      int          recv_buf;
      int          send_flags;
      int          recv_flags;
      MPI_Comm     comm;
      int          partner;
/*       int          comm_size; */
/*       int          comm_rank; */
} module_data;


static int mpi_sendto(int dst, void *buffer, int size) {
  MPI_Send(buffer, size, MPI_BYTE, dst, 0, MPI_COMM_WORLD);
  return size;
}

static int mpi_isendto(int dst, void *buffer, int size, NG_Request *req) {
  MPI_Request *mpireq;
  
  mpireq = malloc(sizeof(MPI_Request));
  /* the NG_Request itself is a pointer to point to arbitrary module
   * data */
  *req = (NG_Request*)mpireq;
  
  MPI_Isend(buffer, size, MPI_BYTE, dst, 0, MPI_COMM_WORLD, mpireq);
  return size;
}

static int mpi_irecvfrom(int src, void *buffer, int size, NG_Request *req) {
  MPI_Request *mpireq;
  
  mpireq = malloc(sizeof(MPI_Request));
  /* the NG_Request itself is a pointer to point to arbitrary module
   * data */
  *req = (NG_Request*)mpireq;
  
  MPI_Irecv(buffer, size, MPI_BYTE, src, 0, MPI_COMM_WORLD, mpireq);
  return size;
}

static int mpi_test(NG_Request *req) {
  MPI_Request *mpireq;
  int flag=0;
  
  /* req == NULL indicates that it is finished */
  if(*req == NULL) return 0;
  
  mpireq = (MPI_Request*)*req;
  MPI_Test(mpireq, &flag, MPI_STATUS_IGNORE);

  //printf("MPI_Test returned %i\n", flag);

  if(flag) {
    free(*req);
    *req = NULL;
    return 0;
  }
  return 1;
}

static int mpi_recvfrom(int src, void *buffer, int size) {
  MPI_Recv(buffer, size, MPI_BYTE, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  return size;
}


/** module registration */
int register_mpi(void) {
   ng_register_module(&mpi_module);
   return 0;
}


/* parse command line parameters for module-specific options */
static int mpi_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;
   char *optchars = NG_OPTION_STRING "N:";	// additional module options
   extern char *optarg;
   extern int optind, opterr, optopt;
   
   int failure = 0;
   

   if (!global_opts->mpi) {
      ng_error("Mode mpi requires the global option '-p' (execution via MPI)");
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
static int mpi_init(struct ng_options *global_opts) {
   
   // get some supplied options from the main program for further use
   // when "global_opts" is not available
//   module_data.comm = global_opts->mpi_opts->comm;
//   module_data.partner = global_opts->mpi_opts->partner;
//	module_data.comm_size = global_opts->mpi_opts->comm_size;
//	module_data.comm_rank = global_opts->mpi_opts->comm_rank;
   
   /* report success */
   return 0;
}

/* module specific shutdown */
void mpi_shutdown(struct ng_options *global_opts) {
   ng_info(NG_VLEV2, "Nothing to do for mode mpi");
}


/* module specific usage information */
void mpi_usage(void) {
//	ng_option_usage("-N", "nonblocking [s]end and/or [r]eceive mode", "[s][r]");
}

/* module specific manpage information */
void mpi_writemanpage(void) {
}

#else

/* module registration */
int register_mpi(void) {
   /* do nothing if MPI is not compiled in */
   return 0;
}


#endif  

