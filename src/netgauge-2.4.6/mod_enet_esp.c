/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#ifdef NG_MOD_ENET_ESP
#include <mpi.h>

#include "mod_enet.h"
#include "eth_helpers.h"

#include <sys/fcntl.h>

/** module private data */
static struct enet_esp_private_data {
  char                        *name;
  char                        *interface;
  unsigned int                ifindex;
  unsigned int                mtu;
  int                         sock;
  int                         conn; //number of connections
  unsigned short              port;
  int                         recv_flags, send_flags;
  short                       zerocopy;
  struct ether_addr           eth_laddr, eth_raddr;
  struct sockaddr_en          enet_laddr, enet_raddr;
  const struct ng_mpi_options *mpi_opts;

  /* sockets to other peers */
  int *peer_connections;

} module_data;


/* function prototypes */
static int enet_esp_getopt(int argc, char **argv, struct ng_options *global_opts);
static int enet_esp_init(struct ng_options *global_opts);
static void enet_esp_shutdown(struct ng_options *global_opts);
static void enet_esp_usage(void);
static void enet_esp_writemanpage(void);
static inline int enet_esp_send(int dst, void *buffer, int size);
static inline int enet_esp_recv(int src, void *buffer, int size);
static int enet_esp_set_blocking(int do_block, int partner);
static int enet_esp_setup_channels_MPI();

/**
 * getopt long options for enet esp
 */

/*
static struct option long_options_enet[]={
        {"interface",                           required_argument,      0, 'I'},
        {"port",                                                required_argument,      0, 'P'},
        {"remoteaddr",                  required_argument,      0, 'R'},
        {"nonblocking",                 required_argument,      0, 'N'},
        {"zerocopy",                            no_argument,                            0, 'Z'},
        {0, 0, 0, 0}
};
*/
/**
 * array of descriptions for enet esp
 */
/*
static struct option_info long_option_infos_enet[]={
        {"network interface to use", "NAME"},
        {"ENET port", "NUMBER"},
        {"ethernet (mac) address of the server (client mode only)", "PEER"},
        {"nonblocking [s]end and/or [r]eceive mode", "[s][r]"},
        {"use zero-copy sending", NULL},
};
*/

struct ng_module esp_module = {
   .name           = "esp",
   .desc           = "Mode esp uses Ethernet Sequenced Packets sockets.",
   .headerlen      = 0,    /* no extra space needed for header */
   .max_datasize   = -1,   /* can send data of arbitrary size */
   .getopt         = enet_esp_getopt,
   .init           = enet_esp_init,
   .shutdown       = enet_esp_shutdown,
   .usage          = enet_esp_usage,
   .writemanpage   = enet_esp_writemanpage,
   .sendto         = enet_esp_send,
   .recvfrom       = enet_esp_recv,
   .set_blocking   = enet_esp_set_blocking,
};


int register_enet_esp(void) {
   ng_register_module(&esp_module);
   return 0;
}

/* parse command line parameters for module-specific options */
static int enet_esp_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;

   /* NG_OPTION_STRING is deprecated in module functions */
   //char *optchars = NG_OPTION_STRING "I:F:P:R:N:Z";  // additional module options
   char *optchars = "I:F:P:R:N:Z";  // additional module options

   extern char *optarg;
   extern int optind, opterr, optopt;
   int option_index = 0;

   int failure = 0;

   memset(&module_data, 0, sizeof(module_data));

   module_data.name = "esp";

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

int enet_esp_init(struct ng_options *global_opts) {
#ifdef NG_MPI
	return enet_esp_setup_channels_MPI();
#else
   /* don't know how to do this right now */
   ng_error("This does not work without MPI.");
   exit(1);
#endif
}

/**
 * p0       p1-+     p2   ...  pn
 * |        ^  |     ^         ^
 * ------------+-----+---------+
 *             ------|---------|
 */
int enet_esp_setup_channels_MPI() {
   const int peer_count = g_options.mpi_opts->worldsize;
   struct sockaddr_en *addresses = 
      malloc(peer_count * sizeof(struct sockaddr_en));
   int i, result, retries = 0;
   ng_info(NG_VLEV2, "Distributing EP adresses between %d peers.", peer_count);

   module_data.ifindex = if_nametoindex(module_data.interface);

   /* initialize the socket for accepting connections */
   module_data.sock = socket(PF_ENET, SOCK_STREAM, EPPROTO_ESP);
   memset(&module_data.enet_laddr, 0, sizeof(struct sockaddr_en));
   module_data.enet_laddr.sen_family  = AF_ENET;
   module_data.enet_laddr.sen_ifindex = module_data.ifindex;
   module_data.enet_laddr.sen_port    = htons(module_data.port);

   // determine local interface ethernet (mac) address
   if(get_if_addr(module_data.sock, module_data.interface, &module_data.eth_laddr))
     return 1; 
   printf("local eth addr: %s\n", ether_ntoa(&ETH_ADDR(&module_data.eth_laddr)));
   memcpy(&module_data.enet_laddr.sen_addr, &module_data.eth_laddr, sizeof(struct ether_addr));

   do {
      errno = 0;
      result = bind(module_data.sock,
          (struct sockaddr*)&module_data.enet_laddr, 
          sizeof(struct sockaddr_en));
      if (result < 0) {
        /* try next port */
        module_data.enet_laddr.sen_port = htons(++module_data.port);
        ng_info(NG_VLEV1 | NG_VPALL, "Could not bind(), trying next port: %d", module_data.port);
      }
   } while ((result < 0) && (retries++ < 64));
   
   /* did we successfully bind()? */
   if (result < 0) {
      ng_perror("Mode enet_esp could not bind socket to %s:%d",
         ether_ntoa(&ETH_ADDR(&module_data.eth_laddr)),
         ntohs(module_data.enet_laddr.sen_port));
      return 1;
   }
   
   ng_info(NG_VLEV2 | NG_VPALL, "Bound network socket to %s:%d",
      ether_ntoa(&ETH_ADDR(&module_data.eth_laddr)),
      ntohs(module_data.enet_laddr.sen_port));
   
   /* distribute the addresses / ports */
   
   if (MPI_Allgather(&(module_data.enet_laddr),
           sizeof(struct sockaddr_en), MPI_CHAR,
           addresses, sizeof(struct sockaddr_en), MPI_CHAR,
           MPI_COMM_WORLD) != MPI_SUCCESS) {
      ng_error("Could not distribute IP adresses.");
      exit(1);
   }
   
   /* make the connections */

   module_data.peer_connections = malloc(sizeof(int) * peer_count);

   for (i=0; i < peer_count; i++) {

      if (i == g_options.mpi_opts->worldrank) {
           /* peer i connects to peers (i+1) .. (peer_count-1) */
           int j;
           
           /* wait for sockets to listen */
           MPI_Barrier(MPI_COMM_WORLD);
           
           for (j=i+1; j < peer_count; j++) {
              /* connect to peer j */
              module_data.peer_connections[j] =
                 socket(PF_ENET, SOCK_STREAM, EPPROTO_ESP);
              
              if (connect(module_data.peer_connections[j],
                         (struct sockaddr*)(addresses + j),
                         sizeof(struct sockaddr_en)) < 0) {
                 ng_perror("Mode %s could not connect socket to %s:%d", module_data.name,
                           ether_ntoa(&ETH_ADDR(&addresses[j].sen_addr)),
                           ntohs(addresses[j].sen_port));
                 return 1;
              }
              
              ng_info(NG_VLEV2 | NG_VPALL, "Connected network socket to %s:%d",
                    ether_ntoa(&ETH_ADDR(&addresses[j].sen_addr)),
                    ntohs(addresses[j].sen_port));
              
           }
      } else {
           /* accept the connection from peer i */
           if (i <= g_options.mpi_opts->worldrank) {
              unsigned int cli_size;
              struct sockaddr_en cli;

              /* start listening */
              if (listen(module_data.sock, 1) == -1) {
                 ng_error("listen() failed");
                 return 1;
              }
              
              MPI_Barrier(MPI_COMM_WORLD);
              
              cli_size = sizeof(cli);
              module_data.peer_connections[i] = accept(
                         module_data.sock,
                         (struct sockaddr*)&cli,
                         &cli_size);

              ng_info(NG_VLEV2 | NG_VPALL, "accepted connection from %s:%d",
                ether_ntoa(&ETH_ADDR(&cli.sen_addr)),
                ntohs(cli.sen_port));

           } else {
              /* just make the barrier happy */
              MPI_Barrier(MPI_COMM_WORLD);
           }
      }
      
   }
   free(addresses);

   ng_info(NG_VLEV1, "Peer connections established.");

   return 0;
}

static int enet_esp_set_blocking(int do_block, int partner) {
   int flags = fcntl(module_data.peer_connections[partner], F_GETFL);
   if(flags<0) {
        ng_perror("Could not get flags");
        return 0;
   }
   if (do_block) {
     flags &= ~O_NONBLOCK; //clear flag
     if(fcntl(module_data.peer_connections[partner], F_SETFL, flags)) {
       ng_perror("Could not set blocking operation for ESP");
       return 0;
     }
   } else {
     flags |= O_NONBLOCK; //set flag
     if (fcntl(module_data.peer_connections[partner], F_SETFL, flags) < 0) {
       ng_perror("Could not set non-blocking operation for ESP");
       return 0;
     }
   }
   return 1;
}

static int enet_esp_send(int dst, void *buffer, int size) {
   int sent = 0;
   int stop_tests = 0;
   
 again:
   /* send data */
   sent = send(
               module_data.peer_connections[dst],
               (void *)buffer,
               size,
               module_data.send_flags);
   
   if (sent == -1) {
      /* CTRL-C */
      if (stop_tests || errno == EINTR) {
         ng_error("enet_sendto()'s call to send() interrupted");
         return -1;
      }
      
      /* nonblocking operation would block - try again */
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
         goto again;
      }

      /* stop on errors */
      ng_perror("Mode %s send failed", module_data.name);
      return -1;
   }
   
   /* report success */
   return sent;
}

static int enet_esp_recv(int src, void *buffer, int size) {
   int rcvd = 0;
   int stop_tests = 0;
   
 again:
   rcvd = recv(
               module_data.peer_connections[src],
               (void *)buffer,
               size,
               module_data.recv_flags);
   
   if (rcvd == -1) {
      /* CTRL-C */
      if (stop_tests || errno == EINTR) {
         ng_error("recv() interrupted");
         return -1;
      }

      /* nonblocking operation would block - try again */
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
         goto again;
      }
      
      /* stop on errors */
      ng_perror("Mode %s recv failed", module_data.name);
      return -1;
   }
   
   /* report success */
   return rcvd;
}

/* module specific shutdown */
static void enet_esp_shutdown(struct ng_options *global_opts) {
   //TODO close all peer sockets
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
static void enet_esp_usage(void) {
/*
        int i;

        for (i=0; long_options_enet[i].name != NULL; i++) {
        ng_longoption_usage(
                long_options_enet[i].val,
                long_options_enet[i].name,
                long_option_infos_enet[i].desc,
                long_option_infos_enet[i].param
        );
  }
  */
}

/* module specific manpage information */
static void enet_esp_writemanpage(void) {
   /*
        int i;

        for (i=0; long_options_enet[i].name != NULL; i++) {
        ng_manpage_module(
                long_options_enet[i].val,
                long_options_enet[i].name,
                long_option_infos_enet[i].desc,
                long_option_infos_enet[i].param
        );
  }
  */
}
#else /* NG_MOD_ENET_ESP */
/*******************************************************************
 *    register stub 
 *******************************************************************/
int register_enet_esp(void) {
  return 0;
}
#endif
