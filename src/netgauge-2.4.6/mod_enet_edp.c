/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

// vim:expandtab

#include "netgauge.h"
#ifdef NG_MOD_ENET_EDP
#include "mod_enet.h"
#include "eth_helpers.h"

#include <sys/fcntl.h>

/** module private data */
static struct enet_edp_private_data {
  char                        *name;
  char                        *interface;
  unsigned int                ifindex;
  unsigned int                mtu;
  int                         sock; //my socket
  int                         conn; //number of connections
  unsigned short              prot_family;
  unsigned short              port;
  int                         recv_flags, send_flags;
  short                       zerocopy;
  struct ether_addr           eth_laddr, eth_raddr;
  struct sockaddr_en          enet_laddr, enet_raddr;
  const struct ng_mpi_options *mpi_opts;
  unsigned int                *rcvd_bytes;

  /** peer addresses */
  struct sockaddr_en *addresses;

} module_data;

/* function prototypes */
static int enet_edp_getopt(int argc, char **argv, struct ng_options *global_opts);
static int enet_edp_init(struct ng_options *global_opts);
static void enet_edp_shutdown(struct ng_options *global_opts);
static void enet_edp_usage(void);
static void enet_edp_writemanpage(void);
static inline int enet_edp_sendto(int dst, void *buffer, int size);
static inline int enet_edp_recvfrom(int src, void *buffer, int size);
static int enet_edp_set_blocking(int do_block, int partner);


/* module registration data structure */
struct ng_module edp_module = {
   .name           = "edp",
   .desc           = "Mode edp uses Ethernet Datagram Protocol sockets.",
   .headerlen      = 0,    /* no extra space needed for header */
   .max_datasize   = -1,   /* can send data of arbitrary size */
   .getopt         = enet_edp_getopt,
   .init           = enet_edp_init,
   .shutdown       = enet_edp_shutdown,
   .usage          = enet_edp_usage,
   .writemanpage   = enet_edp_writemanpage,
   .sendto         = enet_edp_sendto,
   .recvfrom       = enet_edp_recvfrom,
	 .set_blocking   = enet_edp_set_blocking,
};

/* module registration */
int register_enet_edp(void) {
   ng_register_module(&edp_module);
   return 0;
}

/* parse command line parameters for module-specific options */
static int enet_edp_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;

   /* NG_OPTION_STRING is deprecated in module functions */
   //char *optchars = NG_OPTION_STRING "I:F:P:R:N:Z";  // additional module options
   char *optchars = "I:F:P:R:N:Z";  // additional module options

   extern char *optarg;
   extern int optind, opterr, optopt;
   int option_index = 0;

   int failure = 0;

   memset(&module_data, 0, sizeof(module_data));

   module_data.name = "edp";

   /* parse module-options */
   while((c=getopt(argc, argv, optchars)) >= 0 ) {
   switch( c ) {
         case '?':      /* unrecognized or badly used option */
            if (!strchr(optchars, optopt))
               continue;        /* ignore unrecognized option */
            ng_error("Option %c requires an argument", optopt);
            failure = 1;
            break;
         case 'I':      /* network interface */
            module_data.interface = optarg;
            break;
         case 'P':      /* EDP port */
            module_data.port = atoi(optarg);
            break;
         case 'R':      /* remote address */
            if (global_opts->mpi > 0) {
               ng_error("Option -R is not available when using MPI - the remote address is determined automatically");
               failure = 1;
            } else {
               if (!ether_aton_r(optarg, &module_data.eth_raddr)) {
                  ng_error("Malformed ethernet address %s", optarg);
                  failure = 0;
                  continue;
               }
               ng_info(NG_VLEV2 | NG_VPALL, "Set remote ethernet address %s",
                       ether_ntoa(&module_data.eth_raddr));
            }
            break;
         case 'N':      /* non-blocking send/receive mode */
            if (strchr(optarg, 's')) {
               module_data.send_flags |= MSG_DONTWAIT;
               ng_info(NG_VLEV2, "Using nonblocking send mode");
            }
            if (strchr(optarg, 'r')) {
               module_data.recv_flags |= MSG_DONTWAIT;
               ng_info(NG_VLEV2, "Using nonblocking receive mode");
            }
            break;
         case 'Z':
            module_data.zerocopy = 1;
            ng_info(NG_VLEV2, "Using zero-copy send mode");
            break;
      }
   }

   // check for necessary parameters, apply default values where possible
   if (!failure) {
      if (!module_data.port) {
         module_data.port = ENET_DEFAULT_PORT;
         ng_info(NG_VLEV2, "No port specified - using default %d",
                 module_data.port);
      }

      if (!module_data.interface) {
         ng_error("No interface given for mode %s - use option -I",
                  module_data.name);
         failure = 1;
      }
      
      if (!global_opts->mpi && !global_opts->server && ETH_ADDR_NONE(module_data.eth_raddr.ether_addr_octet)) {
         ng_error("No remote address given for mode %s - use option -R",
                  module_data.name);
         failure = 1;
      }
   }

   // report success or failure
   return failure;
}

/* module specific benchmark initialization */
int enet_edp_init(struct ng_options *global_opts) {
   socklen_t optlen, addrlen;
   int optval;
   const int peer_count = g_options.mpi_opts->worldsize;

   module_data.prot_family = PF_ENET;
   module_data.rcvd_bytes  = calloc(peer_count, sizeof(unsigned int));

   /* initialize network socket */
   ng_info(NG_VLEV1, "Initializing network socket");
   module_data.sock = socket(
          module_data.prot_family,
          SOCK_DGRAM,
          module_data.zerocopy? EPPROTO_EZDP : EPPROTO_EDP);
   
	 if (module_data.sock <= 0) {
      ng_perror("Mode %s could not create network socket", module_data.name);
      return 1;
   }

   /* set maximum possible read/write memory */
   optval = global_opts->max_datasize;
   setsockopt(module_data.sock, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
   setsockopt(module_data.sock, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));
   optlen = sizeof(int);
/* FIXME do we need access to the buffers?
	if (getsockopt(module_data.sock, SOL_SOCKET, SO_RCVBUF, &module_data.recv_buf, &optlen) < 0 ||
       getsockopt(module_data.sock, SOL_SOCKET, SO_SNDBUF, &module_data.send_buf, &optlen) < 0) {
      ng_perror("Mode %s could not determine the socket's send or receive buffer size", module_data.name);
      return 1;
   }
   ng_info(NG_VLEV2, "Set receive buffer to %d, send buffer to %d bytes",
           module_data.recv_buf, module_data.send_buf);
*/
   /* determine local interface index */
   module_data.ifindex = if_nametoindex(module_data.interface);
   if (module_data.ifindex <= 0) {
      ng_perror("Mode %s could not determine index of local interface %s",
                module_data.name, module_data.interface);
      return 1;
   }
   ng_info(NG_VLEV2 | NG_VPALL, "Determined local interface %s index %d",
           module_data.interface, module_data.ifindex);

   /* determine local interface ethernet (mac) address */
   get_if_addr(module_data.sock, module_data.interface, &module_data.eth_laddr);

   /* bind socket to local interface */
   memset(&module_data.enet_laddr, 0, sizeof(module_data.enet_laddr));
   module_data.enet_laddr.sen_family   = module_data.prot_family;
   module_data.enet_laddr.sen_ifindex  = module_data.ifindex;
   module_data.enet_laddr.sen_port     = (global_opts->server)
      ? htons(module_data.port) /* only the server binds to a specific port */
      : 0;
   if(bind(module_data.sock, (struct sockaddr*)&module_data.enet_laddr, sizeof(module_data.enet_laddr)) < 0) {
      ng_perror("Mode %s could not bind socket to local interface %s port %d",
                module_data.name, module_data.interface, module_data.enet_laddr.sen_port);
      return 1;
   }

   /* determine the actual port bound to for client mode */
   if (!global_opts->server) {
      addrlen = sizeof(module_data.enet_laddr);
      if (getsockname(module_data.sock,
                      (struct sockaddr*)&module_data.enet_laddr,
                      &addrlen) < 0) {
         ng_perror("Mode %s could not determine bound local port",
                   module_data.name);
         return 1;
      }
      ng_info(NG_VLEV2 | NG_VPALL, "Bound network socket to interface %s port %d",
              module_data.interface,
              ntohs(module_data.enet_laddr.sen_port));
   }

   /* determine edp local protocol mtu */
		optlen = sizeof(module_data.mtu);
		if (getsockopt(module_data.sock, SOL_EP, EDP_SO_MTU, &module_data.mtu, &optlen)) {
			 ng_perror("Mode %s could not determine mtu", module_data.name);
			 return 1;
		}
		ng_info(NG_VLEV1 | NG_VPALL, "Determined EDP protocol MTU %d", module_data.mtu);
#ifdef NG_MPI
   // if we are in MPI mode, the server sends the mac address it will use
   // to the client
   if (global_opts->mpi > 0) {
      // save a pointer to the MPI options for later use when doing synchronization
      module_data.mpi_opts = global_opts->mpi_opts;

      if (global_opts->server) { // server
         ng_info(NG_VLEV2 | NG_VPALL, "Sending MAC address %s to %d...",
                 ether_ntoa(&module_data.eth_laddr),
                 global_opts->mpi_opts->partner);

         if (MPI_Send(&module_data.eth_laddr.ether_addr_octet, ETH_ALEN, MPI_BYTE, global_opts->mpi_opts->partner, 0, global_opts->mpi_opts->comm) != MPI_SUCCESS) {
            ng_error("Could not send MAC address to %d",
                     global_opts->mpi_opts->partner);
            return 1;
         }

      } else { // client
         MPI_Status status;

         ng_info(NG_VLEV2 | NG_VPALL, "Receiving MAC address from %d...",
                 global_opts->mpi_opts->partner);

         if (MPI_Recv(&module_data.eth_raddr.ether_addr_octet, ETH_ALEN, MPI_BYTE, global_opts->mpi_opts->partner, MPI_ANY_TAG, global_opts->mpi_opts->comm, &status) != MPI_SUCCESS) {
            ng_error("Could not receive MAC address from %d",
                     global_opts->mpi_opts->partner);
            return 1;
         }
         ng_info(NG_VLEV1 | NG_VPALL, "Will use MAC address %s for communication with partner server %d",
                 ether_ntoa(&module_data.eth_raddr),
                 global_opts->mpi_opts->partner);
      }
   } // end if (global_opts->mpi > 0)
#endif

   /* init remote address for sendig / connect */
   memset(&module_data.enet_raddr, 0, sizeof(module_data.enet_raddr));
   module_data.enet_raddr.sen_family  = module_data.prot_family;
   module_data.enet_raddr.sen_ifindex = module_data.ifindex;
   module_data.enet_raddr.sen_port    = htons(module_data.port);
   module_data.enet_raddr.sen_addrlen = ETH_ALEN;
   ETH_ADDR(module_data.enet_raddr.sen_addr) = module_data.eth_raddr;
	return 0;
   return 0;
}

static int enet_edp_set_blocking(int do_block, int partner) {
   int flags = fcntl(module_data.sock, F_GETFL);
   if(flags<0) {
        ng_perror("Could not get flags");
        return 0;
   }
   if (do_block) {
     flags &= ~O_NONBLOCK; //clear flag
     if(fcntl(module_data.sock, F_SETFL, flags)) {
       ng_perror("Could not set blocking operation for ESP");
       return 0;
     }
   } else {
     flags |= O_NONBLOCK; //set flag
     if (fcntl(module_data.sock, F_SETFL, flags) < 0) {
       ng_perror("Could not set non-blocking operation for ESP");
       return 0;
     }
   }
   return 1;
}


/* module function for sending a single arbitrary sized buffer of data */
static inline int enet_edp_sendto(int dst, void *buffer, int size) {
   char         *bufptr    = buffer;
   int          sent       = 0;
   unsigned int sent_total = 0;

   bufptr[0] = (char)g_options.mpi_opts->worldrank;

   /* send data */
   while (sent_total < size) {
      errno = 0;
      /* send at most one mtu of data */
      sent = sendto(
	      module_data.sock,
	      (void *)bufptr,
	      ng_min(size - sent_total, module_data.mtu),
	      module_data.send_flags,
	      (struct sockaddr *) &module_data.addresses[dst], //FIXME: get from adresses
	      sizeof(&module_data.addresses[dst])
	   );
      if (sent <= 0) {
         /* CTRL-C */
         if (g_stop_tests || errno == EINTR) {
            ng_error("sendto() interrupted at %d bytes", sent_total);
            return 1;
	      }
	      /* nonblocking operation would block - try again */
	      if (errno == EAGAIN || errno == EWOULDBLOCK)
	         continue;
	      /* stop on errors */
	      ng_perror("Mode %s sendto failed", module_data.name);
	      return 1;
      }
      /* continue sending while there is data left */
      sent_total += sent;
      bufptr     += sent;
   }
   /* report success */
   return 0;
}


/* module function for receiving a single arbitrary sized buffer of data */
static inline int enet_edp_recvfrom(int src, void *buffer, int size) {
   char     *bufptr        = buffer;
   int      rcvd           = 0;
   unsigned int rcvd_total = 0;
   unsigned int addr_len;

  /* look if we already have something */
  rcvd_total = module_data.rcvd_bytes[src];
  module_data.rcvd_bytes[src] = 0;

   /* receive data */
   while (rcvd_total < size) {
      /* receive any outstanding data */
      rcvd = recvfrom(
         module_data.sock,
         (void *)bufptr,
         size - rcvd_total,
         module_data.recv_flags,
         (struct sockaddr *) &module_data.addresses[src],
         &addr_len
      );
      if (rcvd <= 0) {
         /* CTRL-C */
         if (g_stop_tests || errno == EINTR) {
            ng_error("recvfrom() interrupted at %d bytes", rcvd_total);
            return 1;
         }
         /* nonblocking operation would block - try again */
         if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;
         /* stop on errors */
         ng_perror("Mode %s recvfrom failed", module_data.name);
         return 1;
      }
      /* continue sending while there is data left */
      rcvd_total += rcvd;
      bufptr     += rcvd;
   
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
   /* report success */
   return 0;
}



/* module specific shutdown */
static void enet_edp_shutdown(struct ng_options *global_opts) {
   if (module_data.conn > 0) {
      ng_info(NG_VLEV2, "Closing accepted network socket");
      if (close(module_data.conn) < 0)
				ng_perror("Failed to close accepted network socket");
   }

   if (module_data.sock > 0) {
      ng_info(NG_VLEV1, "Closing network socket");
      if (close(module_data.sock) < 0)
				ng_perror("Failed to close network socket");
   }
}

/* module specific usage information */
static void enet_edp_usage(void) {
}


/* module specific manpage information */
static void enet_edp_writemanpage(void) {
}
#else


int register_enet_edp(void) {
// ng_error("This module is not supported on your OS");
   return 0;
}

#endif

