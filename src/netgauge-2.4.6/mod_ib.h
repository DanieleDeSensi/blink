/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef MOD_IB_H_
#define MOD_IB_H_

#include "netgauge.h"
#include "infiniband/verbs.h"
#include "wnlib/wnhtab.h"

/****************************************************
 *           Public function prototypes             *
 ****************************************************/

static int ib_sendto(int dst, void *buffer, int size);
static int ib_recvfrom(int src, void* buffer, int size);
int ib_isendto(int dst, void *buffer, int size, NG_Request *req);
int ib_irecvfrom(int src, void *buffer, int size, NG_Request *req);
static void* ib_malloc(size_t size);
static int ib_init(struct ng_options *global_opts);
static int ib_select(int count, int *clients, unsigned long timeout);
int ib_test(NG_Request *req);
static int ib_set_blocking(int do_block, int partnet);
static int ib_getopt(int argc, char **argv, struct ng_options *global_opts);
static void ib_usage(void);
static void ib_writemanpage(void);
static void ib_shutdown(struct ng_options *global_opts);


/****************************************************
 *           Data structures for mod_ib             *
 ****************************************************/

/* registered memory area */
struct ib_mem_t {
  void *           start_addr;               /* start address of this memory area */
  size_t           size;                     /* size of this memory area */
  void *           end_addr;                 /* for convenience: start_addr + size */
  struct ibv_mr *  mr;                       /* handle of the registered memory */
  uint32_t         lkey;
  uint32_t         rkey;
};

/* connection information for one point-to-point connection */
struct ib_connection_t {
  struct ibv_cq *           send_cq;                /* CQ for send requests of this connection */
  struct ibv_cq *           recv_cq;                /* CQ for recv requests of this connection */
  struct ibv_qp *           q_pair;                 /* QP of this connection */
  struct ibv_port_attr      rem_port_attr;          /* Attributes of the remote port (LID, ...) */
  unsigned int              rem_qp_num;             /* remote QP number */
  void *                    recv_mem;               /* pointer to the pre-registered receive memory */
  
};

/* structure for work requests and scatter/gather lists */
struct ib_work_requests_t {
  struct ibv_send_wr *      send_wr;                /* pointer to an array of send work requests in the registered memory */
  struct ibv_recv_wr *      recv_wr;                /* pointer to an array of recv work requests in the registered memory */
  struct ibv_sge *          sge;                    /* pointer to an array of scatter-gather entries */
  struct ibv_wc *           wc;                     /* pointer to an array of work completion entries */
  int                       num_wr_s;               /* number of work requests in array of send wr */
  int                       num_wr_r;               /* number of work requests in array of recv wr */
  int                       num_sge;                /* number of elements in array of sge */
  int                       num_wc;                 /* number of completions in array of wc */
  uint64_t                  send_id;                /* id used to tag the send work request */
  uint64_t                  recv_id;                /* id used to tag the recv work request */
};

/* private data of the IB module */
static struct ib_module_t {
  int                         num_hca;                /* Number of the HCA to use */
  uint8_t                     port_num;               /* Port number to use; starts from 1 */
  struct ibv_context *        hca_handle;             /* handle to the HCA */
  struct ibv_pd *             pd_handle;              /* handle to the PD */
  struct ibv_port_attr        port_attr;              /* Attributes of the first port (LID, ...) */
  enum ibv_qp_type            tran_type;              /* transfer type RC, UC, UD */
  enum ibv_wr_opcode          tran_mode;              /* transfer mode Send-Recv, RDMA W, RDMA R */
  int                         world_size;             /* number of processes in this job */
  int                         pre_connect_all;        /* do all the connections have to be established during init? */
  struct ib_connection_t *    connections;            /* array of size world_size of all connections to remote nodes */
  struct ib_work_requests_t   work_req;               /* work requests have to be placed in registered memory */
  wn_htab                     mem_hash;               /* hash table of registered memory areas (from libwn) */
  unsigned long long          page_mask;              /* mask addresses to get a page boundary */
  unsigned int                page_bits;              /* number of bits relevant for pagesize bytes */
} ib_module;

/****************************************************
 *              Main module structure               *
 ****************************************************/

static struct ng_module module_ib = {
  .name           = "ib",
  .desc           = "Mode ib uses Infiniband for data transmission",
  .max_datasize   = -1,                                               /* can send data of arbitrary size */
  .headerlen      =  0,                                               /* no extra space needed for header */
  .flags          = NG_MOD_MEMREG | NG_MOD_RELIABLE | NG_MOD_CHANNEL, /* CHANNEL for UD? */
  .getopt         = ib_getopt,
  .malloc         = ib_malloc,
  .init           = ib_init,
  .shutdown       = ib_shutdown,
  .usage          = ib_usage,
  .writemanpage   = ib_writemanpage,
  .sendto         = ib_sendto,
  .recvfrom       = ib_recvfrom,
  .set_blocking   = ib_set_blocking,
  .isendto        = ib_isendto,
  .irecvfrom      = ib_irecvfrom,
  .test           = ib_test,
};

#endif

