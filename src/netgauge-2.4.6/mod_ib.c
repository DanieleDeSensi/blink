/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_OFED && defined NG_MOD_IB

#include <unistd.h>
#include <math.h>
#include <string.h>
#include "wnlib/wnhtab.h"
#include "wnlib/wnhtbl.h"
#include "mod_ib.h"
#include "infiniband/verbs.h"
#include "mpi.h"

// fix for -std=c99 issue - compiler warning
// define log2 here ...
#ifndef _ISOC99_SOURCE
double log2(double x);
#endif

static struct option    ib_options[] = {
  {"transtype" , required_argument, NULL, 'T'},
  {"transmode" , required_argument, NULL, 'M'},
  {"hcanum"    , required_argument, NULL, 'H'},
  {"preconnect", required_argument, NULL, 'C'},
  {NULL        , 0                , NULL, '0'}
};

static struct option_info   ib_opt_info[] = {
  {"transport type [u]n/[r]eliable connection, [U]nreliable datagram", "[u]|[r]|[U]|"},
  {"transport mode [s]end, RDMA [w]rite or [r]read", "[s]|[w]|[r]"},
  {"number of HCA to use (integer number)", "[0]|[1]|[2] .... [n]"},
  {"establish all connections during [i]nitialization or on [d]emand during the first send", "[i]|[d]"},
  {NULL, NULL}
};

/*******************************************************************
 *                 Helper function prototypes                      *
 *******************************************************************/
int create_connection(int mpi_rank);
int destroy_connection(int mpi_rank);
uint32_t get_lkey(void *addr);
int ib_getpagesize(void);
void ib_dereg_memory(void *data, void *key);
int from_err_to_rts(int mpi_rank);
int from_rst_to_rts(int mpi_rank);
int from_rts_to_sqd_to_rts(int mpi_rank);
int from_sqe_to_rts(int mpi_rank);
enum ibv_qp_state is_qp_erroneous(struct ibv_qp *qp);


/*******************************************************************
 *     Functions of the module interface and helper functions      *
 *******************************************************************/
int register_ib(void) {
  ng_register_module(&module_ib);
  return 0;
}

static int ib_sendto(int dst, void *buffer, int size) {

//int temp; //REMOVE THIS
  int                    retval;
  struct ibv_send_wr     *send_wr, *bad_wr;
  struct ibv_wc          wc;

  send_wr = ib_module.work_req.send_wr;

  /* fill in data for the send work request */
  send_wr->next       = NULL;
  send_wr->wr_id      = ib_module.work_req.send_id++;
  send_wr->sg_list    = &ib_module.work_req.sge[0];
  send_wr->sg_list->addr   = (uint64_t) buffer;
  send_wr->sg_list->length = size;
  send_wr->sg_list->lkey   = get_lkey(buffer);
  send_wr->num_sge    = 1;
  send_wr->opcode     = IBV_WR_SEND;
  send_wr->send_flags = IBV_SEND_SIGNALED;

  /* post the send request; in case the recv request is not present retry as long
   * as the poll operation returns the status IBV_WC_RETRY_EXC_ERR
   * or IBV_WC_RNR_RETRY_EXC_ERR; this should cover the case when the sender is a
   * bit too early */
//MPI_Comm_rank(MPI_COMM_WORLD, &temp); printf("[ib_sendto] %d: Sending data to %d of size %d\n", temp, dst, send_wr->sg_list->length);
  do {
    retval = ibv_post_send(ib_module.connections[dst].q_pair, send_wr, &bad_wr);
    if (0 != retval) {
      ng_error("Unable to post send request in function ib_sendto");
      return -1;
    }
//printf("[ib_sendto] %d: Data sent\n", temp);

    /* poll for local completion of the work request */
    do {
      retval = ibv_poll_cq(ib_module.connections[dst].send_cq, 1, &wc);
    } while (0 == retval);
//printf("[ib_sendto] %d: Completion Queue polling complete\n", temp);

    if (IBV_QPS_SQE == is_qp_erroneous(ib_module.connections[dst].q_pair)) {
      ng_info(NG_VLEV1, "[ib_sendto]: QP is in state IBV_QPS_SQE!\n");
      from_sqe_to_rts(dst);
//if (IBV_QPS_SQE == is_qp_erroneous(ib_module.connections[dst].q_pair)) printf("[ib_sendto] %d: QP STILL in state IBV_QPS_SQE!\n", temp);
    } else if (IBV_QPS_ERR == is_qp_erroneous(ib_module.connections[dst].q_pair)) {
      ng_info(NG_VLEV1, "[ib_sendto]: QP is in state IBV_QPS_ERR!\n");
      from_err_to_rts(dst);
//if (IBV_QPS_ERR == is_qp_erroneous(ib_module.connections[dst].q_pair)) printf("[ib_sendto] %d: QP STILL in state IBV_QPS_ERR!\n", temp);
    }

//if(retval < 0)printf("[ib_sendto] %d: ib_poll_cq returned a negative value\n", temp);
//else printf("[ib_sendto] %d: completion status of the send request is %s\n", temp, ib_get_error(wc.status));

  } while ((0 < retval) && ((IBV_WC_RETRY_EXC_ERR == wc.status) || (IBV_WC_RNR_RETRY_EXC_ERR == wc.status) ||
           (IBV_WC_REM_INV_REQ_ERR == wc.status) || (IBV_WC_WR_FLUSH_ERR == wc.status)));

  if ((retval < 0) || (IBV_WC_SUCCESS != wc.status)) {
    ng_error("Failed to poll completion queue of the send queue of connection %d in function ib_sendto", dst);
    return -1;
  }

  if (IBV_WC_SEND != wc.opcode) {
    ng_error("Completion of send work request expected but a different type of work request completed in function ib_sendto");
    return -1;
  }
//printf("[ib_sendto] %d: exiting function\n", temp);

  return size;
}

static int ib_recvfrom(int src, void* buffer, int size) {

  int                   retval, iter_cnt;
  struct ibv_recv_wr    *recv_wr, *bad_wr;
  struct ibv_wc         wc;

  recv_wr = ib_module.work_req.recv_wr;

  /* fill in data for the recv work request */
  recv_wr->next    = NULL;
  recv_wr->wr_id   = ib_module.work_req.send_id++;
  recv_wr->sg_list = &ib_module.work_req.sge[1];
  recv_wr->sg_list->addr   = (uint64_t) buffer;
  recv_wr->sg_list->length = size;
  recv_wr->sg_list->lkey   = get_lkey(buffer);
  recv_wr->num_sge = 1;

//printf("[ib_recvfrom]: posting receive request to get data from %d of size %d\n", src, recv_wr->sg_list->length);fflush(NULL);
  do {

    iter_cnt = 0; /* count iterations for ibv_post_recv */
    do {
      retval = ibv_post_recv(ib_module.connections[src].q_pair, recv_wr, &bad_wr);
      if (5 < ++iter_cnt) {
        from_rts_to_sqd_to_rts(src);
      }
//if (errno == 115) {printf(".");}
    } while ((0 != retval) && ((11 == errno) || (115 == errno))); 
    /* repeat while errno says "Resource temporarily unavailable", which is code 11
       or "Operation now in progress", which is code 115 */

    if (0 != retval) {
      ng_error("Unable to post receive request in function ib_recvfrom (%d, %s) %d", errno, strerror(errno), is_qp_erroneous(ib_module.connections[src].q_pair));
      return -1;
    }

    /* poll for completion of the receive request */
    do {
      retval = ibv_poll_cq(ib_module.connections[src].recv_cq, 1, &wc);
    } while (0  == retval);
//printf("[ib_recvfrom]: Completion Queue polling complete for source %d; status of work request is %s\n", src, ib_get_error(wc.status));

    if (IBV_QPS_SQE == is_qp_erroneous(ib_module.connections[src].q_pair)) {
      from_sqe_to_rts(src);
    } else if (IBV_QPS_ERR == is_qp_erroneous(ib_module.connections[src].q_pair)) {
      from_err_to_rts(src);
    }
  } while ((0 < retval) && ((wc.status == IBV_WC_LOC_LEN_ERR) || (wc.status == IBV_WC_WR_FLUSH_ERR)));

  if ((retval < 0) || (IBV_WC_SUCCESS != wc.status)) {
    ng_error("Failed to poll completion queue of the receive queue of connection %d in function ib_recvfrom %d", src, wc.status);
    return -1;
  }

  if (IBV_WC_RECV != wc.opcode) {
    ng_error("Completion of receive work request expected but a different type of work request completed in function ib_recvfrom");
    return -1;
  }

  return size;
}

int ib_isendto(int dst, void *buffer, int size, NG_Request *req) {

  ng_abort("the function ib_isendto is currently not implemented\n");
  return -1;
}

int ib_irecvfrom(int src, void *buffer, int size, NG_Request *req) {

  ng_abort("the function ib_irecvfrom is currently not implemented\n");
  return -1;
}

static void* ib_malloc(size_t size) {

  int                 i, retval;
  void                *mem;
  unsigned long long  mem_key;
  struct ib_mem_t     *ib_mem, *bad_mem;

#if 0
  /* find a free entry in the array of registered memory areas;
   * quite slow, but memory allocation and registration is horribly slow anyway */
  i = 0;
  while (i < ib_module.num_mem) {
    if (NULL == ib_module.ib_mem[i].start_addr) {
      /* we found the next free entry */
      break;
    }
    i++;
  }

  /* check if there was a free entry */
  if (NULL != ib_module.ib_mem[i].start_addr) {
    /* no free entries available; use realloc to get more entries */
    ib_module.ib_mem =  realloc(ib_module.ib_mem, 2 * ib_module.num_mem * sizeof(struct ib_mem_t));
    ib_module.num_mem *= 2;
    /* finally set i to the next free entry */
    i++;
  }
#endif

  ib_mem = malloc(sizeof(struct ib_mem_t)); /* allocate IB memory item to store in the hash table */
  mem    = malloc(size); /* allocate user's memory */
  if ((NULL ==  mem) || (NULL == ib_mem)) {
    ng_error("Unable to allocate memory in function ib_malloc");
    return NULL;
  }

  ib_mem->mr = ibv_reg_mr(ib_module.pd_handle, mem, size,
                          (enum ibv_access_flags) (IBV_ACCESS_LOCAL_WRITE |
                          IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ));
  if (NULL == ib_mem->mr) {
    ng_error("Unable to register memory in function ib_malloc");
    return NULL;
  } 

  ib_mem->start_addr = mem;
  ib_mem->size       = size;
  ib_mem->end_addr   = (void *) (((uint8_t*) mem) + size);
  ib_mem->lkey       = ib_mem->mr->lkey;
  ib_mem->rkey       = ib_mem->mr->rkey;

  mem_key = (((unsigned long long) mem) & ib_module.page_mask);
  retval = (int) wn_hins(ib_mem, ib_module.mem_hash, (void *) mem_key);
  if (!retval) {
    ng_error("Unable to insert key in hash table in function ib_malloc");
    return NULL;
  }

  return mem;
}


static int ib_init(struct ng_options *global_opts) {

  int                          i, pagesize;
  int                          num_devs;                 /* number of HCAs */
  struct ibv_device **         all_devices;              /* list of available HCAs */

  ib_module.port_num = 1; /* change this to a user specified parameter */

  /* Get MPI job size */
  MPI_Comm_size(MPI_COMM_WORLD, &ib_module.world_size);

  /* request memory to hold connection information */
  ib_module.connections = malloc(ib_module.world_size * sizeof(struct ib_connection_t));
  if (NULL == ib_module.connections) {
    ng_error("Unable to allocate memory for the connections array in ib_init");
    return 1;
  }

  /* Get all available devices */
  all_devices = ibv_get_device_list(NULL);
  for (num_devs =  0; all_devices[num_devs] != NULL; num_devs++);

  /* Devices numbered from 0 to n-1 */
  if (ib_module.num_hca > num_devs) {
    ng_error("There is no HCA with number %d; please use --help to get a list of available HCAs", ib_module.num_hca);
    return 1;
  }

  /* Open the HCA */
  ib_module.hca_handle = ibv_open_device(all_devices[ib_module.num_hca]);
  if (NULL == ib_module.hca_handle) {
    ng_error("Unable to get HCA handle in function ib_init");
    return 1;
  }

  /* Request a Protection Domain */
  ib_module.pd_handle = ibv_alloc_pd(ib_module.hca_handle);
  if (NULL == ib_module.pd_handle) {
    ng_error("Unable to get Protection Domain in function ib_init");
    return 1;
  }

  /* Query port information (incl. LID) */
  ibv_query_port(ib_module.hca_handle, ib_module.port_num, &ib_module.port_attr);

  /* Establish all connections now if this behavior was requested */
  if (ib_module.pre_connect_all) {
    for (i = 0; i < ib_module.world_size; i++)
      if (create_connection(i))
        return 1;
  } else {
    /* To add support for lazy connection setup one has to add the appropriate code
     * in sento and recvfrom (also non-blocking versions). This requires a handshake
     * between send and recv to exchange LID and QP number. Thus, the pattern has to
     * work correctly to ensure not messing up the order of sends and receives */
    ng_error("Sorry, lazy connection setup is currently not supported");
    return 1;
  }

  /* Request a small amount of memory to host work requests */
  ib_module.work_req.send_wr = (struct ibv_send_wr *) malloc(sizeof(struct ibv_send_wr));
  ib_module.work_req.recv_wr = (struct ibv_recv_wr *) malloc(sizeof(struct ibv_recv_wr));
  ib_module.work_req.sge     = (struct ibv_sge *) malloc(2 * sizeof(struct ibv_sge));
  ib_module.work_req.wc      = (struct ibv_wc *) malloc(sizeof(struct ibv_wc));
  if ((NULL == ib_module.work_req.send_wr) || (NULL == ib_module.work_req.recv_wr) ||
      (NULL == ib_module.work_req.sge) || (NULL == ib_module.work_req.wc)) {
    ng_error("Unable to allocate memory for work requests in function ib_init");
    return 1;
  }
  ib_module.work_req.num_wr_s = 1;
  ib_module.work_req.num_wr_r = 1;
  ib_module.work_req.num_sge  = 2;
  ib_module.work_req.num_wc   = 1;
  ib_module.work_req.send_id  = 0;
  ib_module.work_req.recv_id  = 0;

  /* initialize management of registered memory areas for ib_malloc() */
  wn_mkptrhtab(&ib_module.mem_hash);
  if (NULL == ib_module.mem_hash) {
    ng_error("Unable to create hash table in function ib_init");
    return 1;
  }

  /* determine number of bits that has to be masked to get a page boundary */
  pagesize = ib_getpagesize();
  ib_module.page_mask = (~0) << ((unsigned long long) log2((double) pagesize));
  ib_module.page_bits = (unsigned int) log2((double) pagesize);
/*TODO: REMOVE THIS PRINTF */ printf("Using page size of %d bytes (%d mask bits)\n", pagesize, (unsigned int) log2((double) pagesize));

#if 0
  ib_module.ib_mem  = malloc(ib_module.num_mem * sizeof(struct ib_mem_t));
  if (NULL == ib_module.ib_mem) {
    ng_error("Unable to allocate memory in function ib_init");
    return 1;
  }
  for (i = 0; i < ib_module.num_mem; i++) {
    ib_module.ib_mem[i].start_addr = ib_module.ib_mem[i].end_addr = NULL;
    ib_module.ib_mem[i].size = 0;
  }
#endif

  return 0;
}

int create_connection(int mpi_rank) {

  int                            retval;
  struct ibv_qp_init_attr        qp_attr;          /* initial QP attributes */
  MPI_Status                     status;
 
  /* create send Completion Queue */
  ib_module.connections[mpi_rank].send_cq = ibv_create_cq(ib_module.hca_handle,
                                                          32, NULL, NULL, 0);
  if (NULL == ib_module.connections[mpi_rank].send_cq) {
    ng_error("Unable to create a CQ in create_connection");
    return 1;
  }

  /* create recv Completion Queue */
  ib_module.connections[mpi_rank].recv_cq = ibv_create_cq(ib_module.hca_handle,
                                                          255, NULL, NULL, 0);
  if (NULL == ib_module.connections[mpi_rank].recv_cq) {
    ng_error("Unable to create a CQ in create_connection");
    return 1;
  }

  /* create QP */
  qp_attr.qp_context = ib_module.hca_handle;
  qp_attr.send_cq    = ib_module.connections[mpi_rank].send_cq;
  qp_attr.recv_cq    = ib_module.connections[mpi_rank].recv_cq;
  qp_attr.srq        = NULL;
  qp_attr.cap.max_send_wr     = 255;
  qp_attr.cap.max_recv_wr     = 255;
  qp_attr.cap.max_send_sge    = 2;
  qp_attr.cap.max_recv_sge    = 2;
  qp_attr.cap.max_inline_data = 400; /* Why 400 ??? */
  qp_attr.qp_type    = ib_module.tran_type;
  qp_attr.sq_sig_all = 1;

  ib_module.connections[mpi_rank].q_pair = ibv_create_qp(ib_module.pd_handle,
                                                         &qp_attr);
  if (NULL == ib_module.connections[mpi_rank].q_pair) {
    ng_error("Unable to create a QP in create_connection");
    return 1;
  }

  /* Exchange QP number and LID */
  MPI_Sendrecv(&ib_module.port_attr.lid, 1, MPI_UNSIGNED_SHORT, mpi_rank, 22,
               &ib_module.connections[mpi_rank].rem_port_attr.lid, 1, MPI_UNSIGNED_SHORT,
	       mpi_rank, 22, MPI_COMM_WORLD, &status);

  MPI_Sendrecv(&ib_module.connections[mpi_rank].q_pair->qp_num, 1, MPI_UNSIGNED, mpi_rank, 33,
               &ib_module.connections[mpi_rank].rem_qp_num, 1, MPI_UNSIGNED, mpi_rank, 33,
	       MPI_COMM_WORLD, &status);

  retval = from_rst_to_rts(mpi_rank);
  if  (0 != retval) {
    ng_error("Unable to get QP to state RTS in create_connection");
    return 1;
  }

  return 0;
}

/* This function modifies the QP state of the connection belonging to
 * the mpi rank from RST to RTS again. Useful during initialization and error handling */
int from_rst_to_rts(int mpi_rank) {

  int                            retval;
  struct ibv_qp_attr             qp_a;             /* QP attributes */

  /* Modify QP to init state */
  memset(&qp_a, 0, sizeof(struct ibv_qp_attr));
  qp_a.qp_state        = IBV_QPS_INIT;
  qp_a.pkey_index      = 0;
  qp_a.port_num        = ib_module.port_num; /* Port numbers start with 1 */
  qp_a.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_LOCAL_WRITE;

  retval = ibv_modify_qp(ib_module.connections[mpi_rank].q_pair, &qp_a,
                         (enum ibv_qp_attr_mask) (IBV_QP_STATE | IBV_QP_PKEY_INDEX |
                         IBV_QP_PORT | IBV_QP_ACCESS_FLAGS));
  if (0 != retval) {
    ng_error("Unable to modify QP of connection %d to state INIT; errno: %s", mpi_rank, strerror(errno));
    return 1;
  }

  /* Modify QP to RTR state */
  memset(&qp_a, 0, sizeof(struct ibv_qp_attr));
  qp_a.qp_state           = IBV_QPS_RTR;
  qp_a.path_mtu           = ib_module.port_attr.max_mtu;
  qp_a.dest_qp_num        = ib_module.connections[mpi_rank].rem_qp_num;
  qp_a.rq_psn             = 0;
  qp_a.max_dest_rd_atomic = 4; /* default value from Open MPI */
  qp_a.min_rnr_timer      = 5; /* default value from Open MPI */
  qp_a.ah_attr.is_global     = 0;
  qp_a.ah_attr.dlid          = ib_module.connections[mpi_rank].rem_port_attr.lid;
  qp_a.ah_attr.sl            = 0;
  qp_a.ah_attr.src_path_bits = 0;
  qp_a.ah_attr.port_num      = ib_module.port_num; /* Port numbers start with 1 */
  qp_a.ah_attr.static_rate   = 0;

  retval = ibv_modify_qp(ib_module.connections[mpi_rank].q_pair, &qp_a,
                         (enum ibv_qp_attr_mask) (IBV_QP_STATE | IBV_QP_AV |
                         IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                         IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER));
  if (0 != retval) {
    ng_error("Unable to modify QP of connection %d to state RTR; errno: %s", mpi_rank, strerror(errno));
    return 1;
  }

  /* Modify QP to RTS state */
  memset(&qp_a, 0, sizeof(struct ibv_qp_attr));
  qp_a.qp_state      = IBV_QPS_RTS;
  qp_a.timeout       = 10; /* default value from Open MPI */
  qp_a.retry_cnt     = 255; /* default Open MPI is 7, but I guess in our case should be max value of uint8_t */
  qp_a.rnr_retry     = 255; /* default Open MPI is 7, but I guess in our case should be max value of uint8_t */
  qp_a.sq_psn        = 0;
  qp_a.max_rd_atomic = 4; /* default value from Open MPI */

  retval = ibv_modify_qp(ib_module.connections[mpi_rank].q_pair, &qp_a,
                         (enum ibv_qp_attr_mask) (IBV_QP_STATE | IBV_QP_TIMEOUT |
                         IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC));
  if (0 != retval) {
    ng_error("Unable to modify QP of connection %d to state RTS", mpi_rank);
    return 1;
  }

  return 0;
}

int destroy_connection(int mpi_rank) {

  int       retval;

  retval = ibv_destroy_qp(ib_module.connections[mpi_rank].q_pair);
  if (0 != retval) {
    ng_error("Unable to destroy QP of connection %d in function destroy_connection", mpi_rank);
    return 1;
  }

  retval = ibv_destroy_cq(ib_module.connections[mpi_rank].send_cq);
  if (0 != retval) {
    ng_error("Unable to destroy send CQ of connection %d in function destroy_connection", mpi_rank);
    return 1;
  }

  retval = ibv_destroy_cq(ib_module.connections[mpi_rank].recv_cq);
  if (0 != retval) {
    ng_error("Unable to destroy recv CQ of connection %d in function destroy_connection", mpi_rank);
    return 1;
  }

  return 0;
}

/* This function modifies the QP state of the connection belonging to
 * the mpi rank from Send Queue Error to RTS again. Useful after an SQE state was entered */
int from_sqe_to_rts(int mpi_rank) {

  int                       retval;
  struct ibv_qp_attr        qp_a;

  /* Modify QP to RTS state */
  memset(&qp_a, 0, sizeof(struct ibv_qp_attr));
  qp_a.qp_state      = IBV_QPS_RTS;
  qp_a.timeout       = 10; /* default value from Open MPI */
  qp_a.retry_cnt     = 255; /* default Open MPI is 7, but I guess in our case should be max value of uint8_t */
  qp_a.rnr_retry     = 255; /* default Open MPI is 7, but I guess in our case should be max value of uint8_t */
  qp_a.sq_psn        = 0;
  qp_a.max_rd_atomic = 4; /* default value from Open MPI */

  retval = ibv_modify_qp(ib_module.connections[mpi_rank].q_pair, &qp_a,
                         (enum ibv_qp_attr_mask) (IBV_QP_STATE | IBV_QP_TIMEOUT |
                         IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC));
  if (0 != retval) {
    ng_error("Unable to modify QP of connection %d to state RTS in function from_err_to_rts (%s)", mpi_rank, strerror(errno));
    return 1;
  }

  return 0;
}

/* This function modifies the QP state of the connection belonging to
 * the mpi rank to RTS again. Useful after an error state was entered */
int from_err_to_rts(int mpi_rank) {

  int                       retval;
  struct ibv_qp_attr        qp_a;

  /* Modify QP to RTS state */
  memset(&qp_a, 0, sizeof(struct ibv_qp_attr));
  qp_a.qp_state      = IBV_QPS_RESET;

  retval = ibv_modify_qp(ib_module.connections[mpi_rank].q_pair, &qp_a,
                         (enum ibv_qp_attr_mask) (IBV_QP_STATE));
  if (0 != retval) {
    ng_error("Unable to modify QP of connection %d to state RESET in function from_err_to_rts (%s)", mpi_rank, strerror(errno));
    return 1;
  }

  retval = from_rst_to_rts(mpi_rank);
  if (0 != retval) {
    ng_error("Unable to get the QP to RTS again in function from_err_to_rts");
    return 1;
  }

  return 0;
}

/* This function modifies the QP state of the connection belonging to
 * the mpi rank to SQD and back to RTS again. Useful after strange errors
 * happen while trying to post receive requests. */
int from_rts_to_sqd_to_rts(int mpi_rank) {

  int                       retval;
  struct ibv_qp_attr        qp_a;

  /* Modify QP to SQD state */
  memset(&qp_a, 0, sizeof(struct ibv_qp_attr));
  qp_a.qp_state      = IBV_QPS_RTS;//IBV_QPS_SQD;

  retval = ibv_modify_qp(ib_module.connections[mpi_rank].q_pair, &qp_a,
                         (enum ibv_qp_attr_mask) (IBV_QP_STATE));
  if (0 != retval) {
    ng_error("Unable to modify QP of connection %d to state SQD in function from_rts_to_sqd_to_rts (%s)", mpi_rank, strerror(errno));
    return 1;
  }

#if 0
  /* Modify QP to RTS state */
  qp_a.qp_state      = IBV_QPS_RTS;

  retval = ibv_modify_qp(ib_module.connections[mpi_rank].q_pair, &qp_a,
                         (enum ibv_qp_attr_mask) (IBV_QP_STATE));
  if (0 != retval) {
    ng_error("Unable to modify QP of connection %d to state RTS in function from_rts_to_sqd_to_rts (%s)", mpi_rank, strerror(errno));
    return 1;
  }
#endif

  return 0;
}

/* This function determines whether the QP is in the error state or not */
enum ibv_qp_state is_qp_erroneous(struct ibv_qp *qp) {

  int                         retval;
  struct ibv_qp_attr          qp_a;
  struct ibv_qp_init_attr     qp_init;

  retval = ibv_query_qp(qp, &qp_a, IBV_QP_STATE, &qp_init);
  if (0 != retval) {
    ng_error("Unable to query QP in function is_qp_erroneous");
    return IBV_QPS_ERR;
  }

  return qp_a.qp_state;
}

static int ib_select(int count, int *clients, unsigned long timeout) {

  /* To implement this for send-recv semantics we need to send an additional flag
   * to the receiver. This is the only way to notice that the actual data can be received
   * immediatly. */
  ng_abort("The function select is currently not implemented\n");
  return 1;
}

int ib_test(NG_Request *req) {

  ng_abort("The function test is currently not implemented\n");
  return -1;
}

uint32_t get_lkey(void *addr) {

  unsigned int          index;
  int                   retval;
  unsigned long long    address;
  struct ib_mem_t       *ib_mem;

  address = (unsigned long long) addr;
  address &= ib_module.page_mask;

  retval = wn_hget((void *) &ib_mem, ib_module.mem_hash, (void *) address);
  if (!retval) {
    ng_info(NG_VNORM, "Search for memory address in hash table unsuccessfull: trying more addresses");

    index = 1000;
    do {
      /* get next smaller page address */
      address = address >> ib_module.page_bits;
      address--;
      address = address << ib_module.page_bits;
      retval = wn_hget((void *)&ib_mem, ib_module.mem_hash, (void *) address);
      index--;
    } while ((!retval) && (address > 0) && (index > 0));
  }

  if (!retval) {
    ng_error("Unable to find an appropriate entry for memory address %p in function get_lkey", addr);
    return 0;
  }

  return ib_mem->lkey;
}

static int ib_set_blocking(int do_block, int partnet) {

  ng_abort("The function ib_set_blocking is not implemented\n");
  return 1;
}

static int ib_getopt(int argc, char **argv, struct ng_options *global_opts) {

  char         c;
  char        *optchars = "T:M:H:C";
  extern char *optarg;
  extern int   optopt; 

  if (!global_opts->mpi) {
    ng_error("The InfiniBand module needs execution with MPI (global option -p)");
    return 1;
  }

  /* set default values */
  ib_module.tran_type       = IBV_QPT_RC;
  ib_module.tran_mode       = IBV_WR_SEND;
  ib_module.num_hca         = 0;
  ib_module.pre_connect_all = 1;

  while (0 <= (c = getopt_long(argc, argv, optchars, ib_options, NULL))) {
    switch (c) {
    case '?':   /* unrecognized or badly used option */
      if (!strchr(optchars, optopt))
        continue;   /* unrecognized */

      ng_error("Option %c requires an argument", optopt);
      return 1;
      break;

    case 'T':
      switch(optarg[0]) {
      case 'r':
        ib_module.tran_type = IBV_QPT_RC;
        break;
      case 'u':
        ib_module.tran_type = IBV_QPT_UC;
        ng_error("Transport type \"unreliable connection\" is currently not supported");
        return 1;
	break;
      case 'U':
        ib_module.tran_type = IBV_QPT_UD;
        ng_error("Transport type \"unreliable datagram\" is currently not supported");
        return 1;
        break;
      default:
        ng_info(NG_VLEV1, "Unrecognized transport type \"%s\". Using reliable connection", optarg[0]);
      }

    case 'M':
      switch(optarg[0]) {
      case 's':
        ib_module.tran_mode = IBV_WR_SEND;
        break;
      case 'w':
        ib_module.tran_mode = IBV_WR_RDMA_WRITE;
        ng_error("Transport mode \"RDMA write\" is currently not supported");
        return 1;
        break;
      case 'r':
        ib_module.tran_mode = IBV_WR_RDMA_READ;
        ng_error("Transport mode \"RDMA read\" is currently not supported");
        return 1;
        break;
      default:
        ng_info(NG_VLEV1, "Unrecognized transport mode \"%s\". Using send", optarg[0]);
      }

    case 'H':
      ib_module.num_hca = atoi(&optarg[0]);
      ng_info(NG_VLEV1, "Using HCA number %d", ib_module.num_hca);
      break;

    case 'C':
      switch(optarg[0]) {
      case 'i':
        ib_module.pre_connect_all = 1;
        break;
      case 'd':
        ib_module.pre_connect_all = 0;
        ng_error("Setup IB connections on demand is currently not supported");
        break;
      default:
        ng_info(NG_VLEV1, "Unrecognized argument for option \"-C\". All connections are established during initialization");
      }
    }
  }

  return 0;
}

static void ib_usage(void) {

  int   i;

  for(i = 0; ib_options[i].name != NULL; i++) {
    ng_longoption_usage(
      ib_options[i].val,
      ib_options[i].name,
      ib_opt_info[i].desc,
      ib_opt_info[i].param
    );
  }
}

static void ib_writemanpage(void) {

  int i;

  for(i = 0; ib_options[i].name != NULL; i++) {
    ng_manpage_module(
      ib_options[i].val,
      ib_options[i].name,
      ib_opt_info[i].desc,
      ib_opt_info[i].param
    );
  }
}

static void ib_shutdown(struct ng_options *global_opts) {

  int   i;

  /* deregister the memory */
  wn_hact(ib_module.mem_hash, ib_dereg_memory);

  free(ib_module.work_req.send_wr);
  free(ib_module.work_req.recv_wr);
  free(ib_module.work_req.sge);
  free(ib_module.work_req.wc);

  /* destroy QPs and CQs */
  for (i = 0; i < ib_module.world_size; i++)
    destroy_connection(i);

  ibv_dealloc_pd(ib_module.pd_handle);
  ibv_close_device(ib_module.hca_handle);
}

char * ib_get_error(int iberror) {

  switch (iberror) {
    case IBV_WC_SUCCESS:
      return "IBV_WC_SUCCESS";
    case IBV_WC_LOC_LEN_ERR:
      return "IBV_WC_LOC_LEN_ERR";
    case IBV_WC_LOC_QP_OP_ERR:
      return "IBV_WC_LOC_QP_OP_ERR";
    case IBV_WC_LOC_EEC_OP_ERR:
      return "IBV_WC_LOC_EEC_OP_ERR";
    case IBV_WC_LOC_PROT_ERR:
      return "IBV_WC_LOC_PROT_ERR";
    case IBV_WC_WR_FLUSH_ERR:
      return "IBV_WC_WR_FLUSH_ERR";
    case IBV_WC_MW_BIND_ERR:
      return "IBV_WC_MW_BIND_ERR";
    case IBV_WC_BAD_RESP_ERR:
      return "IBV_WC_BAD_RESP_ERR";
    case IBV_WC_LOC_ACCESS_ERR:
      return "IBV_WC_LOC_ACCESS_ERR";
    case IBV_WC_REM_INV_REQ_ERR:
      return "IBV_WC_REM_INV_REQ_ERR";
    case IBV_WC_REM_ACCESS_ERR:
      return "IBV_WC_REM_ACCESS_ERR";
    case IBV_WC_REM_OP_ERR:
      return "IBV_WC_REM_OP_ERR";
    case IBV_WC_RETRY_EXC_ERR:
      return "IBV_WC_RETRY_EXC_ERR";
    case IBV_WC_RNR_RETRY_EXC_ERR:
      return "IBV_WC_RNR_RETRY_EXC_ERR";
    case IBV_WC_LOC_RDD_VIOL_ERR:
      return "IBV_WC_LOC_RDD_VIOL_ERR";
    case IBV_WC_REM_INV_RD_REQ_ERR:
      return "IBV_WC_REM_INV_RD_REQ_ERR";
    case IBV_WC_REM_ABORT_ERR:
      return "IBV_WC_REM_ABORT_ERR";
    case IBV_WC_INV_EECN_ERR:
      return "IBV_WC_INV_EECN_ERR";
    case IBV_WC_INV_EEC_STATE_ERR:
      return "IBV_WC_INV_EEC_STATE_ERR";
    case IBV_WC_FATAL_ERR:
      return "IBV_WC_FATAL_ERR";
    case IBV_WC_RESP_TIMEOUT_ERR:
      return "IBV_WC_RESP_TIMEOUT_ERR";
    case IBV_WC_GENERAL_ERR:
      return "IBV_WC_GENERAL_ERR";
    default:
      return "unknown";
  }

  return "unknown";
}

/* this function is passed to the hash table to deregister all memory areas
 * during shutdown */
void ib_dereg_memory(void *data, void *key) {

  struct ib_mem_t     *ib_mem;

  ib_mem = (struct ib_mem_t *) data;
  ibv_dereg_mr(ib_mem->mr);
}

#ifdef NG_HAS_GETPAGESIZE
/* if the function "int getpagesize(void)" exists we can use it to determine the
 * size of a page
 */
int ib_getpagesize(void) {
  return getpagesize();
}
#elif defined NG_STATIC_PAGESIZE
/* if the OS does not provide this non-POSIX function use a static assignment
 * instead; may come from a configure switch
 */
int ib_getpagesize(void) {
  return NG_STATIC_PAGESIZE;
}
#else
/* last possibility: assume 4 KByte pages */
int ib_getpagesize(void) {
  return 4096;
}
#endif /* NG_HAS_GETPAGESIZE */

#else
/*******************************************************************
 *    register stub 
 *******************************************************************/
int register_ib(void) {
  return 0;
}

#endif /* NG_OPENIB */

