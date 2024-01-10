/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_MPI && defined NG_GM && defined NG_MOD_GM

#include <gm.h>

#define GM_MAX_BOARD_NUM 3 
#define GM_MAX_PORT_NUM  7 

#define GM_TRANSPORT_SENDRECV 1 
#define GM_TRANSPORT_RDMA     2 

#define GM_MAX_RECV_BUF_COUNT 5 

#define GM_BUFFER_REGISTER_MEMORY  1 
#define GM_BUFFER_PREALLOC         2 
#define GM_BUFFER_PREALLOC_NOCOPY  3 


typedef struct {
    unsigned int node_id;
    unsigned int send_port_id;
    unsigned int recv_port_id;
    long         recv_buffer;
} gm1_address_t;
	
	
typedef struct {
    char            *buffer;
    unsigned int    bufsize;
    unsigned int    msglength;
    unsigned int    sender_node_id;
    unsigned int    sender_port_id;
} gm1_recv_info_t;


/* global options */
extern struct ng_options g_options;

/* module function prototypes */
static int  gm1_getopt(int argc, char **argv, struct ng_options *global_opts);
static int  gm1_init(struct ng_options *global_opts);
static void gm1_shutdown(struct ng_options *global_opts);
static void gm1_usage(void);
static int  gm1_sendto(int dst, void *buffer, int size);
static int  gm1_recvfrom(int src, void *buffer, int size);
static void gm1_send_callback(struct gm_port *port, void *the_context, gm_status_t status);


/* module registration data structure */
static struct ng_module gm1_module = {
  .name           = "gm",
  .desc           = "Mode gm uses Myrinet/GM for data transmission.",
  .max_datasize   = -1,                    /* can send data of arbitrary size */
  .headerlen      = 0,                     /* no extra space needed for header */
  .flags          = NG_MOD_MEMREG | NG_MOD_RELIABLE | NG_MOD_CHANNEL,
  .getopt         = gm1_getopt,
  .init           = gm1_init,              /* FIX? no shutdown function call without init function */
  .shutdown       = gm1_shutdown,
  .usage          = gm1_usage,
  .sendto         = gm1_sendto,
  .recvfrom       = gm1_recvfrom,
  .isendto        = NULL,
  .irecvfrom	  = NULL,
  .test           = NULL,
  .writemanpage   = NULL
};


/* module private data */
static struct gm1_private_data {
  struct gm_port *send_port;                             /* local gm port for sending data              */
  struct gm_port *recv_port;                             /* local gm port for receiving data            */
  char           *send_buf_p;                            /* send buffer                                 */
  char			 *dummy_buf_p;							 /* dummy buffer used for memory registration   */
  char           *recv_buf_p_arr[GM_MAX_RECV_BUF_COUNT]; /* receive buffers 							*/
  int            max_num_recv_buffers;                   /* number of receive buffers to use 			*/
  int            num_recv_buffers;                       /* number of receive buffers already allocated */
  gm1_address_t  *addresses;                             /* array of peer addresses 					*/
  unsigned long  max_datasize;                           /* data size to allocate and pin 				*/
  unsigned int   gm_buf_size;                            /* size of buffer to use on receiver side      */
  int            pending_sends;                          /* number of pending sends                     */
  int            transport_mode;                         /* send/recv or rdma                           */
  int            buffer_mode;                            /* use preallocated buffers or register memory */
  int            mpi_partner;                            /* MPI_COMM_WORLD rank of parnter process      */
  MPI_Comm       mpi_comm;                               /* mpi communicator to use                     */
  int            mpi_size;                               /* size of mpi communicator                    */
} module_data;


/* nothing to do here */
static int gm1_init(struct ng_options *global_opts)
{
/* setup connections between peers */
  int board_num, port_num, i;
  gm1_address_t my_address;
  unsigned long local_addr, rem_addr;
  module_data.mpi_size = g_options.mpi_opts->worldsize;
  module_data.mpi_comm = MPI_COMM_WORLD;
  module_data.max_datasize = g_options.max_datasize;
  int open_ports = 0;
  struct gm_port *tmp_port;

  /* Initialize GM */
  if (GM_SUCCESS != gm_init())
  {
    ng_error("Couldn't init GM");
    goto end_error;
  }

  /* open a send and a recv port */
  for (board_num = 0; board_num <= GM_MAX_BOARD_NUM; board_num++)
  {
    /* open the first available gm port for this board */
    for (port_num = 2; port_num <= GM_MAX_PORT_NUM; port_num++)
    {
      if (port_num == 3) continue;  /* port 3 reserved */
      if (GM_SUCCESS == gm_open(&tmp_port, board_num, port_num, NULL, GM_API_VERSION))
      {
        open_ports++;
        if (open_ports == 1)
        {
        	/* send port */
          ng_info(NG_VLEV1 | NG_VPALL, "opened send port %d on board %d", port_num, board_num);
          module_data.send_port = tmp_port;
          module_data.pending_sends = 0;
          my_address.send_port_id = port_num;
          continue;
        }
        else
        {
        	/* receive port */
          ng_info(NG_VLEV1 | NG_VPALL, "opened recv port %d on board %d", port_num, board_num);
          module_data.recv_port = tmp_port;
          my_address.recv_port_id = port_num;
          break;
        }
      }
    }
    if (open_ports == 2)
      break;
  }
  if (open_ports != 2)
  {
    ng_error("Could not open send/recv port");
    goto end_error;
  }

  /* if rmda transport selected allow remote memory access */
  if (module_data.transport_mode == GM_TRANSPORT_RDMA)
  {
    if (GM_SUCCESS != gm_allow_remote_memory_access(module_data.recv_port))
    {
        ng_error("Could not enable remote memory access");
        goto end_error;
    }
  }

  /* set buffer size for usage in send/recv. must match on sender and receiver */
  module_data.gm_buf_size = gm_min_size_for_length(module_data.max_datasize);

  /*  get local node id */
  if (GM_SUCCESS != gm_get_node_id(module_data.recv_port, &my_address.node_id))
  {
    ng_info(NG_VLEV2 | NG_VPALL, "unable to get node id");
    goto end_error;
  }
  
  /* translate local id to global id */
  if (GM_SUCCESS != gm_node_id_to_global_id(module_data.recv_port, my_address.node_id, &my_address.node_id))
  {
    ng_info(NG_VLEV2 | NG_VPALL, "unable to translate node id to global id");
    goto end_error;
  }

	/* allocate and pin send buffer */
	module_data.send_buf_p = gm_dma_malloc(module_data.send_port, module_data.max_datasize);
	if (module_data.send_buf_p == NULL)
	{
	  ng_error("Could not allocate %d bytes for send memory", module_data.max_datasize);
	  goto end_error;
	}

  if (module_data.buffer_mode == GM_BUFFER_REGISTER_MEMORY)
  {
	 /* allocate a dummy buffer, that we use for memory registration */
    module_data.dummy_buf_p = gm_dma_malloc(module_data.send_port, module_data.max_datasize);
    if (module_data.dummy_buf_p == NULL)
    {
      ng_error("Could not allocate %d bytes for send memory", module_data.max_datasize);
      goto end_error;
    }
  }
  else
  {
	/* if we don't register the memory in the send/recv functions we must use preallocated and pinned buffers for transfer */
    /* allocate and pin recv buffer(s) */
    char* buf;
    for (i = 0; i < module_data.max_num_recv_buffers; i++)
    {
      buf = gm_dma_calloc(module_data.recv_port, module_data.max_datasize, sizeof(char));
      if (buf == NULL)
      {
        ng_error("Could not allocate %d bytes for recv memory", module_data.max_datasize);
        goto end_error;
      }
      module_data.recv_buf_p_arr[module_data.num_recv_buffers++] = buf;
	  /* gm may receive any message into this buffer as long as the size and priority fields of the   */
	  /* received message exactly match the size and priority                                         */
      /* for rdma, receiver side doesn't need to inform gm about receive buffers                      */
      if (module_data.transport_mode != GM_TRANSPORT_RDMA)
      	gm_provide_receive_buffer(module_data.recv_port, buf, module_data.gm_buf_size, GM_LOW_PRIORITY);
    }
    my_address.recv_buffer = (long)module_data.recv_buf_p_arr[0]; /* this buffer will be used for rdma transfers to this process */
  }

  /* allocate memory for peer addresses */
  module_data.addresses = (gm1_address_t*)malloc(module_data.mpi_size * sizeof(gm1_address_t));
  if (module_data.addresses == NULL)
  {
    ng_error("Could not allocate memory for addresses array");
    goto end_error;
  }

  /* distribute addresses to all clients */
  if (MPI_Allgather(&my_address, sizeof(gm1_address_t), MPI_CHAR, module_data.addresses, sizeof(gm1_address_t), MPI_CHAR, module_data.mpi_comm) != MPI_SUCCESS)
  {
    ng_error("Could not distribute peer adresses.");
    goto end_error;
  }

  /* convert received global ids to node ids */
  for (i = 0; i < module_data.mpi_size; i++)
  {
    if (GM_SUCCESS != gm_global_id_to_node_id(module_data.recv_port, module_data.addresses[i].node_id, &module_data.addresses[i].node_id))
    {
      ng_error("unable to resolve global id to node id for peer %d", i);
      goto end_error;
    }
    ng_info(NG_VLEV1, "peer %d node id is %d", i, module_data.addresses[i].node_id);
  }

  /* report success */
  return 0;

  /* report failure */
end_error:
	return 1; 
}


/* send function */
static int gm1_sendto(int dst, void *buffer, int size)
{
  gm_recv_event_t *gm_event;
  char* send_buffer;
  int tmp;

  ng_info(NG_VLEV2 | NG_VPALL, "start -> gm1_sendto node %d",dst);

  /* we always use the preallocated buffer for send */
  send_buffer = module_data.send_buf_p;

  if (module_data.buffer_mode == GM_BUFFER_PREALLOC)
  {
    ng_info(NG_VLEV2 | NG_VPALL, "copying user data to pinned buffer");
    memcpy(module_data.send_buf_p, buffer, size);
  }
  else
  if (module_data.buffer_mode == GM_BUFFER_REGISTER_MEMORY)
  {
	/* register the dummy buffer to simulate registration costs */
	/* the buffer we send does not contain the actual user data, but who cares, its just a speed test */
    ng_info(NG_VLEV2 | NG_VPALL, "registering memory");
    gm_register_memory(module_data.send_port, module_data.dummy_buf_p, size);
  }

  if (module_data.transport_mode == GM_TRANSPORT_RDMA)
  {
    ng_info(NG_VLEV2 | NG_VPALL, "calling gm_directed_send_with_callback()");
		/* set the 'transfer-complete-marker' to 0 in the receive buffer    */
		/* this buffer will be used after the send to receive the data back */
	  *(char*)(module_data.recv_buf_p_arr[0]+size-1) = 0;
    /* marker in receive buffer will be '1' if transfer is complete */
    module_data.send_buf_p[size-1] = 1;
    gm_directed_send_with_callback(
      module_data.send_port,
      send_buffer,
      module_data.addresses[dst].recv_buffer, /* address to write to on receiver side */
      size,
      GM_LOW_PRIORITY,
      module_data.addresses[dst].node_id,
      module_data.addresses[dst].recv_port_id,
      gm1_send_callback, /* function, that will be called by the gm_unknown() function, if this send completes */
      NULL);
  }
  else
  {
    ng_info(NG_VLEV2 | NG_VPALL, "calling gm_send_with_callback()");
    gm_send_with_callback(
      module_data.send_port,
      send_buffer,
      module_data.gm_buf_size, /* value will be used by gm on receiver side to match the message with a registered receive buffer */
      size, /* length of user data */
      GM_LOW_PRIORITY,
      module_data.addresses[dst].node_id,
      module_data.addresses[dst].recv_port_id,
      gm1_send_callback,
      NULL);
  }

  ng_info(NG_VLEV2 | NG_VPALL, "waiting for send completion...");

  module_data.pending_sends++;

  /* poll for event on send port, process all send completion events */

repeat:
	tmp = module_data.pending_sends;
    gm_event = gm_receive(module_data.send_port);
    switch (GM_RECV_EVENT_TYPE(gm_event))
	{
      case GM_HIGH_RECV_EVENT:
	  case GM_RECV_EVENT:
    	    ng_error ("gm1_sendto(): Receive Event UNEXPECTED on send port");
            return -1;
	  case GM_NO_RECV_EVENT:
            break;
	  default:
    	    gm_unknown(module_data.send_port, gm_event);
 	    	if (module_data.pending_sends < tmp && module_data.pending_sends > 0)
			{
				goto repeat;
			}
	}
  
  if (module_data.buffer_mode == GM_BUFFER_REGISTER_MEMORY)
    gm_deregister_memory(module_data.send_port, module_data.dummy_buf_p, size);

  return size;
}

/* callback function for send completion */
static void gm1_send_callback(struct gm_port *port, void *the_context, gm_status_t status)
{
  switch (status)
  {
    case GM_SUCCESS:
      break;
    default:
      ng_error("Send completed with error");
  }
  module_data.pending_sends--;
}

/* wait for a reveive on the given port and fill the gm1_recv_info_t struct with */
/* infos about the received message                                              */
/* will be used by the receiver in send/recv mode                                */
#define WAIT_FOR_RECV(port, recv_info_struct) (                                             \
{                                                                                           \
  gm_recv_event_t *gm_event;                                                                \
  int expect_recv = 1;                                                                      \
  while (expect_recv)                                                                       \
  {                                                                                         \
    gm_event = gm_receive(port);                                                            \
    switch (GM_RECV_EVENT_TYPE(gm_event))                                                   \
    {                                                                                       \
      case GM_HIGH_RECV_EVENT:                                                              \
      case GM_RECV_EVENT:                                                                   \
        recv_info_struct.sender_node_id = gm_ntoh_u16(gm_event->recv.sender_node_id);       \
        recv_info_struct.sender_port_id = gm_ntoh_u8(gm_event->recv.sender_port_id);        \
        recv_info_struct.buffer         = gm_ntohp (gm_event->recv.buffer);                 \
        recv_info_struct.bufsize        = gm_ntoh_u8(gm_event->recv.size);                  \
        recv_info_struct.msglength      = gm_ntoh_u32(gm_event->recv.length);               \
        expect_recv = 0;                                                                    \
        break;                                                                              \
      case GM_NO_RECV_EVENT:                                                                \
        break;                                                                              \
      default:                                                                              \
        gm_unknown(module_data.recv_port, gm_event);                                        \
    }                                                                                       \
  }                                                                                         \
})


/* poll on the marker, that indicates rmda transfer is complete */
/* will be used by the receiver in rdma mode                    */
#define WAIT_FOR_RDMA(port, recv_info_struct, size) (                                       \
{                                                                                           \
  volatile char* marker = module_data.recv_buf_p_arr[0]+size-1;                             \
  while (!*marker);                                                                         \
  *marker = 0;																				\
  recv_info_struct.buffer = module_data.recv_buf_p_arr[0];                                  \
  recv_info_struct.msglength = size;                                                        \
})


/* function to receive a message */
static int gm1_recvfrom(int src, void *buffer, int size)
{
  gm1_recv_info_t recv_info;

  ng_info(NG_VLEV2 | NG_VPALL, "start -> gm1_recvfrom node %d", src);

  if (module_data.buffer_mode == GM_BUFFER_REGISTER_MEMORY)
  {   
    /* pin given buffer */
    gm_register_memory(module_data.recv_port, buffer, size);
    /* inform gm about this receive buffer */
    gm_provide_receive_buffer (module_data.recv_port, buffer, module_data.gm_buf_size, GM_LOW_PRIORITY);
  }

	/* wait for the message */
  if (module_data.transport_mode == GM_TRANSPORT_RDMA)
  {
    WAIT_FOR_RDMA(module_data.recv_port, recv_info, size);
  }
  else
  {
    WAIT_FOR_RECV(module_data.recv_port, recv_info);
    if (recv_info.sender_node_id != module_data.addresses[src].node_id || recv_info.sender_port_id != module_data.addresses[src].send_port_id)
    {
      ng_error ("gm1_recvfrom(): sender is not peer %d", src);
      return -1;
    }
  }

  if (recv_info.msglength != size)
  {
    ng_error("gm1_recvfrom: unexpected message length");
    return -1;
  }
  
  switch (module_data.buffer_mode)
  {
    case GM_BUFFER_REGISTER_MEMORY:
	  /* unpin given buffer */
      gm_deregister_memory(module_data.recv_port, buffer, size);
      break;
    case GM_BUFFER_PREALLOC:
    	/* copy data from receive buffer to the given buffer */
      memcpy(buffer,recv_info.buffer, size);
    case GM_BUFFER_PREALLOC_NOCOPY:
    	/* inform gm, that it can reuse this buffer for receives */
      if (module_data.transport_mode == GM_TRANSPORT_SENDRECV)
        gm_provide_receive_buffer (module_data.recv_port, recv_info.buffer, recv_info.bufsize, GM_LOW_PRIORITY);
  }

  ng_info(NG_VLEV1 | NG_VPALL, "end -> gm1_recvfrom");

  return size;
}


/* module registration */
int register_gm(void) {
  ng_register_module(&gm1_module);
  return 0;
}


/* parse command line parameters for module-specific options */
static int gm1_getopt(int argc, char **argv, struct ng_options *global_opts)
{
  int c;
  char *optchars = NG_OPTION_STRING "RM:B:";  /* additional module options */
  extern char *optarg;
  extern int optind, opterr, optopt;

  ng_info(NG_VLEV1 | NG_VPALL, "start -> gm1_getopt");

  /* gm can only be used in combination with mpi because we send */
  /* and receive the gm node ids via MPI to each other           */
  if (!g_options.mpi) {
    ng_error("gm: require global option '-p' (execution via MPI)");
    return 1;
  }

  /* data initialization */
  memset(&module_data, 0, sizeof(module_data));

  /* set default values */
  module_data.transport_mode       = GM_TRANSPORT_SENDRECV;	/* send/recv for transfer                */
  module_data.buffer_mode          = GM_BUFFER_PREALLOC;    /* use preallocated buffers for transfer */
  module_data.max_num_recv_buffers = 3;                     /* use 3 receive buffers                 */

  /* get command-line arguments */
  while ((c = getopt(argc, argv, optchars)) >= 0)
  {
    switch (c)
    {
      case '?': /* unrecognized or badly used option */
        if (!strchr(optchars, optopt))
          continue; /* unrecognized */
        ng_error("gm: option %c requires an argument", optopt);
        return 1;
      case 'R': /* use rdma for transfer */
        module_data.transport_mode = GM_TRANSPORT_RDMA;
        break;
      case 'M': /* memory allocation method */
        switch (optarg[0])
        {
          case 'a':
            module_data.buffer_mode = GM_BUFFER_PREALLOC;
            break;
          case 'n':
            module_data.buffer_mode = GM_BUFFER_PREALLOC_NOCOPY;
            break;
          case 'r':
            module_data.buffer_mode = GM_BUFFER_REGISTER_MEMORY;
            break;
          default:
            ng_error("gm: only 'a', 'n', or 'r' are possible arguments for option -M");
            return 1; 
        }
        break;
      case 'B': /* number of receive buffers to use (1 - GM_MAX_RECV_BUF_COUNT) */
        module_data.max_num_recv_buffers = atoi(optarg);
        if (module_data.max_num_recv_buffers < 1 || module_data.max_num_recv_buffers > GM_MAX_RECV_BUF_COUNT)
        {
          ng_error("gm: number of receive buffers must be in range 1 to %d",GM_MAX_RECV_BUF_COUNT);
          return 1;
        }
    }
  }

  /* check arguments */
  if (module_data.transport_mode == GM_TRANSPORT_RDMA)
  {
    if (strcmp(g_options.pattern,"one_one") != 0)
    {
      ng_error("gm: rdma mode works with 1:1 communication pattern only");
      return 1;
    }
    if (module_data.buffer_mode == GM_BUFFER_REGISTER_MEMORY)
    {
      ng_error("gm: rdma mode doesn't work together with register memory");
      return 1;
    }
    module_data.max_num_recv_buffers = 1; /* if we use rdma, we transfer always to the same buffer on the receiver side */
  }
/*
  if (module_data.buffer_mode == GM_BUFFER_REGISTER_MEMORY && strcmp(g_options.pattern,"one_one") != 0)
  {
    ng_error("gm: the register memory function can be used with 1:1 communication pattern only");
    return 1;
  }
*/
  /* some verbose output */
  if (module_data.transport_mode == GM_TRANSPORT_RDMA)
    ng_info(NG_VLEV2, "gm: using transport mode rdma");
  else
    ng_info(NG_VLEV2, "gm: using transport mode send/recv");

  switch (module_data.buffer_mode)
  {
    case GM_BUFFER_PREALLOC:
      ng_info(NG_VLEV2, "gm: allocate and pin transfer buffer(s) on startup");
      ng_info(NG_VLEV2, "gm: using %d receive buffer(s)",module_data.max_num_recv_buffers);
      break;
    case GM_BUFFER_PREALLOC_NOCOPY:
      ng_info(NG_VLEV2, "gm: allocate and pin transfer buffer(s) on startup without data copy in send/recv functions");
      ng_info(NG_VLEV2, "gm: using %d receive buffer(s)",module_data.max_num_recv_buffers);
      break;
    case GM_BUFFER_REGISTER_MEMORY:
      ng_info(NG_VLEV2, "gm: registering memory in send/recv functions");
      break;
  }

  /* report success */
  return 0;
}


/* module specific shutdown */
void gm1_shutdown(struct ng_options *global_opts)
{
  int i;
  ng_info(NG_VLEV2 | NG_VPALL, "start -> gm1_shutdown()");
  if (module_data.send_buf_p)
    gm_dma_free(module_data.send_port, module_data.send_buf_p);
  for (i = 0; i < module_data.num_recv_buffers; i++)
    gm_dma_free(module_data.recv_port, module_data.recv_buf_p_arr[i]);
  free(module_data.addresses);
  if (module_data.send_port)
    gm_close(module_data.send_port);
  if (module_data.recv_port)
    gm_close(module_data.recv_port);
  gm_finalize();
  ng_info(NG_VLEV2 | NG_VPALL, "end -> gm1_shutdown()");
}


/* module specific usage information */
void gm1_usage(void) {
  ng_option_usage("-R", "use rdma for transport", "");
  ng_option_usage("-M", "use different memory allocation method", "{a,n,r}");
  ng_option_usage("",   "  allocate and pin transfer buffer(s) on startup", "a");
  ng_option_usage("",   "  allocate and pin transfer buffer(s) on startup without copying the user data in the send/recv functions", "n");
  ng_option_usage("",   "  register memory in send/recv functions", "r");
  ng_option_usage("-B", "number of receive buffers to use (ignored in rdma mode)", "num");
}


#else /* #if defined NG_MPI && defined NG_GM */

/* do nothing if MPI is not compiled in */
int register_gm(void) {
  return 0;
}

#endif /* #if defined NG_MPI && defined NG_GM */

