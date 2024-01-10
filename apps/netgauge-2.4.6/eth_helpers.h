/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef __ETH_HELPER_H__
#define __ETH_HELPER_H__

#include <sys/socket.h>
#include <net/if.h>
#if defined HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#elif defined HAVE_SYS_TYPES_H  && defined HAVE_SYS_TYPES_H && defined HAVE_NET_ETHERNET_H && defined HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#endif
#include <string.h>
#include <sys/ioctl.h>

static int get_if_addr(int sock, const char *ifname, struct ether_addr *address) {
#if defined SIOCGIFHWADDR
   struct ifreq if_data;

   memset(&if_data, 0, sizeof(if_data));
   strncpy(if_data.ifr_name, ifname, 5);
   if (ioctl(sock, SIOCGIFHWADDR, &if_data) < 0) {
      ng_perror("Could not determine address of local interface %s", if_data.ifr_name);
      return 1;
   } else {
      memcpy(address, if_data.ifr_hwaddr.sa_data, ETH_ALEN);
      ng_info(NG_VLEV1 | NG_VPALL, "Determined local interface %s address %s", if_data.ifr_name, ether_ntoa(address));
      return 0;
   }
#else

 ng_perror("This module is not supported for your OS.");

#endif

}
#endif
