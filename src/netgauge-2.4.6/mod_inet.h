/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef MOD_INET_H_
#define MOD_INET_H_

#include <string.h>	   /* memset & co. */
#include <sys/socket.h>    /* socket operations */
#include <sys/ioctl.h>     /* ioctl constants */
#include <net/if.h>        /* struct ifreq {for ioctl()} */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>    /* struct iphdr */
#include <netinet/udp.h>   /* struct udphdr */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "netgauge.h"

/* this constant is defined in <linux/in.h> which cannot be included
 * because it redefines some stuff from the other includes that must
 * be included because we need the stuff defined there... craps
 */
#define IP_MTU 14

/** default INET port */
#define INET_DEFAULT_PORT 2805

/* modified code from lib/interface.c:if_readconf(void) of net-tools (ifconfig, ...)
 * (http://bernd.eckenfels.net/net-tools.php);
 * fills "subnet" with local ip address when subnet given by "addr" and "mask"
 * is to be used
 */
static int inet_get_subnet(struct in_addr addr, struct in_addr mask, struct in_addr *subnet) {
   int numreqs = 30;
   struct ifconf ifc; // struct for all interfaces <net/if.h>
   struct ifreq *ifr_glob, ifr_loc; // per interface struct <net/if.h>
   struct in_addr if_addr, if_mask;
   
   char tmpbuf[64];
   int n, err = 1;
   int skfd; // socket filedescriptor
   
   // SIOCGIFCONF currently seems to only work properly on AF_INET sockets
   // (as of 2.1.128)
   // open socket for ioctl
   skfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
   if (skfd < 0) {
      ng_perror("No inet socket available");
      goto out;
   }
   
   ifc.ifc_buf = NULL;
   for (;;) {
      // allocate space for all ifreq structs
      ifc.ifc_len = sizeof(struct ifreq) * numreqs;
      ifc.ifc_buf = realloc(ifc.ifc_buf, ifc.ifc_len);
      
      if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
	 ng_perror("ioctl failed with SIOCGIFCONF");
	 goto out;
      }
      if (ifc.ifc_len == sizeof(struct ifreq) * numreqs) {
	 // assume it overflowed and try again
	 numreqs += 10;
	 continue;
      }
      break;
   } // end for
   
   // ifr_glob becomes the first ifreq struct
   ifr_glob = ifc.ifc_req;
   
   // loop through all interfaces (array)
   for (n = 0; n < ifc.ifc_len; n += sizeof(struct ifreq)) {
      // ifr_loc will be used by two ioctl's to get the
      // ip address and netmask of that interface;
      // for the ioctl to know which interface's data to get, the
      // ifr_name string of the returning structure is filled with
      // the interface's name
      strncpy(ifr_loc.ifr_name, ifr_glob->ifr_name, IFNAMSIZ);
      
      // get the ip address
      if (ioctl(skfd, SIOCGIFADDR, &ifr_loc) < 0) {
	 ng_perror("ioctl failed with SIOCGIFADDR");
	 goto out;
      }
      if_addr = ((struct sockaddr_in *) (&ifr_loc.ifr_addr))->sin_addr;
      
      // copy the ip address in std. dot notation into a char buffer
      // for later printing in user information, because inet_ntoa uses
      // a static buffer for buffering the string and we need two
      // subsequent calls to inet_ntoa which would falsify the result
      strcpy(tmpbuf, inet_ntoa(if_addr));
      
      // get the netmask
      if (ioctl(skfd, SIOCGIFNETMASK, &ifr_loc) < 0) {
	 ng_perror("ioctl failed with SIOCGIFNETMASK");
	 goto out;
      }
      // this is not portable, i hope the fix really fixes this...
	  //if_mask = ((struct sockaddr_in *) (&ifr_loc.ifr_netmask))->sin_addr;
      if_mask = ((struct sockaddr_in *) (&ifr_loc.ifr_addr))->sin_addr;
      
      ng_info(NG_VLEV2 | NG_VPALL, "Checking interface %s: ip=%s, netmask=%s",
	      ifr_loc.ifr_name,
	      tmpbuf,
	      inet_ntoa(if_mask)
	 );
      // check if the ioctl'ed ip and netmask match with the ones
      // passed as the function's parameters
      if ( (addr.s_addr & mask.s_addr) == (if_addr.s_addr & if_mask.s_addr) ) {
	 // return the matching ip address
	 subnet->s_addr = if_addr.s_addr;
	 ng_info(NG_VLEV1 | NG_VPALL, "Found IP to use: %s", inet_ntoa(*subnet));
	 err = 0;
	 // leave the for-loop
	 break;
      }
      
      // goto next ifreq struct (next interface)
      ifr_glob++;
   }  // end for
   
  out:
   free(ifc.ifc_buf);
   return err;
}
#endif
