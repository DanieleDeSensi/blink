/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef NETGAUGE_H_
#define NETGAUGE_H_
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>	   /* getopt() */
#include <sys/types.h>     /* various types */
#include <sys/time.h>      /* time datatypes */
#include <errno.h>	   /* error numbers */
#include <math.h>	   /* mathematical functions */
#include <unistd.h> /* usleep */

#include "config.h"        /* macros created by autoconf */

#ifdef HAVE_GETOPT_H
#include <getopt.h> /* getopt long */
#endif

#ifdef NG_MPI
#define MPICH_SKIP_MPICXX
#include <mpi.h>           /* MPI specific structures and functions */
#else
/* define the MPI_Comm type to int for easier use in the rest of the program */
#define MPI_Comm int  
#endif

#include "cpustat.h"	   /* struct kstat */

/** application version - use autoconf's VERSION macro instead of our own */
#define NG_VERSION VERSION

/** global options */
#define NG_OPTION_STRING "+hwsvm:o:c:d:g:pnt:f:x:"

/** padding infos, used by ng_longoption_usage */
#define DESC_START_POS 40
#define CLI_WIDTH 80

/** default output file name */
#define NG_DEFAULT_OUTPUT_FILE "ng.out"

/** default initial test count */
#define NG_DEFAULT_TEST_COUNT 128

/** initial amount of data to transmit (in bytes) */
#define NG_START_PACKET_SIZE 1

/** maximum amount of data to transmit (in bytes) */
#define NG_MAX_DATA_SIZE 1024*1024*1024

/** gradation of the geometrical data size growth */
#define NG_DEFAULT_DATASIZE_GRADATION 1

/** Amount of dots while testing */
#define NG_DOT_COUNT 50

#define NG_DEFAULT_COMM_PATTERN "one_one"

/*  convenience defines  */
#define rint register int 
#define uint unsigned int

/** constants to indicate verbosity level of an information message */
#define NG_VNORM 1            /*  normal output */
#define NG_VLEV1 2            /*  verbose output */
#define NG_VLEV2 4            /*  extra verbose output */
#define NG_VLEV_MAX NG_VLEV2  /*  don't let the options->verbose variable rise above this value */

/**
 * if this bit is set in the vlevel variable of a msg.
 * and we are using MPI to start the processes, let every process report/print
 * this message (otherwise only one process will report/print the msg.) 
 */
#define NG_VPALL 128

#ifdef __cplusplus
extern "C" {
#endif

/**
 * structure description
 */
struct option_info {
	const char *desc;
	const char *param;
};

/**
 * mpi specific data 
 */
struct ng_mpi_options {
   /** MPI communicator used */
   MPI_Comm comm;
   /** process rank whithin MPI_COMM_WORLD */
   int worldrank;
   /** which communication partner to use (mpi rank whithin "comm") */
   int partner; 
   /** the size of MPI_COMM_WORLD */
   int worldsize;
   /** the requested threadlevel */
   int threadlevel;
};

/* a linked list of communication modules */
struct ng_module_list {
   struct ng_module *module;
   struct ng_module_list *next;
};

/* a netgauge request to identify a non-blocking request. This is a
 * pointer to void. This is used to point to the ng_module specific
 * request structure (it has to be casted in each module to the right
 * one because every module may have an own data type) 
 */
typedef void* NG_Request;

/**
 * Global options data structure. There is exactly one instance of
 * this struct. It's globally aviable under the name "g_options".
 */
struct ng_options {
   /** verbose output switch */
   u_int8_t              verbose;
   /** no (unnecessary) output (only benchmark results) */
   char                  silent;
   /** test mode */
   char                  *mode;
   
   /** communication pattern to use */
   const char *pattern;
   
   /**
    * server / client mode switch
    * TODO: candidate for removal, put into the one_one pattern?
    */
   char                  server;

   /** output file name */
   char                  *output_file;
   /** output file for full output e.g. every single measured value */
   char				   *full_output_file; 
   /** count of tests per packet size */
   unsigned long         testcount;
   /** max number of seconds after which the test is to be interrupted */
   unsigned long         max_testtime;
   /** maximum data size */
   unsigned long         max_datasize;
   /** minimum data size */
   unsigned long         min_datasize;
	 /** did the user select a size range? */
	 char size_given;
   /** gradation of the geometrical data size growth */
   unsigned int          grad_datasize;
   /** parallel execution via MPI */
   unsigned int          mpi;
   /** pointer to mpi data structure */
   struct ng_mpi_options *mpi_opts;
   /* perform timer sanity check */
   int do_sanity_check;
   /* print hostnames */
   int print_hostnames;

   /** pointers to the command line arguments for
       mode and pattern */
	char *modeopts;
	char *ptrnopts;
	char *ngopts;
	char *allopts;

};

/**
 * Communication pattern registration data structure. Has to be
 * filled and passed to ng_register_pattern(...) for the
 * pattern to be useable by the main program.
 */
struct ng_comm_pattern {
   /**
    * The pattern name. Has to be unique accross all patterns.
    */
   const char *name;
   
   /**
    * A short description for this pattern.
    */
   const char *desc;

   /** 
    * some FLAGS, includes:
    *  - NG_PTRN_NB - this pattern needs a module supporting
    *  non-blocking functions (e.g. 1toN and Nto1)
    */
   char flags;

   /* this is the CPP Flag definition */
    #define NG_PTRN_NB   1
   
   /**
    * Executes the actual benchmark(s).
    *
    * @param module The module to use for transmission.
    */
   void (*do_benchmarks)(struct ng_module *module);
  
   /**
    * Let the pattern parse it's command line options.
    * This field may be left void if unneeded.
    */
   int  (*getopt)(int argc, char **argv);

   /**
    * TODO:
    * Is there need for init(...), shutdown(...) functions?
    * Could make some kind of "meta" - benchmark
    * (a benchmark running other benchmarks) possible... Ccool!
    */
};

/**
 * Module registration data structure. This struct has to be filled
 * by each communication module and passed to the ng_register_module(...)
 * function to be useable by the main program.
 */
struct ng_module {
   /**
    * Module name (e.g. "eth", "udp", "tcp", ...). This name must be
    * unique across all modules compiled in as it's for the user to
    * select the desired module via the essential "-m" option.
    */
   char *name;

   /** Short module description, displayed with usage information. */
   char *desc;

   /** Maximum amount of data this module can transmit (-1 for unlimited). */
   int  max_datasize;

   /** Space to allocate additionally to the data size for protocol headers. */
   int  headerlen;

   /** 
    * some FLAGS, includes:
    *  - NG_MOD_RELIABLE - is the module reliable or not. That means is
    *     the data transmission reliable, i.e. may messages be lost? The
    *     module guarantees reliability if it sets this flag. This flag
    *     does nothing but issue a warning at the startup of netgauge
    *     that the run might hang because of packet loss.
    *  - NG_MOD_CHANNEL - do we have channel semantics (i.e., does
    *     recvfrom receive data only from the right host? This is
    *     necessary for connection-less or datagram-based protocols. The
    *     problem there is that one needs to post a recvfrom() on the
    *     socket (e.g., UDP) and data can arrive from any host and go to
    *     the buffer. So data from a wrong host ends up being in the
    *     buffer. So if NG_MOD_CHANNEL is *not* set, the pattern can
    *     *not* rely on the correctness of the data in the
    *     receive-buffer. However, if this flag is set, the module
    *     guarantees correct data receiption in the recvfrom() call. )
    *  - NG_MOD_MEMREG - do we need to register memory before we send
    *     anything? So this flag is actually not used since the patterns
    *     *must* use the memory registration function of the selected
    *     module if it is not NULL. But may be used in the future.
    */
   char flags;

   /* this is the CPP Flag definition */
    #define NG_MOD_MEMREG   1
    #define NG_MOD_RELIABLE 2
    #define NG_MOD_CHANNEL  4

   /**
    * Module option parsing function. Gives the module the chance to grab
    * it's parameters from the command line and remember them in a
    * module-local data structure or stuff. This function pointer my be
    * void if it's not needed.
    */
   int  (*getopt)(int argc, char **argv, struct ng_options *global_opts);

   /**
    * Module specific memory allocation function. Is used to allocate
    * communication memory. May be null, the pattern has to check that.
    * Exists only to deal with certain memory registration issues.
    *
    * @return NULL on failure
    */
   void* (*malloc)(size_t size);

   /**
    * Module initialization function. MTU and headerlen MUST be set after
    * this function has been called. This pointer my be void if unneeded.
    *
    * @return non-zero on failure
    */
   int  (*init)(struct ng_options *global_opts);

   /**
    * Module shutdown function. This function should free any resources
    * the module holds, e.g. close network connections, free memory
    * and so on.
    */
   void (*shutdown)(struct ng_options *global_opts);
   
   /** 
    * Module usage information function. Should print out some instructions
    * for the user which parameters are accepted by this module.
    */
   void (*usage)(void);
   
   /**
    * Module should print out some man page informations
    */ 
   void (*writemanpage)(void);
   
   /**
    * Instructs the comm. module to send _size_ bytes from the buffer
    * beginning at _buffer_ to the peer with id _dst_. There is no
    * guaranty how many bytes will be transfered, even 0 bytes are
    * possible. If you need to transmit the whole buffer there's
    * the convience function "ng_send_all".
    *
    * @return the number of bytes sent with the current call,
    *    values < 0 indicate errors
    */
   int (*sendto)(int dst, void *buffer, int size);
   
   /**
    * Instructs the comm. module to receive from a peer. Semantics is
    * the same as for the sendto(...) function.
    *
    * @return the number of bytes received with the current call,
    *    values < 0 indicate errors
    */
   int (*recvfrom)(int src, void *buffer, int size);

   /**
    * Sets the module to use blocking/nonblocking behavior for th
    * usual sendto/recvfrom functions.
    *
    * @param do_block 1 -> enable blocking behavior (default) 0 -> disable it
    * @param partner the ID the the connection to work on
    * @return 1 is succeeded, 0 otherwise
    */
   int (*set_blocking)(int do_block, int partner);

   /**
    * like sendto but non-blocking. This means that it returns
    * immediately to the user with a handle that can be used later to
    * test or wait for completion of the transfer.
    * 
    * @return the number of bytes sent with the current call,
    *    values < 0 indicate errors
    * @return a request handle that identifies the outstanding message
    *    transfer request   
    */
   int (*isendto)(int dst, void *buffer, int size, NG_Request *req);
   
   /**
    * Instructs the comm. module to receive from a peer. Semantics is
    * the same as for the isendto(...) function.
    *
    * @return the number of bytes received with the current call,
    *    values < 0 indicate errors
    * @return a request handle that identifies the outstanding message
    *    transfer request   
    */
   int (*irecvfrom)(int src, void *buffer, int size, NG_Request *req);
   
   /**
    * Tests an outstanding message transfer request for completion.
    *
    * @return the completion status of the request (0 == complete)
    *    values < 0 indicate errors and > 0 indicates that the operation
    *    is still in progress
    */
   int (*test)(NG_Request *req);

   /**
    * TODO: some needful things:
    *   - generic address parsing function
    */
};

struct ng_comm_pattern_list {
   /** the communication pattern */
   struct ng_comm_pattern *pattern;
   
   /** the next list element */
   struct ng_comm_pattern_list *next;
};

/** flag to stop tests */
extern unsigned char g_stop_tests;

/** access to the global options */
extern struct ng_options g_options;

/* function prototypes */
int ng_register_module(struct ng_module *module);
void ng_option_usage(const char *opt, const char *desc, const char *param);
void ng_longoption_usage
(const char opt, const char *longopt, const char *desc, const char *param);
void ng_info(u_int8_t vlevel, const char *format, ...);
void ng_error(const char *format, ...);
void ng_perror(const char *format, ...);
void ng_exit(int retcode);
void sig_int(int signum);
void ng_abort(const char* error);
int ng_get_options(int *argc, char ***argv, struct ng_options *options);
void ng_get_optionstrings(int *argc, char ***argv, char **ngopts, char **ptrnopts, char **modeopts, char **allopts);
void get_next_testparams(long *data_size, 
			 long *test_count,
			 struct ng_options *options, struct ng_module *module);
int ng_readminmax(char *buf, unsigned long *min, unsigned long  *max);
void write_host_information(FILE *fd);
FILE *open_output_file(char *filename);

/* pattern function prototypes */
int register_pattern_overlap();
int register_pattern_Nto1();
int register_pattern_noise();
int register_pattern_1toN();
int register_pattern_distrtt();
int register_pattern_one_one();
int register_pattern_one_one_mpi_bidirect();
int register_pattern_one_one_sync();
int register_pattern_one_one_req_queue();
int register_pattern_one_one_dtype();
int register_pattern_one_one_perturb();
int register_pattern_one_one_all();                                    
int register_pattern_one_one_randtag();                                
int register_pattern_one_one_randbuf();                                
int register_pattern_memory();
int register_pattern_disk();
int register_pattern_loggp();
int register_pattern_nbov();
int register_pattern_esp_params();
int register_pattern_synctest();
int register_pattern_cpu();
int ng_register_pattern(struct ng_comm_pattern *pattern);
int ng_send_all(int dst, void *buffer, int size, const struct ng_module *module);
int ng_recv_all(int src, void *buffer, int size, const struct ng_module *module);

/* write manpage to STDOUT funktion */
void ng_manpage(void);
void ng_manpage_module(const char, const char *, const char *, const char *);

/* Marks a function as "not implemented" */
#define ng_not_impl() { \
	ng_error("This function is not implemented: %s() %s:%i",__FUNCTION__, __FILE__,__LINE__); \
	exit(1); \
} while(0);

/* maximum long value for time calculations */
#define MAXULONG 4294967296.0

/* simple math functions */
#define ng_min(a, b) (((a) < (b))? (a) : (b))
#define ng_max(a, b) (((a) > (b))? (a) : (b))

/* this macro implements the send loop until all data is sent
 * successfully */
#define NG_SEND(dst, buf, size, module) \
{                                       \
  int NG_SEND_sent = 0;                 \
  do {                                  \
    int NG_SEND_res;                \
    NG_SEND_res = module->sendto(dst, (void*)((char*)buf + NG_SEND_sent), size - NG_SEND_sent);    \
    if(NG_SEND_res < 0) ng_error("module->sendto returned %i", NG_SEND_res);   \
    NG_SEND_sent += NG_SEND_res;        \
  } while(NG_SEND_sent < size);         \
}
                                       

/* this macro implements the recv loop until all data is received
 * successfully */
#define NG_RECV(src, buf, size, module) \
{                                       \
  int NG_RECV_recvd = 0;                 \
  do {                                  \
    int NG_RECV_res;                \
    NG_RECV_res = module->recvfrom(src, (void*)((char*)buf + NG_RECV_recvd), size - NG_RECV_recvd);    \
    if(NG_RECV_res < 0) {               \
      ng_error("module->recvfrom returned %i", NG_RECV_res);   \
      break;                            \
    }                                   \
    NG_RECV_recvd += NG_RECV_res;       \
  } while(NG_RECV_recvd < size);        \
}


/* Netgauge Malloc macro - calls the module's malloc or the normal
 * malloc ... */
#define NG_MALLOC(module, type, size, ret)        \
{                                           \
  if(NULL == module->malloc)                \
    ret = (type) malloc(size);                     \
  else                                      \
    ret = (type) module->malloc(size);             \
  if(ret == NULL) {                         \
    ng_perror("Failed to allocate %d bytes", size); \
    ng_exit(10);                             \
  }                                         \
}
  


typedef struct req_handle {
   unsigned short type; /* send or receive */
   unsigned int index;  /* index of peer */
   int size;            /* size of request */
   void *buffer;        /* buffer to transfer */
   int remaining_bytes; /* number of remaining bytes */
} req_handle_t;

#define get_req_type(req) ((req_handle_t*)*req)->type
#define get_req_index(req) ((req_handle_t*)*req)->index
#define TYPE_SEND 1
#define TYPE_RECV 2




#ifdef __cplusplus
}
#endif

#endif /*NETGAUGE_H_*/


