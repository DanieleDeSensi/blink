/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Timo Schneider <timos@hrz.tu-chemnitz.de>
 *
 */

#include <assert.h>
#include "netgauge.h"

#if defined NG_MPI && defined NG_MX && defined NG_MOD_MX
#include "myriexpress.h" 

#define MSG_DONTWAIT 1

/* module function prototypes */
static int ng_mx_getopt(int argc, char **argv, struct ng_options *global_opts);
static int ng_mx_init(struct ng_options *global_opts);
static void ng_mx_shutdown(struct ng_options *global_opts);
void ng_mx_usage(void);
void ng_mx_writemanpage(void);
static int ng_mx_sendto(int dst, void *buffer, int size);
static int ng_mx_recvfrom(int src, void *buffer, int size);
static int ng_mx_isendto(int dst, void *buffer, int size, NG_Request *req);
static int ng_mx_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static int ng_mx_test(NG_Request *req);

extern struct ng_options g_options;

/* module registration data structure */
static struct ng_module mx_module = {
   .name         = "mx",
   .desc         = "Mode mx uses mx_iend/mx_irecv for data transmission.",
   .max_datasize = -1,			/* can send data of arbitrary size */
   .headerlen    = 0,			/* no extra space needed for header */
   .flags        = NG_MOD_RELIABLE | NG_MOD_CHANNEL,
   .malloc       = NULL,
   .getopt       = ng_mx_getopt,
   .init         = ng_mx_init,
   .shutdown     = ng_mx_shutdown,
   .usage        = ng_mx_usage,
   .sendto       = ng_mx_sendto,
   .recvfrom     = ng_mx_recvfrom,
   .isendto      = ng_mx_isendto,
   .irecvfrom    = ng_mx_irecvfrom,
   .test         = ng_mx_test,
   .writemanpage = ng_mx_writemanpage
};

/* module private data */
static struct mx_private_data {
	int send_flags;
	int recv_flags;
	int local_rank;
	mx_endpoint_t local_endpoint;
	mx_endpoint_addr_t* mx_endpoints;
	int num_mx_endpoints;
	uint32_t filter;
} module_data;


static int ng_mx_sendto(int dst, void *buffer, int size) {
  
  mx_return_t rc;
  mx_request_t req;
  mx_segment_t segm_list;
  mx_status_t stat;
  uint32_t result;

  /* variant using mx_wait - we could also implement a polling variant which might show lower latency but higher cpu overhead */
  
  segm_list.segment_ptr = buffer;
  segm_list.segment_length = size;
  
  rc = mx_isend(module_data.local_endpoint, &segm_list, 1, module_data.mx_endpoints[dst], (uint64_t) module_data.local_rank, NULL, &req);
  assert(rc == MX_SUCCESS);
  rc = mx_wait(module_data.local_endpoint, &req, MX_INFINITE, &stat, &result);
  assert(rc == MX_SUCCESS);
  assert(result != 0);
  
  return size;
}

static int ng_mx_isendto(int dst, void *buffer, int size, NG_Request *req) {
  
  mx_return_t rc;
  mx_segment_t segm_list;
  mx_request_t *mxreq;
  
  mxreq = (mx_request_t*) malloc(sizeof(mx_request_t));
  /* the NG_Request itself is a pointer to point to arbitrary module data */
  *req = (NG_Request*) mxreq;
  
  segm_list.segment_ptr = buffer;
  segm_list.segment_length = size;
  
  rc = mx_isend(module_data.local_endpoint, &segm_list, 1, module_data.mx_endpoints[dst], (uint64_t) module_data.local_rank, NULL, mxreq);
  assert(rc == MX_SUCCESS);
  
  return size;

}

static int ng_mx_irecvfrom(int src, void *buffer, int size, NG_Request *req) {
	
  mx_return_t rc;
  mx_segment_t segm_list;
  mx_request_t *mxreq;
  
  mxreq = (mx_request_t*) malloc(sizeof(mx_request_t));
  /* the NG_Request itself is a pointer to point to arbitrary module data */
  *req = (NG_Request*) mxreq;
  
  segm_list.segment_ptr = buffer;
  segm_list.segment_length = size;
  
  rc = mx_irecv(module_data.local_endpoint, &segm_list, 1, (uint64_t) src, MX_MATCH_MASK_NONE, NULL, mxreq);
  assert(rc == MX_SUCCESS);
  
  return size;

}

static int ng_mx_test(NG_Request *req) {
  
  mx_request_t *mxreq;
  mx_status_t stat;

  uint32_t result=0;
  
  /* req == NULL indicates that it is finished */
  if(*req == NULL) return 0;
  
  mxreq = (mx_request_t*) *req;

  mx_test(module_data.local_endpoint, mxreq, &stat, &result);

  if(result != 0) {
    free(*req);
    *req = NULL;
    return 0;
  }
  return 1;
}

static int ng_mx_recvfrom(int src, void *buffer, int size) {

  mx_return_t rc;
  mx_request_t req;
  mx_segment_t segm_list;
  mx_status_t stat;
  uint32_t result;

  /* variant using mx_wait - we could also implement a polling variant which might show lower latency but higher cpu overhead */
  // TODO make sure matching is correct - right now we just match anything...
  
  segm_list.segment_ptr = buffer;
  segm_list.segment_length = size;

  rc = mx_irecv(module_data.local_endpoint, &segm_list, 1, (uint64_t) src, MX_MATCH_MASK_NONE, NULL, &req);
  assert(rc == MX_SUCCESS);
  rc = mx_wait(module_data.local_endpoint, &req, MX_INFINITE, &stat, &result);
  assert(rc == MX_SUCCESS);
  assert(result != 0);
  
  return size;
}

int register_mx(void) {
   ng_register_module(&mx_module);
   return 0;
}

static int ng_mx_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;
   char *optchars = NG_OPTION_STRING "N:";	// additional module options
   extern char *optarg;
   extern int optind, opterr, optopt;
   
   int failure = 0;
   

   if (!global_opts->mpi) {
      ng_error("Mode mx requires execution via MPI");
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
   
   // report success or failure
   return failure;
}

static int ng_mx_init(struct ng_options *global_opts) {

	mx_return_t rc;
	mx_endpoint_addr_t loc_endp_adr;
	uint64_t* nic_ids;
	uint32_t* endpoint_ids;
	uint64_t loc_nicid;
	uint32_t loc_endpid;
	int cnt;

	
	ng_info(NG_VLEV2, "Initializing MX module\n", cnt);
	
	/* set mx packet filter so that we won't get unexpected messages */
	// TODO: actually this should be random but the same value for every process
	module_data.filter = 0xcafebabe;
	
	/* initialize mx library */
	rc = mx_init();
	assert(rc == MX_SUCCESS);

	
	/* make room for datastructure that holds the connections later */
	MPI_Comm_size(MPI_COMM_WORLD, &(module_data.num_mx_endpoints));
	MPI_Comm_rank(MPI_COMM_WORLD, &(module_data.local_rank));
	module_data.num_mx_endpoints = module_data.num_mx_endpoints;
	module_data.mx_endpoints = (mx_endpoint_addr_t*) malloc(module_data.num_mx_endpoints * sizeof(mx_endpoint_addr_t));
	assert(module_data.mx_endpoints != NULL);


	/* make room for remote nic and endpoint ids */
	nic_ids = (uint64_t*) malloc(module_data.num_mx_endpoints * sizeof(uint64_t));
	endpoint_ids = (uint32_t*) malloc(module_data.num_mx_endpoints * sizeof(uint32_t));


	/* get local endpoint data */
	rc = mx_open_endpoint(MX_ANY_NIC, MX_ANY_ENDPOINT, module_data.filter, 0, 0, &module_data.local_endpoint);
	assert(rc == MX_SUCCESS);
	rc = mx_get_endpoint_addr(module_data.local_endpoint, &loc_endp_adr);
	assert(rc == MX_SUCCESS);
	rc =  mx_decompose_endpoint_addr(loc_endp_adr, &loc_nicid, &loc_endpid);
	assert(rc == MX_SUCCESS);


	/* distribute endpoint data */
	MPI_Allgather( &loc_nicid, 8, MPI_BYTE, nic_ids, 8, MPI_BYTE, MPI_COMM_WORLD);
	MPI_Allgather( &loc_endpid, 4, MPI_BYTE, endpoint_ids, 4, MPI_BYTE, MPI_COMM_WORLD);
	

	/* connect to remote nodes */ 	
	// TODO: There is also a nonblocking variant, but the interface is wired, it doesn't return an mx_endpoint_addr_t for the new connection...
	for (cnt = 0; cnt < module_data.num_mx_endpoints; cnt++) {
		ng_info(NG_VLEV2, "Attempt to connect to rank %i", cnt);
		rc = mx_connect(module_data.local_endpoint, nic_ids[cnt], endpoint_ids[cnt], module_data.filter, 5, &(module_data.mx_endpoints[cnt]));
		assert(rc == MX_SUCCESS);
	}

	/* free temp storage */
	free(nic_ids);
	free(endpoint_ids);

   /* report success */
   return 0;
}

void ng_mx_shutdown(struct ng_options *global_opts) {
	
	int cnt;

	ng_info(NG_VLEV2, "Shutting down MX module\n");

	// we will terminate the mx connections, so we have to ensure that nobody is in
	// a send/recv/etc. but everybody called ng_mx_shutdown - so we need a barrier
	sleep(1);
	MPI_Barrier(MPI_COMM_WORLD);
	
	for (cnt = 0; cnt < module_data.num_mx_endpoints; cnt++) {
		mx_disconnect(module_data.local_endpoint, module_data.mx_endpoints[cnt]);
	}
	free(module_data.mx_endpoints);
}


/* module specific usage information */
void ng_mx_usage(void) {
}

/* module specific manpage information */
void ng_mx_writemanpage(void) {
}

#else

/* module registration */
int register_mx(void) {
   /* do nothing if MPI is not compiled in */
   return 0;
}

#endif  

