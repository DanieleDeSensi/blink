/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

// vim:sts=3:sw=3:ts=3:expandtab

#include "netgauge.h"
#ifdef NG_MOD_UDP

#include "mod_inet.h"
#include <sys/fcntl.h>
#include <getopt.h> /* getopt long */

/* extern stuff */
extern struct ng_options g_options;

/* function prototypes */
static int udp_setup_channels_MPI();
static int udp_setup_channels_NOMPI();
static int udp_sendto(int dst, void *buffer, int size);
static int udp_recvfrom(int src, void *buffer, int size);
static int udp_set_blocking(int do_block, int partner);
static int udp_init(struct ng_options *global_opts);
static int udp_getopt(int argc, char **argv, struct ng_options *global_opts);
static void udp_writemanpage(void);
static void udp_usage(void);
static void udp_shutdown(struct ng_options *global_opts);
static int udp_recv_once(int src, void *buffer, int size);
static int udp_send_once(int dst, void *buffer, int size);
static int udp_isendto(int dst, void *buffer, int size, NG_Request *req);
static int udp_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static int udp_test(NG_Request *req);

/* module registration data structure (udp) */
struct ng_module udp_module = {
   .name             = "udp",
   .desc             = "Mode udp uses UDP sockets for data transmission.",
   .flags            = 0,              /* no channel semantics, not reliable, no memreg */
   //.max_datasize     = 65535 - sizeof(struct udphdr) - sizeof(struct iphdr),
   .max_datasize     = 65535 - sizeof(struct udphdr) - sizeof(struct ip),
   .headerlen        = 0,              /* no extra space needed for header */
   .getopt           = udp_getopt,
   .init             = udp_init,
   .malloc           = NULL,           /* no memreg */
   .shutdown         = udp_shutdown,
   .usage            = udp_usage,
   .writemanpage     = udp_writemanpage,
   .sendto           = udp_sendto,
   .recvfrom         = udp_recvfrom,
   .set_blocking     = udp_set_blocking,
   .isendto          = udp_isendto,
   .irecvfrom        = udp_irecvfrom,
   .test             = udp_test,
};

/**
 * getopt long options for inet
 */
static struct option long_options_ip[] = {
   {"localaddr", required_argument, 0, 'L'},
   {"remoteaddr", required_argument, 0, 'R'},
   {"subnet", required_argument, 0, 'S'},
   {"port", required_argument, 0, 'P'},
   {"nonblocking", required_argument, 0, 'N'},
   {0, 0, 0, 0}
};

/**
 * array of descriptions for inet
 */
static struct option_info long_option_infos_ip[] = {
   {"local ip address to bind to", "ADDRESS"},
   {"hostname or ip address of the server (client mode only)", "PEER"},
   {"subnet to use for server and client (only valid when MPI is used)",
    "SUBNET/MASK"},
   {"server port to bind or connect to (not IP)", "NUMBER"},
   {"nonblocking [s]end and/or [r]eceive mode", "[s][r]"},
   {0, 0}
};

/* module private data */
struct mod_udp_data {
   /* array of all addresses of all peers ... */
   struct sockaddr_in *addresses;
   /* maximum transfer unit */
   unsigned int mtu;
   /* port - user parameter (non-MPI case) */
   unsigned short port;
   /* send and recv flags */
   int send_flags, recv_flags;
   /* local and remote addr - user parameter (non-MPI case) */
   struct in_addr local_addr, remote_addr;
  /** my server address */
   struct sockaddr_in server_address;
  /** my server socket */
   int server_socket;

   /* this array holds the received bytes from every peer. Because UDP does
    * not have channel semantics and we may receive from a peer that we
    * don't want to receive from ... just store the bytes in
    * rcvd_bytes[peer] in this case - the recvfrom function will look into
    * this array first */
   int *rcvd_bytes;
   req_handle_t **requests;
};
static struct mod_udp_data module_data;

/* parse command line parameters for module-specific options */
static int udp_getopt(int argc, char **argv, struct ng_options *global_opts)
{
   int c;

   char *optchars = "P:L:R:S:N:";       // additional module options

   extern char *optarg;
   int option_index = 0;
   extern int optind, opterr, optopt;

   struct hostent *host;
   int failure = 0;

   // initialize module private information data structure
   memset(&module_data, 0, sizeof(module_data));

   module_data.mtu = 65535 -    /* max ip data len  - */
       sizeof(struct udphdr) -  /* udp header len   - */
       //sizeof(struct iphdr);    /* ip  header len     */
       sizeof(struct ip);    /* ip  header len     */

   // parse module-options
   while ((c = getopt_long(argc, argv, optchars, long_options_ip, &option_index)) >= 0) {
      switch (c) {
      case '?':                /* unrecognized or badly used option */
         if (!strchr(optchars, optopt))
            continue;           /* unrecognized */
         ng_error("Option %c requires an argument", optopt);
         failure = 1;
         break;
      case 'P':                /* port */
         module_data.port = atoi(optarg);
         break;
      case 'L':                /* local address to bind to */
         // don't allow specifying a local address when using MPI
         if (global_opts->mpi > 0) {
            ng_error("Option -L is not available when using MPI - use -S instead");
            failure = 1;
         } else {
            if (!inet_aton(optarg, &module_data.local_addr)) {
               ng_error("Malformed local address %s", optarg);
               failure = 1;
               continue;
            }
            ng_info(NG_VLEV2 | NG_VPALL, "Set local ip address %s",
                    inet_ntoa(module_data.local_addr));
         }
         break;
      case 'R':                /* remote address to connect to */
         // don't allow specifying a remote address when using MPI
         if (global_opts->mpi > 0) {
            ng_error("Option -R is not available when using MPI - use -S instead");
            failure = 1;
            break;
         } else {
            host = gethostbyname(optarg);
            if (!host) {
               ng_error("Malformed or unknown remote host name or address %s", optarg);
               failure = 1;
               continue;
            }
            memcpy(&module_data.remote_addr, host->h_addr, host->h_length);
            ng_info(NG_VLEV2 | NG_VPALL, "Set remote ip address %s",
                    inet_ntoa(module_data.remote_addr));
         }
         break;
      case 'S':                /* subnet to use when using MPI */
         if (global_opts->mpi > 0) {
            // address and corresponding netmask for subnet
            char *addr, *mask;
            char *pos;
            // address and corresponding netmask for subnet in
            // "struct in_addr" representation
            struct in_addr addr_in, mask_in;

            pos = strstr(optarg, "/");
            if (pos == NULL) {
               ng_error("Expected an \"address/netmask\" style argument");
               failure = 1;
               break;
            } else {
               // don't copy the user input but use "optarg" and make the slash
               // to a '\0'
               addr = optarg;
               mask = pos + 1;
               *pos = '\0';
            }
            //printf("addr=%s, mask=%s\n", addr, mask);

            if (!inet_aton(addr, &addr_in)) {
               ng_error("Malformed \"address\" part of argument %s/%s", addr, mask);
               failure = 1;
               break;
            }
            // get netmask from user input
            pos = strstr(mask, ".");
            if (pos == NULL) {  // assume user supplied the netmask as number (e.g. 24, 16,...)
               // this is _not_ IPv6 ready !!!
               mask_in.s_addr =
                   htonl(((unsigned int)pow(2, atoi(mask) + 1) - 1) << (32 - atoi(mask)));
            } else {            // assume user supplied a netmask in std. dot notation
               mask_in.s_addr = htonl(inet_network(mask));
               if (mask_in.s_addr == 0) {
                  ng_error("Malformed \"netmask\" part of argument %s/%s", addr, mask);
                  failure = 1;
                  break;
               }
            }

            if (inet_get_subnet(addr_in, mask_in, &module_data.local_addr)) {
               ng_error("Could not determine subnet to use");
               failure = 1;
               break;
            }
         } else
            ng_error("Option -S is only valid when using MPI (global option -p)");
         break;
      case 'N':                /* non-blocking send/receive mode */
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
      if (!module_data.port) {
         module_data.port = INET_DEFAULT_PORT;
         ng_info(NG_VLEV2, "No port specified - using default %d", module_data.port);
      }

      if (!global_opts->mpi && !global_opts->server && module_data.remote_addr.s_addr == 0) {
         ng_error("No remote address given for mode UDP - use option -R");
         failure = 1;
      }
#ifdef NG_MPI
      if (global_opts->mpi && module_data.local_addr.s_addr == 0) {
         ng_error("No subnet address/mask given for mode UDP - use option -S");
         failure = 1;
      }
#endif
   }                            // end if (!failure)

   ng_info(NG_VLEV2, "mode udp: parsed options");

   // report success or failure
   return failure;
}

static int udp_set_blocking(int do_block, int partner)
{
   int flags = fcntl(module_data.server_socket, F_GETFL);
   if (flags < 0) {
      ng_perror("Could not get flags");
      return 0;
   }
   if (do_block) {
      flags &= ~O_NONBLOCK;     //clear flag
      if (fcntl(module_data.server_socket, F_SETFL, flags)) {
         ng_perror("Could not set blocking operation for TCP/IP");
         return 0;
      }
   } else {
      flags |= O_NONBLOCK;      //set flag
      if (fcntl(module_data.server_socket, F_SETFL, flags) < 0) {
         ng_perror("Could not set non-blocking operation for TCP/IP");
         return 0;
      }
   }
   return 1;
}



static int udp_isendto(int dst, void *buffer, int size, NG_Request *req)
{
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
   udp_set_blocking(0, dst);    //non-blocking
   udp_test(req);
   return size-sreq->remaining_bytes;
}

static int udp_irecvfrom(int src, void *buffer, int size, NG_Request *req)
{
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
   udp_set_blocking(0, src);    //non-blocking
   module_data.requests[src] = rreq;
   return 0;
}

static int udp_recv_once(int src, void *buffer, int size)
{
   char               *bufptr   = buffer;
   int                rcvd      = 0;
   struct sockaddr_in client;
   socklen_t          addrlen   = sizeof(client);

   errno = 0;
   // receive any outstanding data
   rcvd = recvfrom(module_data.server_socket,
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
      //return -EAGAIN;
      return 0;
   }
   return rcvd;
}

static int udp_send_once(int dst, void *buffer, int size)
{
   char *bufptr = buffer;
   int sent = 0;

   /* the first byte of the packet is the source-rank - we use this to
    * not add additional overhead to the protocol to have the measurement
    * as accurate as possible */
   *((char *)buffer) = (char)g_options.mpi_opts->worldrank;
   errno = 0;
   // send at most one mtu of data
   sent = sendto(module_data.server_socket,
                 bufptr,
                 ng_min(size, module_data.mtu),
                 module_data.send_flags,
                 (struct sockaddr *)&module_data.addresses[dst], sizeof(struct sockaddr));
   if (sent <= 0) {
      // nonblocking operation would block - try again
      if (errno == EAGAIN)
         return -EAGAIN;
      // os buffers are full - try again 
      if (errno == ENOBUFS) {
         return -EAGAIN;
      }
      // stop on errors
      ng_perror("Mode udp send_once failed");
      return -1;
   }
   return sent;
}

int udp_test(NG_Request *req) {
   req_handle_t *nreq;
   int ret=0;
//do the rest
   nreq = (req_handle_t *)*req;
   if (nreq->remaining_bytes <= 0)
      return 0;

   if (get_req_type(req) == TYPE_RECV) {
      ret = udp_recv_once(nreq->index, nreq->buffer, nreq->remaining_bytes);
      if (ret >= 0) {
         nreq->remaining_bytes -= ret;
         return nreq->remaining_bytes;
      } else if(ret == -EAGAIN) { //was busy
         return 1; //in progress
      }
   } else {
      ret = udp_send_once(nreq->index, nreq->buffer, nreq->remaining_bytes);
      if (ret >= 0) {
         nreq->remaining_bytes -= ret;
         return nreq->remaining_bytes;
      } else if(ret == -EAGAIN) { //was busy
         return 1; //in progress
      }
   }
   return ret;
}

static int udp_sendto(int dst, void *buffer, int size) {
   char         *bufptr    = buffer;
   int          sent       = 0;
   unsigned int sent_total = 0;

   /* the first byte of the packet is the source-rank - we use this to
    * not add additional overhead to the protocol to have the measurement
    * as accurate as possible */
   *((char*)buffer) = (char)g_options.mpi_opts->worldrank;

   /* send data */
   while (sent_total < size) {
      // send at most one mtu of data
      sent = sendto(module_data.server_socket,
                    (void *)bufptr,
                    ng_min(size - sent_total, module_data.mtu),
                    module_data.send_flags,
                    (struct sockaddr *)&module_data.addresses[dst], sizeof(struct sockaddr));
      if (sent <= 0) {
         // CTRL-C
         if (g_stop_tests || errno == EINTR) {
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
         ng_perror("Mode udp sendto failed");
         return 1;
      }
      // continue sending while there is data left
      sent_total += sent;
      bufptr     += sent;
   }

   // report success
   return sent_total;
}

static int udp_recvfrom(int src, void *buffer, int size)
{
   char *bufptr = buffer;
   int rcvd     = 0;
   unsigned int rcvd_total = 0;
   struct sockaddr_in client;
   socklen_t addrlen = sizeof(client);

   /* look if we already have something */
   rcvd_total = module_data.rcvd_bytes[src];
   module_data.rcvd_bytes[src] = 0;

   // receive data
   while (rcvd_total < size) {
      // receive any outstanding data
      rcvd = recvfrom(module_data.server_socket,
                      (void *)bufptr,
                      size - rcvd_total,
                      module_data.recv_flags, (struct sockaddr *)&client, &addrlen);

      //printf("**\n");
      if (rcvd <= 0) {
         // CTRL-C
         if (g_stop_tests || errno == EINTR) {
            ng_error("recvfrom() interrupted at %d bytes", rcvd_total);
            return -1;
         }
         // nonblocking operation would block - try again
         if (errno == EAGAIN || errno == EWOULDBLOCK)
            continue;

         // stop on errors
         ng_perror("Mode udp recvfrom failed with error %i", rcvd);
         return -1;
      }

      /* check if the data is from our src :) */
      if (*((char *)buffer) == (char)src) {
         /* yes it is -  continue sending while there is data left */
         rcvd_total += rcvd;
         bufptr     += rcvd;
      } else {
         /* it's from somebody else */
         module_data.rcvd_bytes[(int)*((char *)buffer)] += rcvd;
      }
   }

   return rcvd_total;
}

/* module specific benchmark initialization */
static int udp_init(struct ng_options *global_opts)
{
   if (!g_options.mpi) {
      /* we don't want or we don't have MPI */
      return udp_setup_channels_NOMPI();
   } else {
      return udp_setup_channels_MPI();
   }
}

/**
 * The simple one. Just two peers to connect.
 */
static int udp_setup_channels_NOMPI()
{
   int result;

   module_data.rcvd_bytes = calloc(2, sizeof(int));
   module_data.addresses = calloc(2, sizeof(struct sockaddr_in));
   module_data.requests = calloc(2, sizeof(req_handle_t*));
   memset(module_data.requests, 0, sizeof(req_handle_t*)*2);

   if (g_options.server) {
      /* SERVER mode */

      ng_info(NG_VLEV1, "Initializing server socket.");
      module_data.server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (module_data.server_socket == -1) {
         ng_perror("Could not create Socket");
         return 1;
      }

      memset(&module_data.server_address, 0, sizeof(struct sockaddr_in));
      module_data.server_address.sin_family = AF_INET;
      module_data.server_address.sin_addr = module_data.remote_addr;
      module_data.server_address.sin_port = htons(module_data.port);
      memcpy(&module_data.addresses[0], &module_data.server_address,
             sizeof(struct sockaddr_in));
      module_data.server_address.sin_addr = module_data.local_addr;
      memcpy(&module_data.addresses[1], &module_data.server_address,
             sizeof(struct sockaddr_in));

      result = bind(module_data.server_socket,
                    (struct sockaddr *)&module_data.server_address,
                    sizeof(struct sockaddr_in));

      if (result < 0) {
         ng_perror("Could not bind to port");
         return 1;
      }

      ng_info(NG_VLEV1, "Bound network socket to %s:%d",
              inet_ntoa(module_data.server_address.sin_addr), module_data.port);

      listen(module_data.server_socket, 1);

   } else {
      ng_info(NG_VLEV2, "CLIENT mode");
      /* CLIENT mode */

      module_data.server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

      if (module_data.server_socket == -1) {
         ng_perror("Could not create socket");
         return 1;
      }

      module_data.server_address.sin_family = AF_INET;
      module_data.server_address.sin_addr = module_data.local_addr;
      module_data.server_address.sin_port = htons(module_data.port);
      memcpy(&module_data.addresses[0], &module_data.server_address,
             sizeof(struct sockaddr_in));
      module_data.server_address.sin_addr = module_data.remote_addr;
      memcpy(&module_data.addresses[1], &module_data.server_address,
             sizeof(struct sockaddr_in));

      ng_info(NG_VLEV2, "Trying to connect to %s:%d",
              inet_ntoa(module_data.server_address.sin_addr), module_data.port);

      if (connect(module_data.server_socket,
                  (struct sockaddr *)(&module_data.server_address),
                  sizeof(struct sockaddr_in)) < 0) {

         ng_perror("Mode %s could not connect socket to %s:%d",
                   udp_module.name,
                   inet_ntoa(module_data.server_address.sin_addr), module_data.port);
         return 1;
      }

      ng_info(NG_VLEV2 | NG_VPALL, "Connected network socket to %s:%d",
              inet_ntoa(module_data.server_address.sin_addr), module_data.port);

   }

   return 0;
}

/**
 * The more complex one. We need to ensure every peer can send to
 * and receive from every other peer. This is done by having peer
 * i connect to peers (i + 1) .. N for i in 0 .. N-1, where N is
 * the number of peers we've got.
 * 
 * In ASCII "Art":
 * 
 * p0       p1-+     p2   ...  pn
 * |        ^  |     ^         ^
 * ------------+-----+---------+
 *             ------|---------|
 */
static int udp_setup_channels_MPI()
{
#ifdef NG_MPI
   const int peer_count = g_options.mpi_opts->worldsize;
   int result, retries = 0;

   ng_info(NG_VLEV2, "Distributing IP adresses between %d peers.", peer_count);

   /* allocate our addresses - an address structure for every peer */
   module_data.addresses = malloc(peer_count * sizeof(struct sockaddr_in));
   module_data.requests = calloc(peer_count, sizeof(req_handle_t*));
   memset(module_data.requests, 0, sizeof(req_handle_t*)*peer_count);

   /* allocate rcvd_bytes array, used to count the bytes that are
    * received "accidentially" */
   module_data.rcvd_bytes = calloc(peer_count, sizeof(int));

   /* initialize the server socket for accepting connections */

   module_data.server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   memset(&module_data.server_address, 0, sizeof(module_data.server_address));
   module_data.server_address.sin_family = AF_INET;
   module_data.server_address.sin_addr = module_data.local_addr;
   module_data.server_address.sin_port = htons(module_data.port);
   module_data.send_flags = 0;
   module_data.recv_flags = 0;

   do {
      result = bind(module_data.server_socket,
                    (struct sockaddr *)&module_data.server_address,
                    sizeof(module_data.server_address));
      if (result < 0) {
         /* try next port */

         module_data.server_address.sin_port = htons(++module_data.port);
         ng_info(NG_VLEV1 | NG_VPALL,
                 "Could not bind(), trying next port: %d", module_data.port);
      }
   }
   while ((result < 0) && (retries++ < 64));

   /* did we successfully bind()? */
   if (result < 0) {
      ng_perror("Mode UDP could not bind socket to %s:%d",
                inet_ntoa(module_data.server_address.sin_addr),
                ntohs(module_data.server_address.sin_port));
      return 1;
   }

   ng_info(NG_VLEV1 | NG_VPALL, "Bound UDP socket to %s:%d",
           inet_ntoa(module_data.server_address.sin_addr),
           ntohs(module_data.server_address.sin_port));

   /* distribute the addresses / ports */

   if (MPI_Allgather(&(module_data.server_address), sizeof(struct sockaddr_in), MPI_BYTE,
                     module_data.addresses, sizeof(struct sockaddr_in),
                     MPI_BYTE, MPI_COMM_WORLD) != MPI_SUCCESS) {
      ng_error("Could not distribute IP adresses.");
      exit(1);
   }

   return 0;
#else
   /* we should have called the NOMPI version of this
    * function - if get here, it's in error */
   return 1;
#endif
}

/* module specific shutdown */
static void udp_shutdown(struct ng_options *global_opts)
{

   free(module_data.addresses);

   if (module_data.server_socket > 0) {
      ng_info(NG_VLEV1, "Closing network socket");
      if (close(module_data.server_socket) < 0)
         ng_perror("Failed to close UDP socket");
   }
}

/* module specific manpage information */
static void udp_writemanpage(void)
{
   int i;

   for (i = 0; long_options_ip[i].name != NULL; i++) {
      ng_manpage_module(long_options_ip[i].val,
                        long_options_ip[i].name,
                        long_option_infos_ip[i].desc, long_option_infos_ip[i].param);
   }
}

/* module specific usage information */
static void udp_usage(void)
{
   int i;

   for (i = 0; long_options_ip[i].name != NULL; i++) {
      ng_longoption_usage(long_options_ip[i].val,
                          long_options_ip[i].name,
                          long_option_infos_ip[i].desc, long_option_infos_ip[i].param);
   }
}


/* module registration */
int register_udp(void)
{
   ng_register_module(&udp_module);
   return 0;
}

#else

/* dummy module registration */
int register_udp(void)
{
   return 0;
}


#endif
