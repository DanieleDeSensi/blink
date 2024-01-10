/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef MOD_ENET_H_
#define MOD_ENET_H_

#include <string.h>		/* memset & co. */
#include <sys/socket.h>		/* socket operations */
#include <net/ethernet.h>	/* struct ether_addr, ethernet protocol constants */
#include <netinet/ether.h>	/* ethernet address conversion functions */
#include <netinet/in.h>
#include <sys/ioctl.h>		/* ioctl constants */
#include <net/if.h>		/* struct ifreq {for ioctl()} */

#include "netgauge.h"
#include "af_enet.h"


#define ETH_ADDR(arr) (*((struct ether_addr*)arr))


/* default ENET port */
#define ENET_DEFAULT_PORT 2805



#endif
