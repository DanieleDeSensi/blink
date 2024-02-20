/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"

#if defined NG_ARMCI && defined NG_MOD_ARMCI
#include "armci.h"

#define NG_ARMCI_WAITING 0
#define NG_ARMCI_ARRIVED 1


#define MSG_DONTWAIT 1

/* module function prototypes */
static int  armci_getopt(int argc, char **argv, struct ng_options *global_opts);
static int  armci_init(struct ng_options *global_opts);
static int  armci_server(void *buffer, int size);
static int  armci_client(void *buffer, int size, unsigned long long *blockcycle, unsigned long long *rttcycle);
static void armci_shutdown(struct ng_options *global_opts);
void armci_usage(void);
static int armci_sendto(int dst, void *buffer, int size);
static int armci_recvfrom(int src, void *buffer, int size);
static int armci_isendto(int dst, void *buffer, int size, NG_Request *req);
static int armci_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static int armci_test(NG_Request *req);
static void* armci_malloc(size_t size);

extern struct ng_options g_options;

/* module registration data structure */
static struct ng_module armci_module = {
   .name         = "armci",
   .desc         = "Mode armci uses ARCMI_Put for data transmission.",
   .max_datasize = -1,			/* can send data of arbitrary size */
   .headerlen    = 0,			/* no extra space needed for header */
   .flags        = NG_MOD_RELIABLE | NG_MOD_MEMREG,
   .malloc       = armci_malloc, 
   .getopt       = armci_getopt,
   .init         = armci_init,
   .shutdown     = armci_shutdown,
   .usage        = armci_usage,
   .sendto       = armci_sendto,
   .recvfrom     = armci_recvfrom,
   .isendto      = armci_isendto,
   .irecvfrom    = armci_irecvfrom,
   .test         = armci_test,
   .writemanpage = NULL 
};

/* an array of size p that indicates the current counter of received
 * messages per sender - is used to prevent race conditions */
static unsigned long* recvnums;
/* an array of size p that indicates the current counter of sent
 * messages per receiver - is used to prevent race conditions */
static unsigned long* sendnums;

/* this array stores all destination starting adresses - needed in
 * armci_sendto */
static void** rank2buf;
/* this array stores all flags - needed in
 * armci_sendto/armci_probe - for a communicator size of p, every node
 * has a char array of p elements to be notified if some host sent
 * something */
static volatile unsigned long** rank2flags;

/* module specific benchmark initialization */
static int armci_init(struct ng_options *global_opts) {
  int ret, i; 

  ret = ARMCI_Init();
  if(0 != ret) ng_error("ARMCI_Init returned %i\n", ret);

  /* conpletion indication flags */
  rank2flags = malloc(g_options.mpi_opts->worldsize*sizeof(unsigned long*));
  ret = ARMCI_Malloc((void**)rank2flags, g_options.mpi_opts->worldsize*sizeof(unsigned long));
  if(0 != ret) ng_error("ARMCI_malloc returned %i\n", ret);

  /* set the flags to NG_ARMCI_WAITING */
  for(i=0; i<g_options.mpi_opts->worldsize;i++)
    ((long*)rank2flags[g_options.mpi_opts->worldrank])[i] = NG_ARMCI_WAITING;

  MPI_Barrier(MPI_COMM_WORLD);

  /* allocate counter arrays */
  recvnums = malloc(g_options.mpi_opts->worldsize*sizeof(unsigned long));
  if(recvnums == NULL) ng_error("malloc error\n");
  for(i=0; i<g_options.mpi_opts->worldsize;i++) recvnums[i] = 0;
  sendnums = malloc(g_options.mpi_opts->worldsize*sizeof(unsigned long));
  if(sendnums == NULL) ng_error("malloc error\n");
  for(i=0; i<g_options.mpi_opts->worldsize;i++) sendnums[i] = 0;
  
   /* report success */
   return 0;
}

static void* armci_malloc(size_t size) {
  int ret;

  /* buffer flags */
  rank2buf = malloc(g_options.mpi_opts->worldsize*sizeof(void*));
  ret = ARMCI_Malloc(rank2buf, size);
  if(0 != ret) ng_error("ARMCI_malloc returned %i\n", ret);

  return rank2buf[g_options.mpi_opts->worldrank];
  
}

static int armci_sendto(int dst, void *buffer, int size) {
  int ret;
  
  /* ok, we need a specific buffer address fo every destination ... so
   * this is in the global array rank2buf[rank] */
  ret = ARMCI_Put(buffer, (void*)rank2buf[dst], size, dst);

  /* avoids race condition on case of multiple sends/recvs */
  //ARMCI_Fence(dst);

  /* put the flag */
  sendnums[dst]++;
  if(sendnums[dst] == ULONG_MAX) ng_error("we hit the upper bound on unsigned long - too many messages\n");
  ret = ARMCI_Put(&sendnums[dst], (void*)&(rank2flags[dst])[g_options.mpi_opts->worldrank], sizeof(unsigned long), dst);
  //printf("[%i] set flag to %lu in %i\n", g_options.mpi_opts->worldrank, sendnums[dst], dst);

  return size;
}

static int armci_recvfrom(int src, void *buffer, int size) {

  recvnums[src]++;
  if(recvnums[src] == ULONG_MAX) ng_error("we hit the upper bound on unsigned long - too many messages\n");
  //printf("[%i] waiting for flag (%lu) from %i to become %lu\n", g_options.mpi_opts->worldrank, (rank2flags[g_options.mpi_opts->worldrank])[src], src, recvnums[src]);
  while((rank2flags[g_options.mpi_opts->worldrank])[src] < recvnums[src]);
  //printf("[%i] got flag (%lu) from %i\n", g_options.mpi_opts->worldrank, recvnums[src], src);

  return size;
}

static int armci_isendto(int dst, void *buffer, int size, NG_Request *req) {
  /*
  MPI_Request *mpireq;
  
  mpireq = malloc(sizeof(MPI_Request));
  req = (NG_Request*)mpireq;
  
  MPI_Isend(buffer, size, MPI_BYTE, dst, 0, MPI_COMM_WORLD, mpireq);
  */
  ng_error("not implemented yet!\n");
  return size;
}

static int armci_irecvfrom(int src, void *buffer, int size, NG_Request *req) {
  /*MPI_Request *mpireq;
  
  mpireq = malloc(sizeof(MPI_Request));
  req = (NG_Request*)mpireq;
  
  MPI_Irecv(buffer, size, MPI_BYTE, src, 0, MPI_COMM_WORLD, mpireq);
  */
  ng_error("not implemented yet!\n");
  return size;
}

static int armci_test(NG_Request *req) {
  /*
  MPI_Request *mpireq;
  int flag=0;
  
  mpireq = (MPI_Request*)req;
  MPI_Test(mpireq, &flag, MPI_STATUS_IGNORE);

  if(flag) {
    free(req);
    return 0;
  }
  */
  ng_error("not implemented yet!\n");
  return 1;
}


/** module registration */
int register_armci(void) {
   ng_register_module(&armci_module);
   return 0;
}


/* parse command line parameters for module-specific options */
static int armci_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;
   char *optchars = NG_OPTION_STRING "N:";	// additional module options
   extern char *optarg;
   extern int optind, opterr, optopt;
   
   int failure = 0;

   if (!global_opts->mpi) {
      ng_error("Mode armci requires the global option '-p' (execution via MPI)");
      failure = 1;
   }
   
   return failure;
}


/* module specific shutdown */
void armci_shutdown(struct ng_options *global_opts) {
   ARMCI_Free((void*)rank2flags[g_options.mpi_opts->worldrank]);
   ARMCI_Free(rank2buf[g_options.mpi_opts->worldrank]);
   free(rank2flags);
   free(rank2buf);
  
   //ng_info(NG_VLEV2, "Nothing to do for mode mpi");
   ARMCI_Finalize();
}


/* module specific usage information */
void armci_usage(void) {
//	ng_option_usage("-N", "nonblocking [s]end and/or [r]eceive mode", "[s][r]");
}

/* module specific manpage information */
void arcmi_writemanpage(void) {
}

#else

/* module registration */
int register_armci(void) {
   /* do nothing if MPI is not compiled in */
   return 0;
}


#endif  /* #ifdef NG_MPI */

