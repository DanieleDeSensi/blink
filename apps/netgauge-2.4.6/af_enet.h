/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef AF_ENET_H_
#define AF_ENET_H_

#include <linux/types.h>
#include <sys/socket.h>	/* sa_family_t */
#include <linux/if_ether.h>	/* ETH_ALEN */


/*
 * Protocol and Address Families to use for ENET
 * (choose values currently unused by the kernel, see
 *  AF_* constants in %KERNEL_SRC%/include/linux/socket.h)
 */
#define AF_ENET 28
#define PF_ENET AF_ENET

/*
 * Socket Level ENET constant to use with get/setsockopt()
 * (choose a value currently unused by the kernel, see
 *  SOL_* constants in %KERNEL_SRC%/include/linux/socket.h)
 */
#define SOL_EP  285


typedef __u16 en_port_t;


/* set the interface index to ENET_INTERFACE_ANY when binding a
 * server socket to listen on all available interfaces */
#define ENET_INTERFACE_ANY 0


/* ENET address struct for socket calls */
struct sockaddr_en
{
      sa_family_t	sen_family;		/*< address family */
      en_port_t	        sen_port;               /*< port */
      unsigned int	sen_ifindex;		/*< interface index */
      unsigned char	sen_addrlen;		/*< address length (should always be == ATH_ALEN) */
      unsigned char	sen_addr[ETH_ALEN];	/*< ethernet (mac) address */
      /* pad to size of struct sockaddr */
      unsigned char sen_zero[
	 sizeof(struct sockaddr) -
	 sizeof(sa_family_t) -
	 sizeof(en_port_t) -
	 sizeof(int) -
	 sizeof(char) -
	 ETH_ALEN ];
};

/* EDP - Ethernet Datagram Protocol
 * provides an unreliable datagram transport via Ethernet
 */


/*
 * EDP Protocols to use with the socket() system call
 */
enum __edp_protos {
   EPPROTO_EDP  = 0,	/* standard ethernet datagram protocol */
   EPPROTO_EZDP = 1	/* (too) simple zero-copy sending approach */
};


/*
 * EDP socket options
 */
enum __edp_so {
   EDP_SO_MIN   = 0,
   EDP_SO_MTU   = 1,
   EDP_SO_STATS = 2,
   EDP_SO_END
};


/*
 * EDP control message header
 */
struct edp_cmsg_header {
      unsigned char msg_type;    /**< control message type */
} __attribute__((packed));


/*
 * EDP control message codes
 */
enum __edp_cmsg_types {
   EDP_CMSG_START   = 0,
   EDP_ECHO_REQUEST = 1,
   EDP_ECHO_REPLY   = 2,
   EDP_CMSG_END
};


#define EDP_CMSG_HLEN sizeof(struct edp_cmsg_header)


/* EDP socket stats */
struct edp_stats {
   unsigned int	pkt_tx;		/**< packets sent */
   unsigned int	tx_err;		/**< send errors */
   unsigned int	pkt_rx;		/**< packets received */
   unsigned int	pkt_rx_drop;	/**< packets dropped */
};


/* ESP - Ethernet Streaming Protocol
 * provides reliable and ordered datagram transport via Ethernet
 */


/*
 * ESP Protocols to use with the socket() system call
 */
enum __esp_protos {
   EPPROTO_ESP  = 0	/* standard ethernet streaming protocol */
};


/*
 * ESP socket options
 */
enum __esp_so {
   ESP_SO_MIN   = 0,
   ESP_SO_MTU   = 1,
   ESP_SO_STATS = 2,
   ESP_SO_END
};


/* ESP socket stats */
struct esp_stats {
   unsigned int pkt_tx;      /**< packets sent */
   unsigned int pkt_rtx;     /**< re-transmissions */
   unsigned int tx_err;      /**< send errors */
   unsigned int pkt_rx;      /**< packets received */
   unsigned int pkt_rx_oos;  /**< out-of-sequence receives */
   unsigned int pkt_rx_drop; /**< packets dropped */
   unsigned int seq_lost;    /**< packets with seq. # > expected recieved */
   unsigned int arq_tx;
};

/* Ethernet address functions and macros */


/* ETH_ADDR_NONE - Test for a zero ethernet address (00:00:00:00:00:00)
 * @addr:   char eth_addr[]
 */
#define ETH_ADDR_NONE(addr)					\
	(!((*((__u32*)&addr[0])) | (*((__u16*)&addr[4]))))


/* ETH_ADDR_CMP - Ethernet address comparison (48 bits...)
 * @addr1:   char eth_addr1[]
 * @addr2:   char eth_addr2[]
 * Returns zero for equal addresses and a non-zero value otherwise
 */
#define ETH_ADDR_CMP(addr1, addr2)				\
	((*((__u32*)&addr1[0]) ^ *((__u32*)&addr2[0])) |	\
	 (*((__u16*)&addr1[4]) ^ *((__u16*)&addr2[4])))


/* ETH_ADDR_CPY - Ethernet address copying (48 bits...) 
 * @dst:   char destination[]
 * @src:   char source[]
 */
#define ETH_ADDR_CPY(dst, src)					\
	(*((__u32*)&(dst)[0]) = *((__u32*)&(src)[0]));		\
	(*((__u16*)&(dst)[4]) = *((__u16*)&(src)[4]));


/* Ethernet address output */
#define NETH_HEX(addr)		\
	(unsigned char)addr[0], \
	(unsigned char)addr[1],	\
	(unsigned char)addr[2],	\
	(unsigned char)addr[3],	\
	(unsigned char)addr[4],	\
	(unsigned char)addr[5]
#define NETH_FORMAT "%02x:%02x:%02x:%02x:%02x:%02x"

/* For sysctl access.
 * 
 * If this table should be shown under /proc/sys/net/esp (I think this
 * would be a nice place), changes to net/sysctl_net.c would be needed.
 * Therefore we just add it to toplevel, eg. /proc/sys/esp
 */
enum {
   CTL_ESP = 313, /* < must be unused, see linux/sysctl.h */
   CTL_BURST_LENGTH = 1,
   CTL_INITIAL_ACK_BURST_LENGTH = 2,
   CTL_PACKETS_TO_ACK = 3,
   CTL_ROUND_TRIP_TIME = 4,
   CTL_SEND_BUFF = 5,
   CTL_RECV_BUFF = 6
};



#endif /* AF_ENET_H_ */
