/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/*  vim:sts=3:sw=3:ts=3:expandtab */

#include "netgauge.h"
#ifdef NG_MOD_TCP

#include "mod_inet.h"
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h> /* getopt long */

/* extern stuff */
extern struct ng_options g_options;

/* function prototypes */
static int tcp_setup_channels_MPI();
static int tcp_setup_channels_NOMPI();
static int tcp_sendto(int dst, void *buffer, int size);
static int tcp_recvfrom(int src, void *buffer, int size);
static int tcp_set_blocking(int do_block, int partner);
static int tcp_init(struct ng_options *global_opts);
static int tcp_getopt(int argc, char **argv, struct ng_options *global_opts);
static void tcp_writemanpage(void);
static void tcp_usage(void);
static void tcp_shutdown(struct ng_options *global_opts);
static int tcp_isendto(int dst, void *buffer, int size, NG_Request *req);
static int tcp_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static int tcp_test(NG_Request *req);
static int tcp_send_once(int dst, void *buffer, int size);
static int tcp_recv_once(int src, void *buffer, int size);

/* module registration data structure (TCP) */
struct ng_module tcp_module = {
   .name         = "tcp",
   .desc         = "Mode tcp uses TCP sockets for data transmission.",
   .flags        = NG_MOD_RELIABLE | NG_MOD_CHANNEL,
   .max_datasize = -1,        /*  can send data of arbitrary size */
   .headerlen    = 0,         /*  no extra space needed for header */
   .malloc       = NULL,
   .getopt       = tcp_getopt,
   .init         = tcp_init,
   .shutdown     = tcp_shutdown,
   .usage        = tcp_usage,
   .writemanpage = tcp_writemanpage,   
   .sendto         = tcp_sendto,
   .recvfrom       = tcp_recvfrom,
   .set_blocking   = tcp_set_blocking,
   .isendto        = tcp_isendto,
   .irecvfrom      = tcp_irecvfrom,
   .test           = tcp_test,
};

/**
 * getopt long options for inet
 */
static struct option long_options_ip[]={
   {"localaddr", required_argument, 0, 'L'},
   {"remoteaddr", required_argument, 0, 'R'},
   {"subnet", required_argument, 0, 'S'},
   {"port", required_argument, 0, 'P'},
   {"nonblocking",   required_argument, 0, 'N'},
   {0, 0, 0, 0}
};

/**
 * array of descriptions for inet
 */
static struct option_info long_option_infos_ip[]={
   {"local ip address to bind to", "ADDRESS"},
   {"hostname or ip address of the server (client mode only)", "PEER"},
   {"subnet to use for server and client (only valid when MPI is used)", "SUBNET/MASK"},
   {"server port to bind or connect to (not IP)", "NUMBER"},
        {"nonblocking [s]end and/or [r]eceive mode", "[s][r]"},
        {0, 0}
};

/* module private data */
struct tcp_private_data {
      int                         sock;
      int                         conn;
      unsigned short              port;
      int                         send_buf;
      int                         recv_buf;
      int                         send_flags;
      int                         recv_flags;
      struct in_addr              local_addr, remote_addr;
      struct sockaddr_in          inet_laddr, inet_raddr;
      const struct ng_mpi_options *mpi_opts;
      /** the connections to the other peers */
      int *peer_connections;
      /** my server address */
      struct sockaddr_in server_address;
      /** my server socket */
      int server_socket;
} module_data;


/* parse command line parameters for module-specific options */
static int tcp_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;
   
    char *optchars = "P:L:R:S:N:";  /*  additional module options */
   
   extern char *optarg;
   int option_index = 0;
   extern int optind, opterr, optopt;
   
   struct hostent *host;
   int failure = 0;
   
   /*  initialize module private information data structure */
   memset(&module_data, 0, sizeof(module_data));
   
   
   /*  parse module-options */
   while((c=getopt_long(argc, argv, optchars, long_options_ip, &option_index)) 
         >= 0 ) {
      switch( c ) {
    case '?':  /* unrecognized or badly used option */
       if (!strchr(optchars, optopt))
          continue;  /* unrecognized */
       ng_error("Option %c requires an argument", optopt);
       failure = 1;
       break;
    case 'P':  /* port */
       module_data.port = atoi(optarg);
       break;
    case 'L':  /* local address to bind to */
       /*  don't allow specifying a local address when using MPI */
       if (global_opts->mpi > 0) {
          ng_error("Option -L is not available when using MPI - use -S instead");
          failure = 1;
       } else {
         if (!inet_aton(optarg, &module_data.local_addr)) {
            ng_error("Malformed local address %s", optarg);
            failure = 1;
            continue;
         }
         ng_info(NG_VLEV2 | NG_VPALL, "Set local ip address %s", inet_ntoa(module_data.local_addr));
       }
       break;
    case 'R':  /* remote address to connect to */
       /*  don't allow specifying a remote address when using MPI */
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
        ng_info(NG_VLEV2 | NG_VPALL, "Set remote ip address %s", inet_ntoa(module_data.remote_addr));
       }
       break;
    case 'S':  /* subnet to use when using MPI */
       if (global_opts->mpi > 0) {
         /*  address and corresponding netmask for subnet */
         char *addr, *mask;
         char *pos;
         /*  address and corresponding netmask for subnet in */
         /*  "struct in_addr" representation */
         struct in_addr addr_in, mask_in;
         
         pos = strstr(optarg, "/");
         if (pos == NULL) {
            ng_error("Expected an \"address/netmask\" style argument");
            failure = 1;
            break;
         } else {
            /*  don't copy the user input but use "optarg" and make the slash */
            /*  to a '\0' */
            addr = optarg;
            mask = pos + 1;
            *pos = '\0';
         }
         /* printf("addr=%s, mask=%s\n", addr, mask); */
          
         if (!inet_aton(addr, &addr_in)) {
            ng_error("Malformed \"address\" part of argument %s/%s", addr, mask);
            failure = 1;
            break;
         }
         /*  get netmask from user input */
         pos = strstr(mask, ".");
         if (pos == NULL) { /*  assume user supplied the netmask as number (e.g. 24, 16,...) */
            /*  this is _not_ IPv6 ready !!! */
            mask_in.s_addr = htonl( ( (unsigned int) pow(2, atoi(mask) + 1) - 1 ) << (32 - atoi(mask)) );
         } else { /*  assume user supplied a netmask in std. dot notation */
            mask_in.s_addr = htonl(inet_network(mask));
            if (mask_in.s_addr == 0) {
              ng_error("Malformed \"netmask\" part of argument %s/%s",
               addr, mask);
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
    case 'N':  /* non-blocking send/receive mode */
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
   
   /*  check for necessary parameters, apply default values where possible */
   if (!failure) {
      if (!module_data.port) {
    module_data.port = INET_DEFAULT_PORT;
    ng_info(NG_VLEV2, "No port specified - using default %d", module_data.port);
      }
      
      if (!global_opts->mpi && !global_opts->server && 
     module_data.remote_addr.s_addr == 0) {
    ng_error("No remote address given for mode TCP - use option -R");
    failure = 1;
      }
      
#ifdef NG_MPI
      if (global_opts->mpi && module_data.local_addr.s_addr == 0) {
    ng_error("No subnet address/mask given for mode TCP - use option -S");
    failure = 1;
      }
#endif
      
   } /*  end if (!failure) */
   
   /*  report success or failure */
   return failure;
}

static int tcp_set_blocking(int do_block, int partner) {
   int flags = fcntl(module_data.peer_connections[partner], F_GETFL);
   if(flags<0) {
      ng_perror("Could not get flags");
   return 0;
   }
   if (do_block) {
     flags &= ~O_NONBLOCK; /* clear flag */
     if(fcntl(module_data.peer_connections[partner], F_SETFL, flags)) {
       ng_perror("Could not set blocking operation for TCP/IP");
       return 0;
     }
   } else {
     flags |= O_NONBLOCK; /* set flag */
     if (fcntl(module_data.peer_connections[partner], F_SETFL, flags) < 0) {
       ng_perror("Could not set non-blocking operation for TCP/IP");
       return 0;
     }
   }
   return 1;
}

static int tcp_send_once(int dst, void *buffer, int size) {
   int sent = 0;
   errno = 0;
   sent = send(
      module_data.peer_connections[dst],
      (void *)buffer,
      size,
      module_data.send_flags);
   if (sent == -1) {
      if (errno == EAGAIN) {
         return -EAGAIN;
      }
      ng_perror("Mode TCP send failed");
      return -1;
   }
   return sent;
}

static int tcp_sendto(int dst, void *buffer, int size) {
   int sent = 0;
   
  again:
   /* send data */
   sent = send(
      module_data.peer_connections[dst],
      (void *)buffer,
      size,
      module_data.send_flags);
   
   if (sent == -1) {
      /* CTRL-C */
      if (g_stop_tests || errno == EINTR) {
    return -1;
      }
      
      /* nonblocking operation would block - try again */
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
         goto again;
      }

      /* stop on errors */
      ng_perror("Mode TCP send failed");
      return -1;
   }
   
   /* report success */
   return sent;
}

static int tcp_recv_once(int src, void *buffer, int size) {
   int rcvd = 0;
   errno = 0;
   
   rcvd = recv(
      module_data.peer_connections[src],
      (void *)buffer,
      size,
      module_data.recv_flags);
   
   if (rcvd == -1) {
      if (errno == EAGAIN) {
         return -EAGAIN;
      }
      ng_perror("Mode TCP irecv failed");
      return -1;
   }
   return rcvd;
}

static int tcp_recvfrom(int src, void *buffer, int size) {
   int rcvd = 0;

  again:
   rcvd = recv(
      module_data.peer_connections[src],
      (void *)buffer,
      size,
      module_data.recv_flags);
   
   if (rcvd == -1) {
      /* CTRL-C */
      if (g_stop_tests || errno == EINTR) {
         ng_error("recv() interrupted");
         return -1;
      }

      /* nonblocking operation would block - try again */
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
         goto again;
      }
      
      /* stop on errors */
      ng_perror("Mode TCP recv failed");
      return -1;
   }
   
   /* report success */
   return rcvd;
}

int tcp_isendto(int dst, void *buffer, int size, NG_Request *req) {
   req_handle_t *sreq;
   /* init sending */
   sreq = malloc(sizeof(req_handle_t));
   sreq->type            = TYPE_SEND;
   sreq->index           = dst;
   sreq->buffer          = buffer;
   sreq->size            = size;
   sreq->remaining_bytes = size;
   *req = (NG_Request *) sreq;
   tcp_set_blocking(0, dst);    /* non-blocking */
   return tcp_test(req);
}

int tcp_irecvfrom(int src, void *buffer, int size, NG_Request *req) {
   req_handle_t *rreq;
   /* init receiving */
   rreq = malloc(sizeof(req_handle_t));
   rreq->type            = TYPE_RECV;
   rreq->index           = src;
   rreq->buffer          = buffer;
   rreq->size            = size;
   rreq->remaining_bytes = size;
   *req = (NG_Request *) rreq;
   tcp_set_blocking(0, src);    /* non-blocking */
   return tcp_test(req);
}

int tcp_test(NG_Request *req) {
   req_handle_t *nreq;
   int ret=0;
/* do the rest */
   nreq = (req_handle_t *)*req;
   if (nreq->remaining_bytes <= 0)
      return 0;

   if (get_req_type(req) == TYPE_RECV) {
      ret = tcp_recv_once(nreq->index, nreq->buffer, nreq->remaining_bytes);
      if (ret >= 0) {
         nreq->remaining_bytes -= ret;
         return nreq->remaining_bytes;
      } else if(ret == -EAGAIN) { /* was busy */
         return 1; /* in progress */
      }
   } else {
      ret = tcp_send_once(nreq->index, nreq->buffer, nreq->remaining_bytes);
      if (ret >= 0) {
         nreq->remaining_bytes -= ret;
         return nreq->remaining_bytes;
      } else if(ret == -EAGAIN) { /* was busy */
         return 1; /* in progress */
      }
   }
   return ret;
}


/* module specific benchmark initialization */
static int tcp_init(struct ng_options *global_opts) {
  if (!g_options.mpi) {
    /* we don't want or we don't have MPI */
    return tcp_setup_channels_NOMPI();
  } else {
    return tcp_setup_channels_MPI();
  }
}

/**
 * The simple one. Just two peers to connect.
 */
static int tcp_setup_channels_NOMPI() {
  int result;
  unsigned int cli_size;
  struct sockaddr_in cli;
  
  /* one for client, one for server */
  module_data.peer_connections = malloc(sizeof(int) * 2);

  if (g_options.server) {
    /* SERVER mode */
    const int partner = 0;

    ng_info(NG_VLEV2, "Initializing server socket.");
    module_data.server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&module_data.server_address, 0, sizeof(struct sockaddr_in));
    module_data.server_address.sin_family = AF_INET;
    module_data.server_address.sin_addr = module_data.local_addr;
    module_data.server_address.sin_port = htons(module_data.port);

    result = bind(module_data.server_socket,
        (struct sockaddr*)&module_data.server_address, 
        sizeof(struct sockaddr_in));
    
    if (result < 0) {
      ng_perror("Could not bind to port:");
      return 1;
    }
    
    ng_info(NG_VLEV2, "Bound network socket to %s:%d",
       inet_ntoa(module_data.server_address.sin_addr),
       ntohs(module_data.server_address.sin_port));
    
    if (listen(module_data.server_socket, 1) == -1) {
      ng_perror("listen() failed");
      return 1;
    }

    cli_size = sizeof(cli);
    ng_info(NG_VLEV1, "Waiting for client to connect...");
    module_data.peer_connections[partner] = accept(module_data.server_socket,
                     (struct sockaddr*)&cli,
                     &cli_size);
    
    ng_info(NG_VLEV2 | NG_VPALL, "accepted connection from %s:%d",
       inet_ntoa(cli.sin_addr),
       ntohs(cli.sin_port));
    
  } else {
    /* CLIENT mode */
    const int partner = 1;

    module_data.peer_connections[partner] =
      socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (module_data.peer_connections[partner] == -1) {
      ng_perror("Could not create socket:");
      return 1;
    }

    module_data.inet_raddr.sin_family = AF_INET;
    module_data.inet_raddr.sin_addr = module_data.remote_addr;
    module_data.inet_raddr.sin_port = htons(module_data.port);

    ng_info(NG_VLEV2, "Trying to connect to %s:%d",
          inet_ntoa(module_data.inet_raddr.sin_addr),
          ntohs(module_data.inet_raddr.sin_port));
    
    if (connect(module_data.peer_connections[partner],
      (struct sockaddr*)(&module_data.inet_raddr),
      sizeof(struct sockaddr_in)) < 0) {

      ng_perror("Mode TCP could not connect socket to %s:%d",
      inet_ntoa(module_data.inet_raddr.sin_addr),
      ntohs(module_data.inet_raddr.sin_port));
      return 1;
    }
    
    ng_info(NG_VLEV2 | NG_VPALL, "Connected network socket to %s:%d",
          inet_ntoa(module_data.inet_raddr.sin_addr),
          ntohs(module_data.inet_raddr.sin_port));
    
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
static int tcp_setup_channels_MPI() {
#ifdef NG_MPI
   const int peer_count = g_options.mpi_opts->worldsize;
   struct sockaddr_in *addresses = 
      malloc(peer_count * sizeof(struct sockaddr_in));
   int i, result, retries = 0;
   ng_info(NG_VLEV2, "Distributing IP adresses between %d peers.", peer_count);

   /* initialize the server socket for accepting connections */
   
   module_data.server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   memset(&module_data.server_address, 0, sizeof(struct sockaddr_in));
   module_data.server_address.sin_family = AF_INET;
   module_data.server_address.sin_addr = module_data.local_addr;
   module_data.server_address.sin_port = htons(module_data.port);
   
   do {
      result = bind(module_data.server_socket,
          (struct sockaddr*)&module_data.server_address, 
          sizeof(struct sockaddr_in));
      if (result < 0) {
    /* try next port */
    
    module_data.server_address.sin_port = htons(++module_data.port);
    ng_info(NG_VLEV1 | NG_VPALL,
       "Could not bind(), trying next port: %d", module_data.port);
      }
   } while ((result < 0) && (retries++ < 64));
   
   /* did we successfully bind()? */
   if (result < 0) {
      ng_perror("Mode TCP could not bind socket to %s:%d",
      inet_ntoa(module_data.server_address.sin_addr),
      ntohs(module_data.server_address.sin_port));
      return 1;
   }
   
   ng_info(NG_VLEV2 | NG_VPALL, "Bound network socket to %s:%d",
      inet_ntoa(module_data.server_address.sin_addr),
      ntohs(module_data.server_address.sin_port));
   
   /* distribute the addresses / ports */
   
   if (MPI_Allgather(&(module_data.server_address),
           sizeof(struct sockaddr_in), MPI_CHAR,
           addresses, sizeof(struct sockaddr_in), MPI_CHAR,
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
          socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
       
       if (connect(module_data.peer_connections[j],
         (struct sockaddr*)(addresses + j),
         sizeof(struct sockaddr_in)) < 0) {
          ng_perror("Mode TCP could not connect socket to %s:%d",
          inet_ntoa(addresses[j].sin_addr),
          ntohs(addresses[j].sin_port));
          return 1;
       }
       
       ng_info(NG_VLEV2 | NG_VPALL, "Connected network socket to %s:%d",
          inet_ntoa(addresses[j].sin_addr),
          ntohs(addresses[j].sin_port));
       
    }
    
      } else {
    /* accept the connection from peer i */
    if (i <= g_options.mpi_opts->worldrank) {
       unsigned int cli_size;
       struct sockaddr_in cli;

       /* start listening */
       if (listen(module_data.server_socket, 1) == -1) {
          ng_error("listen() failed");
          return 1;
       }
       
       MPI_Barrier(MPI_COMM_WORLD);
       
       cli_size = sizeof(cli);
       module_data.peer_connections[i] = accept(module_data.server_socket,
                       (struct sockaddr*)&cli,
                       &cli_size);

       ng_info(NG_VLEV2 | NG_VPALL, "accepted connection from %s:%d",
          inet_ntoa(cli.sin_addr),
          ntohs(cli.sin_port));
       
    } else {
       /* just make the barrier happy */
       MPI_Barrier(MPI_COMM_WORLD);
    }
      }
      
   }

   free(addresses);

   ng_info(NG_VLEV1, "Peer connections established.");

   return 0;
#else
   /* we should have called the NOMPI version of this
    * function - if get here, it's in error */
   return 1;
#endif
}

/* module specific shutdown */
static void tcp_shutdown(struct ng_options *global_opts) {
  /*  close accepted socket (if any) */
  if (module_data.conn > 0) {
/*    // print socket statistics ???
      if (global_opts->verbose > 1) {
      }
*/
    ng_info(NG_VLEV2, "Closing accepted network socket");
    if (close(module_data.conn) < 0)
     ng_perror("Failed to close accepted network socket");
  }
   
  if (module_data.sock > 0) {
/*    // print socket statistics ???
      if (global_opts->verbose > 1) {
      }
*/
    ng_info(NG_VLEV1, "Closing network socket");
    if (close(module_data.sock) < 0)
     ng_perror("Failed to close network socket");
  }
}

/* module specific manpage information */
static void tcp_writemanpage(void) {
   int i;
   
   for (i=0; long_options_ip[i].name != NULL; i++) {
      ng_manpage_module(
         long_options_ip[i].val, 
         long_options_ip[i].name, 
         long_option_infos_ip[i].desc,
         long_option_infos_ip[i].param
      );
  }
}

/* module specific usage information */
static void tcp_usage(void) {
   int i;
   
   for (i=0; long_options_ip[i].name != NULL; i++) {
      ng_longoption_usage(
         long_options_ip[i].val, 
         long_options_ip[i].name, 
         long_option_infos_ip[i].desc,
         long_option_infos_ip[i].param
      );
  }
}


/* module registration */
int register_tcp(void) {
   ng_register_module(&tcp_module);
   return 0;
}
#else

/* dummy module registration */
int register_tcp(void) {
   return 0;
}

#endif
