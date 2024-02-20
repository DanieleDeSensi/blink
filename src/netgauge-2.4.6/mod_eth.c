/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */


#include "netgauge.h"
#ifdef NG_MOD_EHT

#include "eth_helpers.h"

#include <string.h>		/* memset & co. */
#include <sys/socket.h>		/* socket operations */
#if 0
// Why the fuck is this in here!? It's already in eth_helpers.h
#include <netinet/ether.h>	/* ethernet address, conversion functions, constants */
#endif
#include <netinet/in.h>
#include <sys/ioctl.h>		/* ioctl constants */
#include <net/if.h>		/* struct ifreq {for ioctl()} */
#ifdef HAVE_LINUX_IF_PACKET_H
#include <linux/if_packet.h>	/* for socked statistics */
#endif
#include <sys/fcntl.h>

#define ETH_DEFAULT_ETHER_TYPE 0x8888

/* module function prototypes */
static int eth_getopt(int argc, char **argv, struct ng_options *global_opts);
static int eth_init(struct ng_options *global_opts);
static void eth_shutdown(struct ng_options *global_opts);
static void eth_usage(void);
static void eth_writemanpage(void);

#ifdef SIOCGIFHWADDR

static int eth_set_blocking(int do_block, int partner);
static int eth_sendto(int dst, void *buffer, int size);
static int eth_recvfrom(int src, void *buffer, int size);
static int eth_isendto(int dst, void *buffer, int size, NG_Request *req);
static int eth_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static int eth_test(NG_Request *req);
static int eth_send_once(int dst, void *buffer, int size);
static int eth_recv_once(int src, void *buffer, int size);

/* module registration data structure */
static struct ng_module eth_module = {
   .name         = "eth",
   .desc         = "Mode eth uses raw ethernet sockets for data transmission.",
   .max_datasize = 65535 - sizeof(struct ethhdr),			/* can send data of arbitrary size */
   .headerlen    = 0,			/* no extra space needed for header */
   .flags        = 0,
   .getopt       = eth_getopt,
   .malloc       = NULL,
   .init         = eth_init,
   .shutdown     = eth_shutdown,
   .usage        = eth_usage,
   .writemanpage = eth_writemanpage,
   .sendto       = eth_sendto,
   .recvfrom     = eth_recvfrom,
   .isendto      = eth_isendto,
   .irecvfrom    = eth_irecvfrom,
	 .test         = eth_test
};

/**
 * getopt long options for eth
 */
static struct option long_options_eth[]={
	{"interface",				required_argument,	0, 'I'},
	{"protocol",				required_argument,	0, 'P'},
	{"remoteaddr",			required_argument,	0, 'R'},
	{"nonblocking",			required_argument,	0, 'N'},
	{0, 0, 0, 0}
};

/**
 * array of descriptions for eth
 */
static struct option_info long_option_infos_eth[]={
	{"network interface to use", "NAME"},
	{"ethernet frame protocol field value", "VALUE"},
	{"ethernet (mac) address of the server (client mode only)", "PEER"},
	{"nonblocking [s]end and/or [r]eceive mode", "[s][r]"},
};

/* module private data */
static struct eth_private_data {
   char                        *interface;
   unsigned int                ifindex;
   unsigned int                mtu;
   int                         sock;
   unsigned short              protocol;
   int                         recv_buf;
   int                         send_buf;
   int                         recv_flags;
   int                         send_flags;
   struct sockaddr_ll          address; //For the socket (ll means "link level")
   struct ether_addr           local_addr, remote_addr; //remote_addr is for non MPI
   struct ether_addr					 *addresses; //addresses of all peers (indexed by rank)
   const struct ng_mpi_options *mpi_opts;
	 unsigned int                *rcvd_bytes;
	 req_handle_t                **requests;
} module_data;


/* module registration */
int register_eth(void) {
   /* this module is not working yet in NGv2! */
   ng_register_module(&eth_module);
   return 0;
}


/* parse command line parameters for module-specific options */
static int eth_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;
   
   char *optchars = "I:P:R:N:";  // additional module options
   
   extern char *optarg;
   extern int optind, opterr, optopt;
   int option_index = 0;
   struct ether_addr no_addr;

   int failure = 0;

   memset(&module_data, 0, sizeof(module_data));
   memset(&no_addr, 0, sizeof(no_addr));

	/* parse module-options */
   while((c=getopt_long(argc, argv, optchars, long_options_eth, &option_index)) 
   		>= 0 ) {
      switch( c ) {
      case '?':	/* unrecognized or badly used option */
         if (!strchr(optchars, optopt))
            continue;	/* unrecognized */
         ng_error("Option %c requires an argument", optopt);
         failure = 1;
         break;
      case 'I':	/* network interface */
         module_data.interface = optarg;
         break;
      case 'P':	/* protocol / ethernet packet type */
         module_data.protocol = atoi(optarg);
         break;
      case 'R':	/* remote address */
         if (global_opts->mpi > 0) {
            ng_error("Option -R is not available when using MPI - the remote address is determined automatically");
            failure = 1;
         } else {
            if (!ether_aton_r(optarg, &module_data.remote_addr)) {
               ng_error("Malformed ethernet address %s", optarg);
               failure = 1;
               continue;
            }
            ng_info(NG_VLEV2 | NG_VPALL, "Set remote ethernet address %s", ether_ntoa(&module_data.remote_addr));
         }
         break;
      case 'N':	/* non-blocking send/receive mode */
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
   if (!failure) {
      if (!module_data.protocol) {
         module_data.protocol = ETH_DEFAULT_ETHER_TYPE;
         ng_info(NG_VLEV2, "No protocol specified - using default %#x", module_data.protocol);
      }

      if (!module_data.interface) {
         ng_error("No interfac given for mode %s - use option -I", eth_module.name);
         failure = 1;
      }

      if (!global_opts->mpi && !global_opts->server && !memcmp(&module_data.remote_addr, &no_addr, ETH_ALEN)) {
         ng_error("No remote address given for mode %s - use option -R", eth_module.name);
         failure = 1;
      }
   }

   // report success or failure
   return failure;
}

static int eth_set_blocking(int do_block, int partner)
{
   int flags = fcntl(module_data.sock, F_GETFL);
   if (flags < 0) {
      ng_perror("Could not get flags");
      return 0;
   }
   if (do_block) {
      flags &= ~O_NONBLOCK;     //clear flag
      if (fcntl(module_data.sock, F_SETFL, flags)) {
         ng_perror("Could not set blocking operation for ETH");
         return 0;
      }
   } else {
      flags |= O_NONBLOCK;      //set flag
      if (fcntl(module_data.sock, F_SETFL, flags) < 0) {
         ng_perror("Could not set non-blocking operation for ETH");
         return 0;
      }
   }
   return 1;
}

/* module specific benchmark initialization */
static int eth_init(struct ng_options *global_opts) {
#ifdef NG_MPI
   struct ifreq if_data;
   socklen_t optlen;
   int optval;
	 const int peer_count = g_options.mpi_opts->worldsize;
	 module_data.mpi_opts = g_options.mpi_opts;

	 module_data.addresses   = calloc(peer_count, sizeof(struct ether_addr));
	 module_data.rcvd_bytes  = calloc(peer_count, sizeof(unsigned int));
	 module_data.requests = calloc(peer_count, sizeof(req_handle_t*));
	 memset(module_data.requests, 0, sizeof(req_handle_t*)*peer_count);
		
   // initialize network socket
   ng_info(NG_VLEV2, "Initializing network socket");
   module_data.sock = socket(PF_PACKET, SOCK_DGRAM, htons(module_data.protocol));
   if (module_data.sock <= 0) {
      ng_perror("Mode %s could not create network socket", eth_module.name);
      return 1;
   }

   // set maximum possible read/write memory
   optval = global_opts->max_datasize;
   setsockopt(module_data.sock, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
   setsockopt(module_data.sock, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
   optlen = sizeof(int);
   if (getsockopt(module_data.sock, SOL_SOCKET, SO_RCVBUF, &module_data.recv_buf, &optlen) < 0 ||
       getsockopt(module_data.sock, SOL_SOCKET, SO_SNDBUF, &module_data.send_buf, &optlen) < 0) {
      ng_perror("Mode %s could not determine the socket's send or receive buffer size",
                eth_module.name);
      return 1;
   }
   ng_info(NG_VLEV2, "Set receive buffer to %d, send buffer to %d bytes",
           module_data.recv_buf, module_data.send_buf);

   // determine local interface index
   module_data.ifindex = if_nametoindex(module_data.interface);
   if (module_data.ifindex <= 0) {
      ng_perror("Mode %s could not determine index of local interface %s",
                eth_module.name, module_data.interface);
      return 1;
   }
   ng_info(NG_VLEV2 | NG_VPALL, "Determined local interface %s index %d",
           module_data.interface, module_data.ifindex);

   // determine local interface ethernet (mac) address
	 get_if_addr(module_data.sock, module_data.interface, &module_data.local_addr);

   // determine local interface mtu
   memset(&if_data, 0, sizeof(if_data));
   strncpy(if_data.ifr_name, module_data.interface, IF_NAMESIZE);
   if (ioctl(module_data.sock, SIOCGIFMTU, &if_data) < 0) {
      ng_perror("Mode %s could not determine mtu of local interface %s", eth_module.name, if_data.ifr_name);
      return 1;
   } else {
      module_data.mtu = if_data.ifr_mtu - sizeof(struct ethhdr);
      ng_info(NG_VLEV1 | NG_VPALL, "Determined local interface %s MTU %d", if_data.ifr_name, module_data.mtu);
   }

   // bind socket to local interface
   ng_info(NG_VLEV1, "Binding network socket to interface %s", module_data.interface);
   memset(&module_data.address, 0, sizeof(module_data.address));
   module_data.address.sll_protocol = htons(module_data.protocol);
   module_data.address.sll_family   = AF_PACKET;
   module_data.address.sll_ifindex  = module_data.ifindex;
	 module_data.address.sll_halen    = ETH_ALEN; 
	 if(bind(module_data.sock, (struct sockaddr*)&module_data.address, sizeof(module_data.address)) < 0) {
      ng_perror("Mode %s could not bind socket to local interface %s", eth_module.name, module_data.interface);
      return 1;
   }
	 //copy address of peer i to position i in module_data.addresses	
   if (MPI_Allgather(&module_data.local_addr, sizeof(struct ether_addr), MPI_CHAR,
                     module_data.addresses, sizeof(struct ether_addr), MPI_CHAR,
                     MPI_COMM_WORLD) != MPI_SUCCESS) {
      ng_error("Could not distribute ethernet address");
      exit(1);
   }
	 MPI_Barrier(MPI_COMM_WORLD);
   ng_info(NG_VLEV1, "Peer connections established.");
#else
	 ng_error("This module does not work without MPI yet.");
#endif
   // report success
   return 0;
}

static int eth_isendto(int dst, void *buffer, int size, NG_Request *req) {
   req_handle_t *sreq;
   //init sending
   sreq = malloc(sizeof(req_handle_t));
	 memset(sreq, 0, sizeof(req_handle_t));
   sreq->type = TYPE_SEND;
   sreq->index = dst;
   sreq->buffer = buffer;
   sreq->size = size;
   sreq->remaining_bytes = size;
   *req = (NG_Request *) sreq;
   eth_set_blocking(0, dst);    //non-blocking
   eth_test(req);
	 return size-sreq->remaining_bytes;
}

static int eth_irecvfrom(int src, void *buffer, int size, NG_Request *req) {
   req_handle_t *rreq;
//init receiving
   rreq = malloc(sizeof(req_handle_t));
	 memset(rreq, 0, sizeof(req_handle_t));
   rreq->type = TYPE_RECV;
   rreq->index = src;
   rreq->buffer = buffer;
   rreq->size = size;
   rreq->remaining_bytes = size;
   *req = (NG_Request *) rreq;
   eth_set_blocking(0, src);    //non-blocking
	 module_data.requests[src] = rreq;
   return 0;
}

int eth_test(NG_Request *req) {
	 req_handle_t *nreq;
   int ret=0;
//do the rest
   nreq = (req_handle_t *)*req;
   if (nreq->remaining_bytes <= 0)
      return 0;

   if (get_req_type(req) == TYPE_RECV) {
      ret = eth_recv_once(nreq->index, nreq->buffer, nreq->remaining_bytes);
      if (ret >= 0) {
         nreq->remaining_bytes -= ret;
         return nreq->remaining_bytes;
      } else if(ret == -EAGAIN) { //was busy or data from unexpected host
         return 1; //in progress
      }
   } else {
      ret = eth_send_once(nreq->index, nreq->buffer, nreq->remaining_bytes);
      if (ret >= 0) {
         nreq->remaining_bytes -= ret;
         return nreq->remaining_bytes;
      } else if(ret == -EAGAIN) { //was busy
         return 1; //in progress
      }
   }
   return ret;
}

static int eth_send_once(int dst, void *buffer, int size)
{
   char         *bufptr    = buffer;
   int          sent       = 0;

	 memcpy(module_data.address.sll_addr, &module_data.addresses[dst], sizeof(struct ether_addr));
   /* the first byte of the packet is the source-rank - we use this to
    * not add additional overhead to the protocol to have the measurement
    * as accurate as possible */
   *((char *)buffer) = (char)g_options.mpi_opts->worldrank;
   errno = 0;
   // send at most one mtu of data
   sent = sendto(module_data.sock,
                 bufptr,
                 ng_min(size, module_data.mtu),
                 module_data.send_flags,
                 (struct sockaddr *)&module_data.address, sizeof(struct sockaddr_ll));
   if (sent <= 0) {
      // nonblocking operation would block - try again
      if (errno == EAGAIN)
         return -EAGAIN;
      // os buffers are full - try again
      if (errno == ENOBUFS) {
         return -EAGAIN;
      }
      // stop on errors
      ng_perror("Mode eth send_once failed");
      return -1;
   }
   return sent;
}

static int eth_recv_once(int src, void *buffer, int size)
{
   char               *bufptr  = buffer;
   struct sockaddr_ll client;
   socklen_t          addrlen  = sizeof(client);
   int                rcvd     = 0;

   errno = 0;
   // receive any outstanding data
   rcvd = recvfrom(module_data.sock,
                   (void *)bufptr,
                   ng_min(size, module_data.mtu),
                   module_data.recv_flags,
                   (struct sockaddr *)&client,
                   &addrlen);
   if (rcvd < 0) {
      if (errno == EAGAIN)
         return -EAGAIN;

      ng_perror("Mode udp recv_once() failed");
      return -1;
   }

   /* check if the data is from our src */
	 int index = (int)*((char *)buffer);
   if (index != src) {
      /* it's from somebody else */
      //ng_info(NG_VLEV2, "Data from wrong host: host: %i, size: %i\n", *((char *)buffer), rcvd);
      if(module_data.requests[index]) {
         module_data.requests[index]->remaining_bytes -= rcvd;
      }
      return 0;
   }
   return rcvd;
}


static int eth_sendto(int dst, void *buffer, int size) {
	 char         *bufptr    = buffer;
   ssize_t      sent       = 0;
   unsigned int sent_total = 0;

   memcpy(module_data.address.sll_addr, &module_data.addresses[dst], sizeof(struct ether_addr));
  /* the first byte of the packet is the source-rank - we use this to
   * not add additional overhead to the protocol to have the measurement
   * as accurate as possible */
   bufptr[0] = (char)g_options.mpi_opts->worldrank;

   while (sent_total < size) {
			errno = 0;   
			sent = sendto(module_data.sock,
                    bufptr,
                    ng_min(size - sent_total, module_data.mtu),
                    module_data.send_flags,
                    (struct sockaddr *)&module_data.address, sizeof(struct sockaddr_ll));
      if (sent <= 0) {
         // CTRL-C
         if (stop_tests || errno == EINTR) {
            ng_error("sendto() interrupted at %d bytes", sent_total);
						return 1;
         }
         // nonblocking operation would block - try again
         if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
		     // os buffers are full - try again
				 if (errno == ENOBUFS) {
						continue;
				 }
         // stop on errors
         ng_perror("Mode %s sendto failed", eth_module.name);
				 return 1;
      }
      // continue sending while there is data left
      sent_total += sent;
      bufptr     += sent;
   }

   // report success
   return sent_total;
}

static int eth_recvfrom(int src, void *buffer, int size) 
{
	 char         *bufptr    = buffer;
   int          rcvd       = 0;
   unsigned int rcvd_total = 0;
	 struct sockaddr client;
	 unsigned int addrlen   = sizeof(client);
   
  /* look if we already have something */
  rcvd_total = module_data.rcvd_bytes[src];
  module_data.rcvd_bytes[src] = 0;

   // receive data
   while (rcvd_total < size) {
			errno = 0;
      // receive any outstanding data
      rcvd = recvfrom(module_data.sock,
                      bufptr,
                      size - rcvd_total,
                      module_data.recv_flags, (struct sockaddr *)&client, &addrlen);
      if (rcvd <= 0) {
         // CTRL-C
         if (stop_tests || errno == EINTR) {
            ng_error("recvfrom() interrupted at %d bytes", rcvd_total);
            return 1;
         }
         // nonblocking operation would block - try again
         if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
         // stop on errors
         ng_perror("Mode %s recvfrom failed", eth_module.name);
         return 1;
      }
    /* check if the data is from our src :) */
    if(*((char*)buffer) == (char)src) {
      /* yes it is -  continue sending while there is data left */
      rcvd_total += rcvd;
      bufptr     += rcvd;
    } else {
      /* it's from somebody else */
      module_data.rcvd_bytes[(int)*((char*)buffer)] += rcvd;
    }
	}

   // report success
   return rcvd_total;
}

/* module specific shutdown */
void eth_shutdown(struct ng_options *global_opts) {
   if (module_data.sock > 0) {
      // print socket statistics
      if (global_opts->verbose > 1) {
         struct tpacket_stats st;
         socklen_t size = sizeof(st);

         getsockopt(module_data.sock, SOL_PACKET, PACKET_STATISTICS, &st, &size);
         ng_info(NG_VLEV1 | NG_VPALL, "%d packets received, %d packets dropped",
                 st.tp_packets, st.tp_drops);
      }

      ng_info(NG_VLEV1, "Closing network socket");
      if (close(module_data.sock) < 0)
         ng_perror("Failed to close network socket");
   }
}


/* module specific usage information */
void eth_usage(void) {
	int i;
	
	for (i=0; long_options_eth[i].name != NULL; i++) {
   	ng_longoption_usage(
   		long_options_eth[i].val, 
   		long_options_eth[i].name, 
   		long_option_infos_eth[i].desc,
   		long_option_infos_eth[i].param
   	);
  }
}


/* module specific usage information */
void eth_writemanpage(void) {
	int i;
	
	for (i=0; long_options_eth[i].name != NULL; i++) {
   	ng_manpage_module(
   		long_options_eth[i].val, 
   		long_options_eth[i].name, 
   		long_option_infos_eth[i].desc,
   		long_option_infos_eth[i].param
   	);
  }
}

#else

int register_eth(void) {
// ng_error("This module is not supported on your OS");
   return 0;
}

#endif

#else

int register_eth(void) {
   return 0;
}

#endif
