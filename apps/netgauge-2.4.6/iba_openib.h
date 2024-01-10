/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */


#include <sysfs/libsysfs.h>
#include <infiniband/verbs.h>

/* -------------------------------------------------------------------------------- */

typedef int  iba_status_t;

#define IBA_OK       0
#define IBA_ERROR    1
#define IBA_EAGAIN   2
#define IBA_CQ_EMPTY 3

#define RES1(x) ((x) != NULL ? IBA_OK : IBA_ERROR)
#define RES2(x) ((x) == 0 ? IBA_OK : IBA_ERROR)

/* -------------------------------------------------------------------------------- */

typedef unsigned char  iba_bool_t;

/* -------------------------------------------------------------------------------- */

typedef uint8_t  iba_uint8_t;

/* -------------------------------------------------------------------------------- */

typedef uint16_t  iba_uint16_t;

/* -------------------------------------------------------------------------------- */

typedef uint32_t  iba_uint32_t;

/* -------------------------------------------------------------------------------- */

typedef uint64_t  iba_uint64_t;

/* -------------------------------------------------------------------------------- */

typedef struct ibv_device*  iba_device_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef struct ibv_context*  iba_hca_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef struct ibv_cq*  iba_cq_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef struct ibv_qp*  iba_qp_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef struct ibv_srq*  iba_srq_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef struct ibv_pd*  iba_pd_hndl_t;

/* -------------------------------------------------------------------------------- */

#define IBA_INVAL_HNDL  0

/* -------------------------------------------------------------------------------- */

typedef struct ibv_ah*  iba_av_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef uint16_t  iba_pkey_t;

/* -------------------------------------------------------------------------------- */

typedef int  iba_sig_type_t;

#define IBA_SIGNAL_ALL_WR  1
#define IBA_SIGNAL_REQ_WR  0

/* -------------------------------------------------------------------------------- */

typedef enum ibv_qp_type  iba_qp_type_t;

#define IBA_QPT_RC   IBV_QPT_RC
#define IBA_QPT_UC   IBV_QPT_UC
#define IBA_QPT_UD   IBV_QPT_UD

/* -------------------------------------------------------------------------------- */

typedef struct ibv_qp_cap  iba_qp_cap_t;

/*
----- INTERN -----

struct ibv_qp_cap {
  uint32_t  max_send_wr;
  uint32_t  max_recv_wr;
  uint32_t  max_send_sge;
  uint32_t  max_recv_sge;
  uint32_t  max_inline_data;
};

----- EXTERN -----

typedef struct {
  iba_uint32_t  max_send_wr;
  iba_uint32_t  max_recv_wr;
  iba_uint32_t  max_send_sge;
  iba_uint32_t  max_recv_sge;
  iba_uint32_t  max_inline_data;
} iba_qp_cap_t;

------------------
*/

#define SET_iba_qp_cap_t__max_send_wr(struct_name, value) ((struct_name).max_send_wr = (uint32_t)(value))
#define SET_iba_qp_cap_t__max_recv_wr(struct_name, value) ((struct_name).max_recv_wr = (uint32_t)(value))
#define SET_iba_qp_cap_t__max_send_sge(struct_name, value) ((struct_name).max_send_sge = (uint32_t)(value))
#define SET_iba_qp_cap_t__max_recv_sge(struct_name, value) ((struct_name).max_recv_sge = (uint32_t)(value))
#define SET_iba_qp_cap_t__max_inline_data(struct_name, value) ((struct_name).max_inline_data = (uint32_t)(value))

#define GET_iba_qp_cap_t__max_send_wr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_send_wr))
#define GET_iba_qp_cap_t__max_recv_wr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_recv_wr))
#define GET_iba_qp_cap_t__max_send_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).max_send_sge))
#define GET_iba_qp_cap_t__max_recv_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).max_recv_sge))
#define GET_iba_qp_cap_t__max_inline_data(struct_name, dummy) ((iba_uint32_t)((struct_name).max_inline_data))

/* -------------------------------------------------------------------------------- */

typedef struct {                       /* read only */
  uint32_t           qp_num;
  struct ibv_qp_cap  cap;
} iba_qp_prop_t;

/*
----- INTERN -----

----- EXTERN -----

typedef struct {
  iba_uint32_t  qp_num;
  iba_qp_cap_t  cap;
} iba_qp_prop_t;

------------------
*/

#define GET_iba_qp_prop_t__qp_num(struct_name, dummy) ((iba_uint32_t)((struct_name).qp_num))
#define GET_iba_qp_prop_t__cap(struct_name, field_name, ...) (GET_iba_qp_cap_t__##field_name((struct_name).cap, __VA_ARGS__))
    
/* -------------------------------------------------------------------------------- */

typedef struct {
  struct ibv_qp_init_attr   openib_init_attr;
  struct ibv_qp_cap         cap;
  struct ibv_pd            *pd;
} iba_qp_init_attr_t;

/*
----- INTERN -----

struct ibv_qp_init_attr {
  void               *qp_context;
  struct ibv_cq      *send_cq;
  struct ibv_cq      *recv_cq;
  struct ibv_srq     *srq;
  struct ibv_qp_cap   cap;
  enum ibv_qp_type    qp_type;
  int                 sq_sig_all;
};

----- EXTERN -----

typedef struct {
  iba_qp_type_t   qp_type;
  iba_cq_hndl_t   sq_cq_hndl;
  iba_cq_hndl_t   rq_cq_hndl;
  iba_srq_hndl_t  srq_hndl;
  iba_pd_hndl_t   pd_hndl;
  iba_sig_type_t  sq_sig_type;
  iba_qp_cap_t    cap;
} iba_qp_init_attr_t;

------------------
*/

#define SET_iba_qp_init_attr_t__qp_type(struct_name, value) ((struct_name).openib_init_attr.qp_type = (enum ibv_qp_type)(value))
#define SET_iba_qp_init_attr_t__sq_cq_hndl(struct_name, value) ((struct_name).openib_init_attr.send_cq = (struct ibv_cq*)(value))
#define SET_iba_qp_init_attr_t__rq_cq_hndl(struct_name, value) ((struct_name).openib_init_attr.recv_cq = (struct ibv_cq*)(value))
#define SET_iba_qp_init_attr_t__srq_hndl(struct_name, value) ((struct_name).openib_init_attr.srq = (struct ibv_srq*)(value))
#define SET_iba_qp_init_attr_t__pd_hndl(struct_name, value) ((struct_name).pd = (struct ibv_pd*)(value))
#define SET_iba_qp_init_attr_t__sq_sig_type(struct_name, value) ((struct_name).openib_init_attr.sq_sig_all = (int)(value))
#define SET_iba_qp_init_attr_t__cap(struct_name, field_name, ...) (SET_iba_qp_cap_t__##field_name((struct_name).cap, __VA_ARGS__))

#define GET_iba_qp_init_attr_t__qp_type(struct_name, dummy) ((iba_qp_type_t)((struct_name).openib_init_attr.qp_type))
#define GET_iba_qp_init_attr_t__sq_cq_hndl(struct_name, dummy) ((iba_cq_hndl_t)((struct_name).openib_init_attr.send_cq))
#define GET_iba_qp_init_attr_t__rq_cq_hndl(struct_name, dummy) ((iba_cq_hndl_t)((struct_name).openib_init_attr.recv_cq))
#define GET_iba_qp_init_attr_t__srq_hndl(struct_name, dummy) ((iba_srq_hndl_t)((struct_name).openib_init_attr.srq))
#define GET_iba_qp_init_attr_t__pd_hndl(struct_name, dummy) ((iba_pd_hndl_t)((struct_name).pd))
#define GET_iba_qp_init_attr_t__sq_sig_type(struct_name, dummy) ((iba_sig_type_t)((struct_name).openib_init_attr.sq_sig_all))
#define GET_iba_qp_init_attr_t__cap(struct_name, field_name, ...) (GET_iba_qp_cap_t__##field_name((struct_name).cap, __VA_ARGS__))

/* -------------------------------------------------------------------------------- */

typedef struct {                  /* read only */
  uint32_t  vendor_id;
	uint32_t  vendor_part_id;
	uint32_t  hw_ver;
	uint64_t  fw_ver;
} iba_hca_vendor_t;

/*
----- INTERN -----

part of ibv_device_attr:

struct ibv_device_attr {
	uint64_t             fw_ver;
  ...
	uint32_t             vendor_id;
	uint32_t             vendor_part_id;
	uint32_t             hw_ver;
  ...
}

----- EXTERN -----

typedef struct {
  iba_uint32_t  vendor_id;
  iba_uint32_t  vendor_part_id;
  iba_uint32_t  hw_ver;
  iba_uint64_t  fw_ver;
} iba_hca_vendor_t;

------------------
*/

#define GET_iba_hca_vendor_t__vendor_id(struct_name, dummy) ((iba_uint32_t)((struct_name).vendor_id))
#define GET_iba_hca_vendor_t__vendor_part_id(struct_name, dummy) ((iba_uint32_t)((struct_name).vendor_part_id))
#define GET_iba_hca_vendor_t__hw_ver(struct_name, dummy) ((iba_uint32_t)((struct_name).hw_ver))
#define GET_iba_hca_vendor_t__fw_ver(struct_name, dummy) ((iba_uint64_t)((struct_name).fw_ver))

/* -------------------------------------------------------------------------------- */

typedef int  iba_hca_cap_flags_t; /* read only */

/*
----- INTERN -----

enum ibv_device_cap_flags {
	IBV_DEVICE_RESIZE_MAX_WR      = 1,
	IBV_DEVICE_BAD_PKEY_CNTR      = 1 <<  1,
	IBV_DEVICE_BAD_QKEY_CNTR      = 1 <<  2,
	IBV_DEVICE_RAW_MULTI          = 1 <<  3,
	IBV_DEVICE_AUTO_PATH_MIG      = 1 <<  4,
	IBV_DEVICE_CHANGE_PHY_PORT    = 1 <<  5,
	IBV_DEVICE_UD_AV_PORT_ENFORCE = 1 <<  6,
	IBV_DEVICE_CURR_QP_STATE_MOD  = 1 <<  7,
	IBV_DEVICE_SHUTDOWN_PORT      = 1 <<  8,
	IBV_DEVICE_INIT_TYPE          = 1 <<  9,
	IBV_DEVICE_PORT_ACTIVE_EVENT  = 1 << 10,
	IBV_DEVICE_SYS_IMAGE_GUID     = 1 << 11,
	IBV_DEVICE_RC_RNR_NAK_GEN     = 1 << 12,
	IBV_DEVICE_SRQ_RESIZE         = 1 << 13,
	IBV_DEVICE_N_NOTIFY_CQ        = 1 << 14,
};

----- EXTERN -----

typedef struct {
  iba_bool_t  resize_max_wr;
  iba_bool_t  bad_pkey_counter;
  iba_bool_t  bad_qkey_counter;
  iba_bool_t  raw_multicast;
  iba_bool_t  auto_path_mig;
  iba_bool_t  change_phy_port;
  iba_bool_t  ud_av_port_enforce;
  iba_bool_t  curr_qp_state_mod;
  iba_bool_t  shutdown_port;
  iba_bool_t  init_type;
  iba_bool_t  port_active_event;
  iba_bool_t  sys_image_guid;
  iba_bool_t  rc_rnr_nak_gen;
  iba_bool_t  srq_resize;
} iba_hca_cap_flags_t;

------------------
*/

#define GET_iba_hca_cap_flags_t__resize_max_wr(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_RESIZE_MAX_WR) != 0))
#define GET_iba_hca_cap_flags_t__bad_pkey_counter(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_BAD_PKEY_CNTR) != 0))
#define GET_iba_hca_cap_flags_t__bad_qkey_counter(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_BAD_QKEY_CNTR) != 0))
#define GET_iba_hca_cap_flags_t__raw_multicast(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_RAW_MULTI) != 0))
#define GET_iba_hca_cap_flags_t__auto_path_mig(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_AUTO_PATH_MIG) != 0))
#define GET_iba_hca_cap_flags_t__change_phy_port(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_CHANGE_PHY_PORT) != 0))
#define GET_iba_hca_cap_flags_t__ud_av_port_enforce(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_UD_AV_PORT_ENFORCE) != 0))
#define GET_iba_hca_cap_flags_t__curr_qp_state_mod(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_CURR_QP_STATE_MOD) != 0))
#define GET_iba_hca_cap_flags_t__shutdown_port(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_SHUTDOWN_PORT) != 0))
#define GET_iba_hca_cap_flags_t__init_type(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_INIT_TYPE) != 0))
#define GET_iba_hca_cap_flags_t__port_active_event(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_PORT_ACTIVE_EVENT) != 0))
#define GET_iba_hca_cap_flags_t__sys_image_guid(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_SYS_IMAGE_GUID) != 0))
#define GET_iba_hca_cap_flags_t__rc_rnr_nak_gen(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_RC_RNR_NAK_GEN) != 0))
#define GET_iba_hca_cap_flags_t__srq_resize(struct_name, dummy) ((iba_bool_t)(((struct_name) & IBV_DEVICE_SRQ_RESIZE) != 0))

/* -------------------------------------------------------------------------------- */

typedef enum ibv_atomic_cap  iba_atomic_cap_t;

#define IBA_ATOMIC_CAP_NONE  IBV_ATOMIC_NONE
#define IBA_ATOMIC_CAP_HCA   IBV_ATOMIC_HCA
#define IBA_ATOMIC_CAP_GLOB  IBV_ATOMIC_GLOB

/* -------------------------------------------------------------------------------- */

typedef struct ibv_device_attr  iba_hca_cap_t;

/*
----- INTERN -----

struct ibv_device_attr {
	uint64_t             fw_ver;
	uint64_t             node_guid;
	uint64_t             sys_image_guid;
	uint64_t             max_mr_size;
	uint64_t             page_size_cap;
	uint32_t             vendor_id;
	uint32_t             vendor_part_id;
	uint32_t             hw_ver;
	int                  max_qp;
	int                  max_qp_wr;
	int                  device_cap_flags;
	int                  max_sge;
	int                  max_sge_rd;
	int                  max_cq;
	int                  max_cqe;
	int                  max_mr;
	int                  max_pd;
	int                  max_qp_rd_atom;
	int                  max_ee_rd_atom;
	int                  max_res_rd_atom;
	int                  max_qp_init_rd_atom;
	int                  max_ee_init_rd_atom;
	enum ibv_atomic_cap  atomic_cap;
	int                  max_ee;
	int                  max_rdd;
	int                  max_mw;
	int                  max_raw_ipv6_qp;
	int                  max_raw_ethy_qp;
	int                  max_mcast_grp;
	int                  max_mcast_qp_attach;
	int                  max_total_mcast_qp_attach;
	int                  max_ah;
	int                  max_fmr;
	int                  max_map_per_fmr;
	int                  max_srq;
	int                  max_srq_wr;
	int                  max_srq_sge;
	uint16_t             max_pkeys;
	uint8_t              local_ca_ack_delay;
	uint8_t              phys_port_cnt;
};

----- EXTERN -----

typedef struct {
  iba_hca_vendor_t     vendor;
  iba_uint64_t         node_guid;
  iba_uint64_t         sys_image_guid;
  iba_uint8_t          phys_port_num;
  iba_uint64_t         max_mr_size;
  iba_uint64_t         page_size_cap;
  iba_hca_cap_flags_t  flags;
  iba_uint32_t         max_qp;
  iba_uint32_t         max_qp_wr;
  iba_uint32_t         max_cq;
  iba_uint32_t         max_cqe;
  iba_uint32_t         max_sge;
  iba_uint32_t	       max_sge_rd;
  iba_uint32_t         max_mr;
  iba_uint32_t         max_pd;
  iba_uint16_t         max_pkeys;
  iba_atomic_cap_t     atomic_cap;
  iba_uint32_t         max_qp_rd_atom;
  iba_uint32_t         max_ee_rd_atom;
  iba_uint32_t         max_res_rd_atom;
  iba_uint32_t         max_qp_init_rd_atom;
  iba_uint32_t         max_ee_init_rd_atom;
  iba_uint32_t         max_ee;
  iba_uint32_t         max_rdd;
  iba_uint32_t         max_mw;
  iba_uint32_t         max_raw_ipv6_qp;
  iba_uint32_t         max_raw_ethy_qp;
  iba_uint32_t         max_mcast_grp;
  iba_uint32_t         max_mcast_qp_attach;
  iba_uint32_t         max_total_mcast_qp_attach;
  iba_uint32_t         max_ah;
  iba_uint32_t         max_fmr;
  iba_uint32_t         max_map_per_fmr;
  iba_uint32_t         max_srq;
  iba_uint32_t         max_srq_wr;
  iba_uint32_t         max_srq_sge;
  iba_uint8_t          local_ca_ack_delay;
} iba_hca_cap_t;

------------------
*/

#define GET_iba_hca_cap_t__vendor(struct_name, field_name, ...) (GET_iba_hca_vendor_t__##field_name((struct_name), __VA_ARGS__))
#define GET_iba_hca_cap_t__node_guid(struct_name, dummy) ((iba_uint64_t)((struct_name).node_guid))
#define GET_iba_hca_cap_t__sys_image_guid(struct_name, dummy) ((iba_uint64_t)((struct_name).sys_image_guid))
#define GET_iba_hca_cap_t__phys_port_num(struct_name, dummy) ((iba_uint8_t)((struct_name).phys_port_cnt))
#define GET_iba_hca_cap_t__max_mr_size(struct_name, dummy) ((iba_uint64_t)((struct_name).max_mr_size))
#define GET_iba_hca_cap_t__page_size_cap(struct_name, dummy) ((iba_uint64_t)((struct_name).page_size_cap))
#define GET_iba_hca_cap_t__flags(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).device_cap_flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__max_qp(struct_name, dummy) ((iba_uint32_t)((struct_name).max_qp))
#define GET_iba_hca_cap_t__max_qp_wr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_qp_wr))
#define GET_iba_hca_cap_t__max_cq(struct_name, dummy) ((iba_uint32_t)((struct_name).max_cq))
#define GET_iba_hca_cap_t__max_cqe(struct_name, dummy) ((iba_uint32_t)((struct_name).max_cqe))
#define GET_iba_hca_cap_t__max_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).max_sge))
#define GET_iba_hca_cap_t__max_sge_rd(struct_name, dummy) ((iba_uint32_t)((struct_name).max_sge_rd))
#define GET_iba_hca_cap_t__max_mr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_mr))
#define GET_iba_hca_cap_t__max_pd(struct_name, dummy) ((iba_uint32_t)((struct_name).max_pd))
#define GET_iba_hca_cap_t__max_pkeys(struct_name, dummy) ((iba_uint16_t)((struct_name).max_pkeys))
#define GET_iba_hca_cap_t__atomic_cap(struct_name, dummy) ((iba_atomic_cap_t)((struct_name).atomic_cap))
#define GET_iba_hca_cap_t__max_qp_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).max_qp_rd_atom))
#define GET_iba_hca_cap_t__max_ee_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).max_ee_rd_atom))
#define GET_iba_hca_cap_t__max_res_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).max_res_rd_atom))
#define GET_iba_hca_cap_t__max_qp_init_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).max_qp_init_rd_atom))
#define GET_iba_hca_cap_t__max_ee_init_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).max_ee_init_rd_atom))
#define GET_iba_hca_cap_t__max_ee(struct_name, dummy) ((iba_uint32_t)((struct_name).max_ee))
#define GET_iba_hca_cap_t__max_rdd(struct_name, dummy) ((iba_uint32_t)((struct_name).max_rdd))
#define GET_iba_hca_cap_t__max_mw(struct_name, dummy) ((iba_uint32_t)((struct_name).max_mw))
#define GET_iba_hca_cap_t__max_raw_ipv6_qp(struct_name, dummy) ((iba_uint32_t)((struct_name).max_raw_ipv6_qp))
#define GET_iba_hca_cap_t__max_raw_ethy_qp(struct_name, dummy) ((iba_uint32_t)((struct_name).max_raw_ethy_qp))
#define GET_iba_hca_cap_t__max_mcast_grp(struct_name, dummy) ((iba_uint32_t)((struct_name).max_mcast_grp))
#define GET_iba_hca_cap_t__max_mcast_qp_attach(struct_name, dummy) ((iba_uint32_t)((struct_name).max_mcast_qp_attach))
#define GET_iba_hca_cap_t__max_total_mcast_qp_attach(struct_name, dummy) ((iba_uint32_t)((struct_name).max_total_mcast_qp_attach))
#define GET_iba_hca_cap_t__max_ah(struct_name, dummy) ((iba_uint32_t)((struct_name).max_ah))
#define GET_iba_hca_cap_t__max_fmr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_fmr))
#define GET_iba_hca_cap_t__max_map_per_fmr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_map_per_fmr))
#define GET_iba_hca_cap_t__max_srq(struct_name, dummy) ((iba_uint32_t)((struct_name).max_srq))
#define GET_iba_hca_cap_t__max_srq_wr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_srq_wr))
#define GET_iba_hca_cap_t__max_srq_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).max_srq_sge))
#define GET_iba_hca_cap_t__local_ca_ack_delay(struct_name, dummy) ((iba_uint8_t)((struct_name).local_ca_ack_delay))

/* -------------------------------------------------------------------------------- */

typedef enum ibv_port_state  iba_port_state_t;

#define IBA_PORT_NOP           IBV_PORT_NOP
#define IBA_PORT_DOWN          IBV_PORT_DOWN
#define IBA_PORT_INIT          IBV_PORT_INIT
#define IBA_PORT_ARMED         IBV_PORT_ARMED
#define IBA_PORT_ACTIVE        IBV_PORT_ACTIVE
#define IBA_PORT_ACTIVE_DEFER  IBV_PORT_ACTIVE_DEFER

/* -------------------------------------------------------------------------------- */

typedef uint32_t  iba_port_cap_flags_t;   /* read only */

/*
----- INTERN -----
*/
enum ib_port_cap_flags {
	IB_PORT_SM			        = 1 <<  1,
	IB_PORT_NOTICE_SUP			= 1 <<  2,
	IB_PORT_TRAP_SUP			= 1 <<  3,
	IB_PORT_OPT_IPD_SUP                     = 1 <<  4,
	IB_PORT_AUTO_MIGR_SUP			= 1 <<  5,
	IB_PORT_SL_MAP_SUP			= 1 <<  6,
	IB_PORT_MKEY_NVRAM			= 1 <<  7,
	IB_PORT_PKEY_NVRAM			= 1 <<  8,
	IB_PORT_LED_INFO_SUP			= 1 <<  9,
	IB_PORT_SM_DISABLED			= 1 << 10,
	IB_PORT_SYS_IMAGE_GUID_SUP		= 1 << 11,
	IB_PORT_PKEY_SW_EXT_PORT_TRAP_SUP	= 1 << 12,
	IB_PORT_CM_SUP				= 1 << 16,
	IB_PORT_SNMP_TUNNEL_SUP			= 1 << 17,
	IB_PORT_REINIT_SUP			= 1 << 18,
	IB_PORT_DEVICE_MGMT_SUP			= 1 << 19,
	IB_PORT_VENDOR_CLASS_SUP		= 1 << 20,
	IB_PORT_DR_NOTICE_SUP			= 1 << 21,
	IB_PORT_CAP_MASK_NOTICE_SUP		= 1 << 22,
	IB_PORT_BOOT_MGMT_SUP			= 1 << 23,
	IB_PORT_LINK_LATENCY_SUP		= 1 << 24,
	IB_PORT_CLIENT_REG_SUP			= 1 << 25
};
/*
----- EXTERN -----

typedef struct {
  iba_bool_t  sm;
  iba_bool_t  notice_sup;
  iba_bool_t  trap_sup;
  iba_bool_t  auto_migr_sup;
  iba_bool_t  sl_map_sup;
  iba_bool_t  mkey_nvram;
  iba_bool_t  pkey_nvram;
  iba_bool_t  led_info_sup;
  iba_bool_t  sm_disabled;
  iba_bool_t  sys_image_guid_sup;
  iba_bool_t  pkey_sw_ext_port_trap_sup;
  iba_bool_t  conn_mgmt_sup;
  iba_bool_t  snmp_tunnel_sup;
  iba_bool_t  reinit_sup;
  iba_bool_t  device_mgmt_sup;
  iba_bool_t  vendor_class_sup;
  iba_bool_t  dr_notice_sup;
  iba_bool_t  cap_mask_notice_sup;
  iba_bool_t  boot_mgmt_sup;
} iba_port_cap_flags_t;

------------------
*/

#define GET_iba_port_cap_flags_t__sm(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_SM) != 0))
#define GET_iba_port_cap_flags_t__notice_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_NOTICE_SUP) != 0))
#define GET_iba_port_cap_flags_t__trap_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_TRAP_SUP) != 0))
#define GET_iba_port_cap_flags_t__auto_migr_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_AUTO_MIGR_SUP) != 0))
#define GET_iba_port_cap_flags_t__sl_map_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_SL_MAP_SUP) != 0))
#define GET_iba_port_cap_flags_t__mkey_nvram(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_MKEY_NVRAM) != 0))
#define GET_iba_port_cap_flags_t__pkey_nvram(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_PKEY_NVRAM) != 0))
#define GET_iba_port_cap_flags_t__led_info_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_LED_INFO_SUP) != 0))
#define GET_iba_port_cap_flags_t__sm_disabled(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_SM_DISABLED) != 0))
#define GET_iba_port_cap_flags_t__sys_image_guid_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_SYS_IMAGE_GUID_SUP) != 0))
#define GET_iba_port_cap_flags_t__pkey_sw_ext_port_trap_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_PKEY_SW_EXT_PORT_TRAP_SUP) != 0))
#define GET_iba_port_cap_flags_t__conn_mgmt_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_CM_SUP) != 0))
#define GET_iba_port_cap_flags_t__snmp_tunnel_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_SNMP_TUNNEL_SUP) != 0))
#define GET_iba_port_cap_flags_t__reinit_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_REINIT_SUP) != 0))
#define GET_iba_port_cap_flags_t__device_mgmt_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_DEVICE_MGMT_SUP) != 0))
#define GET_iba_port_cap_flags_t__vendor_class_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_VENDOR_CLASS_SUP) != 0))
#define GET_iba_port_cap_flags_t__dr_notice_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_DR_NOTICE_SUP) != 0))
#define GET_iba_port_cap_flags_t__cap_mask_notice_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_CAP_MASK_NOTICE_SUP) != 0))
#define GET_iba_port_cap_flags_t__boot_mgmt_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_PORT_BOOT_MGMT_SUP) != 0))

/* -------------------------------------------------------------------------------- */

typedef enum ibv_mtu  iba_mtu_t;

#define IBA_MTU_256   IBV_MTU_256
#define IBA_MTU_512   IBV_MTU_512
#define IBA_MTU_1024  IBV_MTU_1024
#define IBA_MTU_2048  IBV_MTU_2048
#define IBA_MTU_4096  IBV_MTU_4096

/* -------------------------------------------------------------------------------- */

typedef struct ibv_port_attr  iba_hca_port_t;

/*
----- INTERN -----

struct ibv_port_attr {
	enum ibv_port_state	 state;
	enum ibv_mtu		     max_mtu;
	enum ibv_mtu		     active_mtu;
	int			             gid_tbl_len;
	uint32_t             port_cap_flags;
	uint32_t             max_msg_sz;
	uint32_t             bad_pkey_cntr;
	uint32_t             qkey_viol_cntr;
	uint16_t             pkey_tbl_len;
	uint16_t             lid;
	uint16_t             sm_lid;
	uint8_t              lmc;
	uint8_t              max_vl_num;
	uint8_t              sm_sl;
	uint8_t              subnet_timeout;
	uint8_t              init_type_reply;
	uint8_t              active_width;
	uint8_t              active_speed;
	uint8_t              phys_state;
};

----- EXTERN -----

typedef struct {
  iba_port_state_t      state;
  iba_port_cap_flags_t  flags;
  iba_mtu_t             max_mtu;
  iba_uint32_t          max_msg_size;
  iba_uint16_t          lid;
  iba_uint8_t           lmc;
  iba_uint8_t           max_vl_num;
  iba_uint32_t          bad_pkey_counter;
  iba_uint32_t          qkey_viol_counter;
  iba_uint32_t          gid_tbl_len;
  iba_uint16_t          pkey_tbl_len;
  iba_uint16_t          sm_lid;
  iba_uint8_t           sm_sl;
  iba_uint8_t           subnet_timeout;
  iba_uint8_t           init_type_reply;
} iba_hca_port_t;

------------------
*/

#define GET_iba_hca_port_t__state(struct_name, dummy) ((iba_port_state_t)((struct_name).state))
#define GET_iba_hca_port_t__flags(struct_name, field_name, ...) (GET_iba_port_cap_flags_t__##field_name((struct_name).port_cap_flags, __VA_ARGS__))
#define GET_iba_hca_port_t__max_mtu(struct_name, dummy) ((iba_mtu_t)((struct_name).max_mtu))
#define GET_iba_hca_port_t__max_msg_size(struct_name, dummy) ((iba_uint32_t)((struct_name).max_msg_sz))
#define GET_iba_hca_port_t__lid(struct_name, dummy) ((iba_uint16_t)((struct_name).lid))
#define GET_iba_hca_port_t__lmc(struct_name, dummy) ((iba_uint8_t)((struct_name).lmc))
#define GET_iba_hca_port_t__max_vl_num(struct_name, dummy) ((iba_uint8_t)((struct_name).max_vl_num))
#define GET_iba_hca_port_t__bad_pkey_counter(struct_name, dummy) ((iba_uint32_t)((struct_name).bad_pkey_cntr))
#define GET_iba_hca_port_t__qkey_viol_counter(struct_name, dummy) ((iba_uint32_t)((struct_name).qkey_viol_cntr))
#define GET_iba_hca_port_t__gid_tbl_len(struct_name, dummy) ((iba_uint32_t)((struct_name).gid_tbl_len))
#define GET_iba_hca_port_t__pkey_tbl_len(struct_name, dummy) ((iba_uint16_t)((struct_name).pkey_tbl_len))
#define GET_iba_hca_port_t__sm_lid(struct_name, dummy) ((iba_uint16_t)((struct_name).sm_lid))
#define GET_iba_hca_port_t__sm_sl(struct_name, dummy) ((iba_uint8_t)((struct_name).sm_sl))
#define GET_iba_hca_port_t__subnet_timeout(struct_name, dummy) ((iba_uint8_t)((struct_name).subnet_timeout))
#define GET_iba_hca_port_t__init_type_reply(struct_name, dummy) ((iba_uint8_t)((struct_name).init_type_reply))

/* -------------------------------------------------------------------------------- */

typedef enum ibv_qp_state  iba_qp_state_t;

#define IBA_QPS_RESET  IBV_QPS_RESET
#define IBA_QPS_INIT   IBV_QPS_INIT
#define IBA_QPS_RTR    IBV_QPS_RTR
#define IBA_QPS_RTS    IBV_QPS_RTS
#define IBA_QPS_SQD    IBV_QPS_SQD
#define IBA_QPS_SQE    IBV_QPS_SQE
#define IBA_QPS_ERR    IBV_QPS_ERR

/* -------------------------------------------------------------------------------- */

typedef enum ibv_mig_state  iba_mig_state_t;

#define IBA_MIG_MIGRATED  IBV_MIG_MIGRATED
#define IBA_MIG_REARM     IBV_MIG_REARM
#define IBA_MIG_ARMED     IBV_MIG_ARMED

/* -------------------------------------------------------------------------------- */

typedef int  iba_rdma_atom_acl_t;

#define IBA_ENABLE_REMOTE_WRITE   IBV_ACCESS_REMOTE_WRITE
#define IBA_ENABLE_REMOTE_READ    IBV_ACCESS_REMOTE_READ
#define IBA_ENABLE_REMOTE_ATOMIC  IBV_ACCESS_REMOTE_ATOMIC

/* -------------------------------------------------------------------------------- */

typedef union ibv_gid  iba_gid_t;

/*
----- INTERN -----

union ibv_gid {
	uint8_t			raw[16];
	struct {
		uint64_t	subnet_prefix;
		uint64_t	interface_id;
	} global;
};

----- EXTERN -----

typedef struct {
  iba_uint64_t	subnet_prefix;
  iba_uint64_t	interface_id;
} iba_gid_t;

------------------
*/

#define SET_iba_gid_t__subnet_prefix(struct_name, value) ((struct_name).global.subnet_prefix = (uint64_t)(value))
#define SET_iba_gid_t__interface_id(struct_name, value) ((struct_name).global.interface_id = (uint64_t)(value))

#define GET_iba_gid_t__subnet_prefix(struct_name, dummy) ((iba_uint64_t)((struct_name).global.subnet_prefix))
#define GET_iba_gid_t__interface_id(struct_name, dummy) ((iba_uint64_t)((struct_name).global.interface_id))

/* -------------------------------------------------------------------------------- */

typedef struct ibv_ah_attr  iba_av_t;

/*
----- INTERN -----

struct ibv_global_route {
	union ibv_gid		dgid;
	uint32_t		flow_label;
	uint8_t			sgid_index;
	uint8_t			hop_limit;
	uint8_t			traffic_class;
};

struct ibv_ah_attr {
	struct ibv_global_route	grh;
	uint16_t		dlid;
	uint8_t			sl;
	uint8_t			src_path_bits;
	uint8_t			static_rate;
	uint8_t			is_global;
	uint8_t			port_num;
};

----- EXTERN -----

typedef struct {
  iba_uint16_t  dlid;
  iba_uint8_t   sl;
  iba_uint8_t   src_path_bits;
  iba_uint8_t   static_rate;
  iba_bool_t    is_global;
  iba_gid_t     dgid;
  iba_uint32_t  flow_label;
  iba_uint8_t   sgid_index;
  iba_uint8_t   hop_limit;
  iba_uint8_t   traffic_class;
  iba_uint8_t   port;
} iba_av_t;

------------------
*/

#define SET_iba_av_t__dlid(struct_name, value) ((struct_name).dlid = (uint16_t)(value))
#define SET_iba_av_t__sl(struct_name, value) ((struct_name).sl = (uint8_t)(value))
#define SET_iba_av_t__src_path_bits(struct_name, value) ((struct_name).src_path_bits = (uint8_t)(value))
#define SET_iba_av_t__static_rate(struct_name, value) ((struct_name).static_rate = (uint8_t)(value))
#define SET_iba_av_t__is_global(struct_name, value) ((struct_name).is_global = (uint8_t)(value))
#define SET_iba_av_t__dgid(struct_name, field_name, ...) (SET_iba_gid_t__##field_name((struct_name).grh.dgid, __VA_ARGS__))
#define SET_iba_av_t__flow_label(struct_name, value) ((struct_name).grh.flow_label = (uint32_t)(value))
#define SET_iba_av_t__sgid_index(struct_name, value) ((struct_name).grh.sgid_index = (uint8_t)(value))
#define SET_iba_av_t__hop_limit(struct_name, value) ((struct_name).grh.hop_limit = (uint8_t)(value))
#define SET_iba_av_t__traffic_class(struct_name, value) ((struct_name).grh.traffic_class = (uint8_t)(value))
#define SET_iba_av_t__port(struct_name, value) ((struct_name).port_num = (uint8_t)(value))


#define GET_iba_av_t__dlid(struct_name, dummy) ((iba_uint16_t)((struct_name).dlid))
#define GET_iba_av_t__sl(struct_name, dummy) ((iba_uint8_t)((struct_name).sl))
#define GET_iba_av_t__src_path_bits(struct_name, dummy) ((iba_uint8_t)((struct_name).src_path_bits))
#define GET_iba_av_t__static_rate(struct_name, dummy) ((iba_uint8_t)((struct_name).static_rate))
#define GET_iba_av_t__is_global(struct_name, dummy) ((iba_bool_t)((struct_name).is_global))
#define GET_iba_av_t__dgid(struct_name, field_name, ...) (GET_iba_gid_t__##field_name((struct_name).grh.dgid, __VA_ARGS__))
#define GET_iba_av_t__flow_label(struct_name, dummy) ((iba_uint32_t)((struct_name).grh.flow_label))
#define GET_iba_av_t__sgid_index(struct_name, dummy) ((iba_uint8_t)((struct_name).grh.sgid_index))
#define GET_iba_av_t__hop_limit(struct_name, dummy) ((iba_uint8_t)((struct_name).grh.hop_limit))
#define GET_iba_av_t__traffic_class(struct_name, dummy) ((iba_uint8_t)((struct_name).grh.traffic_class))
#define GET_iba_av_t__port(struct_name, dummy) ((iba_uint8_t)((struct_name).port_num))

/* -------------------------------------------------------------------------------- */

typedef struct ibv_qp_attr  iba_qp_attr_t;

/*
----- INTERN -----

struct ibv_qp_attr {
	enum ibv_qp_state   qp_state;
	enum ibv_qp_state   cur_qp_state;
	enum ibv_mtu        path_mtu;
	enum ibv_mig_state  path_mig_state;
	uint32_t            qkey;
	uint32_t            rq_psn;
	uint32_t            sq_psn;
	uint32_t            dest_qp_num;
	int                 qp_access_flags;
	struct ibv_qp_cap   cap;
	struct ibv_ah_attr  ah_attr;
	struct ibv_ah_attr  alt_ah_attr;
	uint16_t            pkey_index;
	uint16_t            alt_pkey_index;
	uint8_t             en_sqd_async_notify;
	uint8_t             sq_draining;
	uint8_t             max_rd_atomic;
	uint8_t             max_dest_rd_atomic;
	uint8_t             min_rnr_timer;
	uint8_t             port_num;
	uint8_t             timeout;
	uint8_t             retry_cnt;
	uint8_t             rnr_retry;
	uint8_t             alt_port_num;
	uint8_t             alt_timeout;
}; */
/*
----- EXTERN -----

typedef struct {
  iba_qp_state_t       qp_state;
  iba_mtu_t            path_mtu;
  iba_mig_state_t      path_mig_state;
  iba_uint32_t         qkey;
  iba_uint32_t         rq_psn;
  iba_uint32_t         sq_psn;
  iba_uint32_t         dest_qp_num;
  iba_rdma_atom_acl_t  remote_atomic_flags;
  iba_qp_cap_t         cap;
  iba_bool_t           en_sqd_async_notify; 
  iba_bool_t           sq_draining;
  iba_uint8_t          max_rd_atomic;
  iba_uint8_t          max_dest_rd_atomic;
  iba_uint8_t          min_rnr_timer;
  iba_uint16_t         pkey_index;
  iba_uint8_t          port;
  iba_av_t             av;
  iba_uint8_t          timeout;
  iba_uint8_t          retry_count;
  iba_uint8_t          rnr_retry;
  iba_uint16_t         alt_pkey_index;
  iba_uint8_t          alt_port;
  iba_av_t             alt_av;
  iba_uint8_t          alt_timeout;
} iba_qp_attr_t;

------------------
*/

#define SET_iba_qp_attr_t__qp_state(struct_name, value) ((struct_name).qp_state = (enum ibv_qp_state)(value))
#define SET_iba_qp_attr_t__path_mtu(struct_name, value) ((struct_name).path_mtu = (enum ibv_mtu)(value))
#define SET_iba_qp_attr_t__path_mig_state(struct_name, value) ((struct_name).path_mig_state = (enum ibv_mig_state)(value))
#define SET_iba_qp_attr_t__qkey(struct_name, value) ((struct_name).qkey = (uint32_t)(value))
#define SET_iba_qp_attr_t__rq_psn(struct_name, value) ((struct_name).rq_psn = (uint32_t)(value))
#define SET_iba_qp_attr_t__sq_psn(struct_name, value) ((struct_name).sq_psn = (uint32_t)(value))
#define SET_iba_qp_attr_t__dest_qp_num(struct_name, value) ((struct_name).dest_qp_num = (uint32_t)(value))
#define SET_iba_qp_attr_t__remote_atomic_flags(struct_name, value) ((struct_name).qp_access_flags = (int)(value))
#define SET_iba_qp_attr_t__cap(struct_name, field_name, ...) (SET_iba_qp_cap_t__##field_name((struct_name).cap, __VA_ARGS__))
#define SET_iba_qp_attr_t__en_sqd_async_notify(struct_name, value) ((struct_name).en_sqd_async_notify = (uint8_t)(value))
/* sq_draining read only */
#define SET_iba_qp_attr_t__max_rd_atomic(struct_name, value) ((struct_name).max_dest_rd_atomic = (uint8_t)(value))
#define SET_iba_qp_attr_t__max_dest_rd_atomic(struct_name, value) ((struct_name).max_rd_atomic = (uint8_t)(value))
#define SET_iba_qp_attr_t__min_rnr_timer(struct_name, value) ((struct_name).min_rnr_timer = (uint8_t)(value))
#define SET_iba_qp_attr_t__pkey_index(struct_name, value) ((struct_name).pkey_index = (uint16_t)(value))
#define SET_iba_qp_attr_t__port(struct_name, value) ((struct_name).port_num = (uint8_t)(value))
#define SET_iba_qp_attr_t__av(struct_name, field_name, ...) (SET_iba_av_t__##field_name((struct_name).ah_attr, __VA_ARGS__))
#define SET_iba_qp_attr_t__timeout(struct_name, value) ((struct_name).timeout = (uint8_t)(value))
#define SET_iba_qp_attr_t__retry_count(struct_name, value) ((struct_name).retry_cnt = (uint8_t)(value))
#define SET_iba_qp_attr_t__rnr_retry(struct_name, value) ((struct_name).rnr_retry = (uint8_t)(value))
#define SET_iba_qp_attr_t__alt_pkey_index(struct_name, value) ((struct_name).alt_pkey_index = (uint16_t)(value))
#define SET_iba_qp_attr_t__alt_port(struct_name, value) ((struct_name).alt_port_num = (uint8_t)(value))
#define SET_iba_qp_attr_t__alt_av(struct_name, field_name, ...) (SET_iba_av_t__##field_name((struct_name).alt_ah_attr, __VA_ARGS__))
#define SET_iba_qp_attr_t__alt_timeout(struct_name, value) ((struct_name).alt_timeout = (uint8_t)(value))


#define GET_iba_qp_attr_t__qp_state(struct_name, dummy) ((iba_qp_state_t)((struct_name).qp_state))
#define GET_iba_qp_attr_t__path_mtu(struct_name, dummy) ((iba_mtu_t)((struct_name).path_mtu))
#define GET_iba_qp_attr_t__path_mig_state(struct_name, dummy) ((iba_mig_state_t)((struct_name).path_mig_state))
#define GET_iba_qp_attr_t__qkey(struct_name, dummy) ((iba_uint32_t)((struct_name).qkey))
#define GET_iba_qp_attr_t__rq_psn(struct_name, dummy) ((iba_uint32_t)((struct_name).rq_psn))
#define GET_iba_qp_attr_t__sq_psn(struct_name, dummy) ((iba_uint32_t)((struct_name).sq_psn))
#define GET_iba_qp_attr_t__dest_qp_num(struct_name, dummy) ((iba_uint32_t)((struct_name).dest_qp_num))
#define GET_iba_qp_attr_t__remote_atomic_flags(struct_name, dummy) ((iba_rdma_atom_acl_t)((struct_name).qp_access_flags))
#define GET_iba_qp_attr_t__cap(struct_name, field_name, ...) (GET_iba_qp_cap_t__##field_name((struct_name).cap, __VA_ARGS__))
#define GET_iba_qp_attr_t__en_sqd_async_notify(struct_name, dummy) ((iba_bool_t)((struct_name).en_sqd_async_notify))
#define GET_iba_qp_attr_t__sq_draining(struct_name, dummy) ((iba_bool_t)((struct_name).sq_draining))
#define GET_iba_qp_attr_t__max_rd_atomic(struct_name, dummy) ((iba_uint8_t)((struct_name).max_dest_rd_atomic))
#define GET_iba_qp_attr_t__max_dest_rd_atomic(struct_name, dummy) ((iba_uint8_t)((struct_name).max_rd_atomic))
#define GET_iba_qp_attr_t__min_rnr_timer(struct_name, dummy) ((iba_uint8_t)((struct_name).min_rnr_timer))
#define GET_iba_qp_attr_t__pkey_index(struct_name, dummy) ((iba_uint16_t)((struct_name).pkey_index))
#define GET_iba_qp_attr_t__port(struct_name, dummy) ((iba_uint8_t)((struct_name).port_num))
#define GET_iba_qp_attr_t__av(struct_name, field_name, ...) (GET_iba_av_t__##field_name((struct_name).ah_attr, __VA_ARGS__))
#define GET_iba_qp_attr_t__timeout(struct_name, dummy) ((iba_uint8_t)((struct_name).timeout))
#define GET_iba_qp_attr_t__retry_count(struct_name, dummy) ((iba_uint8_t)((struct_name).retry_cnt))
#define GET_iba_qp_attr_t__rnr_retry(struct_name, dummy) ((iba_uint8_t)((struct_name).rnr_retry))
#define GET_iba_qp_attr_t__alt_pkey_index(struct_name, dummy) ((iba_uint16_t)((struct_name).alt_pkey_index))
#define GET_iba_qp_attr_t__alt_port(struct_name, dummy) ((iba_uint8_t)((struct_name).alt_port_num))
#define GET_iba_qp_attr_t__alt_av(struct_name, field_name, ...) (GET_iba_av_t__##field_name((struct_name).alt_ah_attr, __VA_ARGS__))
#define GET_iba_qp_attr_t__alt_timeout(struct_name, dummy) ((iba_uint8_t)((struct_name).alt_timeout))

/* -------------------------------------------------------------------------------- */

typedef enum ibv_qp_attr_mask  iba_qp_attr_mask_t;

#define IBA_QP_STATE                IBV_QP_STATE
#define IBA_QP_PATH_MTU             IBV_QP_PATH_MTU
#define IBA_QP_PATH_MIG_STATE       IBV_QP_PATH_MIG_STATE
#define IBA_QP_QKEY                 IBV_QP_QKEY
#define IBA_QP_RQ_PSN               IBV_QP_RQ_PSN
#define IBA_QP_SQ_PSN               IBV_QP_SQ_PSN
#define IBA_QP_DEST_QP_NUM          IBV_QP_DEST_QPN
#define IBA_QP_REMOTE_ATOMIC_FLAGS  IBV_QP_ACCESS_FLAGS
#define IBA_QP_CAP                  IBV_QP_CAP
#define IBA_QP_EN_SQD_ASYNC_NOTIFY  IBV_QP_EN_SQD_ASYNC_NOTIFY
#define IBA_QP_MAX_RD_ATOMIC        IBV_QP_MAX_DEST_RD_ATOMIC
#define IBA_QP_MAX_DEST_RD_ATOMIC   IBV_QP_MAX_QP_RD_ATOMIC
#define IBA_QP_MIN_RNR_TIMER        IBV_QP_MIN_RNR_TIMER
#define IBA_QP_PKEY_INDEX           IBV_QP_PKEY_INDEX
#define IBA_QP_PORT                 IBV_QP_PORT
#define IBA_QP_AV                   IBV_QP_AV
#define IBA_QP_TIMEOUT              IBV_QP_TIMEOUT
#define IBA_QP_RETRY_COUNT          IBV_QP_RETRY_CNT
#define IBA_QP_RNR_RETRY            IBV_QP_RNR_RETRY
#define IBA_QP_ALT_PATH             IBV_QP_ALT_PATH

#define IBA_QP_ATTR_MASK_CLR_ALL(mask)  ((mask)=0)
#define IBA_QP_ATTR_MASK_SET_ALL(mask)  ((mask)=(0x1FFFFD))
#define IBA_QP_ATTR_MASK_SET(mask,attr) ((mask)=((mask)|(attr)))
#define IBA_QP_ATTR_MASK_CLR(mask,attr) ((mask)=((mask)&(~(attr))))
#define IBA_QP_ATTR_MASK_IS_SET(mask,attr) (((mask)&(attr))!=0)

/* -------------------------------------------------------------------------------- */

typedef struct ibv_mr*  iba_mr_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef enum ibv_access_flags  iba_mr_acl_t;

#define IBA_ACCESS_LOCAL_WRITE    IBV_ACCESS_LOCAL_WRITE
#define IBA_ACCESS_REMOTE_WRITE   IBV_ACCESS_REMOTE_WRITE
#define IBA_ACCESS_REMOTE_READ    IBV_ACCESS_REMOTE_READ
#define IBA_ACCESS_REMOTE_ATOMIC  IBV_ACCESS_REMOTE_ATOMIC
#define IBA_ACCESS_MW_BIND        IBV_ACCESS_MW_BIND

/* -------------------------------------------------------------------------------- */

typedef struct {
  void*                  addr;
  size_t                 size;
  struct ibv_pd*         pd_hndl;
  enum ibv_access_flags  acl;
  uint32_t               lkey;
  uint32_t               rkey;
} iba_mr_attr_t;

/*
----- INTERN -----

----- EXTERN -----

typedef struct {
  iba_uint64_t   addr;
  iba_uint64_t   size;
  iba_pd_hndl_t  pd_hndl;
  iba_mr_acl_t   acl;
  iba_uint32_t   lkey;
  iba_uint32_t   rkey;
} iba_mr_attr_t;

------------------
*/

#define SET_iba_mr_attr_t__addr(struct_name, value) ((struct_name).addr = (void*)(value))
#define SET_iba_mr_attr_t__size(struct_name, value) ((struct_name).size = (size_t)(value))
#define SET_iba_mr_attr_t__pd_hndl(struct_name, value) ((struct_name).pd_hndl = (struct ibv_pd*)(value))
#define SET_iba_mr_attr_t__acl(struct_name, value) ((struct_name).acl = (enum ibv_access_flags)(value))
#define SET_iba_mr_attr_t__lkey(struct_name, value) ((struct_name).lkey = (uint32_t)(value))
#define SET_iba_mr_attr_t__rkey(struct_name, value) ((struct_name).rkey = (uint32_t)(value))


#define GET_iba_mr_attr_t__addr(struct_name, dummy) ((iba_uint64_t)((struct_name).addr))
#define GET_iba_mr_attr_t__size(struct_name, dummy) ((iba_uint64_t)((struct_name).size))
#define GET_iba_mr_attr_t__pd_hndl(struct_name, dummy) ((iba_pd_hndl_t)((struct_name).pd_hndl))
#define GET_iba_mr_attr_t__acl(struct_name, dummy) ((iba_mr_acl_t)((struct_name).acl))
#define GET_iba_mr_attr_t__lkey(struct_name, dummy) ((iba_uint32_t)((struct_name).lkey))
#define GET_iba_mr_attr_t__rkey(struct_name, dummy) ((iba_uint32_t)((struct_name).rkey))

/* -------------------------------------------------------------------------------- */

typedef enum ibv_wr_opcode  iba_wr_opcode_t;

#define IBA_WR_RDMA_WRITE            IBV_WR_RDMA_WRITE
#define IBA_WR_RDMA_WRITE_WITH_IMM   IBV_WR_RDMA_WRITE_WITH_IMM
#define IBA_WR_SEND                  IBV_WR_SEND
#define IBA_WR_SEND_WITH_IMM         IBV_WR_SEND_WITH_IMM
#define IBA_WR_RDMA_READ             IBV_WR_RDMA_READ
#define IBA_WR_ATOMIC_CMP_AND_SWP    IBV_WR_ATOMIC_CMP_AND_SWP
#define IBA_WR_ATOMIC_FETCH_AND_ADD  IBV_WR_ATOMIC_FETCH_AND_ADD

/* -------------------------------------------------------------------------------- */

typedef enum ibv_send_flags  iba_send_flags_t;

#define IBA_SEND_FENCE     IBV_SEND_FENCE
#define IBA_SEND_SIGNALED  IBV_SEND_SIGNALED
#define IBA_SEND_SOLICITED IBV_SEND_SOLICITED

/* -------------------------------------------------------------------------------- */

typedef struct ibv_sge  iba_sge_t;

/*
----- INTERN -----

struct ibv_sge {
	uint64_t		addr;
	uint32_t		length;
	uint32_t		lkey;
};

----- EXTERN -----

typedef struct {
  iba_uint64_t   addr;
  iba_uint32_t   length;
  iba_uint32_t   lkey;
} iba_sge_t;

------------------
*/

#define SET_iba_sge_t__addr(struct_name, value) ((struct_name).addr = (uint64_t)(value))
#define SET_iba_sge_t__length(struct_name, value) ((struct_name).length = (uint32_t)(value))
#define SET_iba_sge_t__lkey(struct_name, value) ((struct_name).lkey = (uint32_t)(value))

#define GET_iba_sge_t__addr(struct_name, dummy) ((iba_uint64_t)((struct_name).addr))
#define GET_iba_sge_t__length(struct_name, dummy) ((iba_uint32_t)((struct_name).length))
#define GET_iba_sge_t__lkey(struct_name, dummy) ((iba_uint32_t)((struct_name).lkey))

/* -------------------------------------------------------------------------------- */

typedef struct ibv_send_wr  iba_send_wr_t;

/*
----- INTERN -----

struct ibv_send_wr {
	struct ibv_send_wr  *next;
	uint64_t             wr_id;
	struct ibv_sge      *sg_list;
	int                  num_sge;
	enum ibv_wr_opcode   opcode;
	enum ibv_send_flags  send_flags;
	uint32_t             imm_data;
	union {
		struct {
			uint64_t         remote_addr;
			uint32_t         rkey;
		} rdma;
		struct {
			uint64_t         remote_addr;
			uint64_t         compare_add;
			uint64_t         swap;
			uint32_t         rkey;
		} atomic;
		struct {
			struct ibv_ah   *ah;
			uint32_t         remote_qpn;
			uint32_t         remote_qkey;
		} ud;
	} wr;
};

----- EXTERN -----

typedef struct {
  iba_uint64_t       id;
  iba_wr_opcode_t    opcode;
  iba_send_flags_t   send_flags;
  iba_sge_t         *sg_list;
  iba_uint32_t       num_sge;
  iba_uint32_t       imm_data;
  union {
    struct {
      iba_uint64_t   remote_addr;
      iba_uint32_t   rkey;
    } rdma;
    struct {
      iba_uint64_t   remote_addr;
      iba_uint64_t   compare_add;
      iba_uint64_t   swap;
      iba_uint32_t   rkey;
    } atomic;
    struct {
      iba_av_hndl_t  remote_ah;
      iba_uint32_t   remote_qp_num;
      iba_uint32_t   remote_qkey;
    } ud;
  } wr;
} iba_send_wr_t;

------------------
*/

#define SET_iba_send_wr_t__id(struct_name, value) ((struct_name).wr_id = (uint64_t)(value))
#define SET_iba_send_wr_t__opcode(struct_name, value) ((struct_name).opcode = (enum ibv_wr_opcode)(value))
#define SET_iba_send_wr_t__send_flags(struct_name, value) ((struct_name).send_flags = (enum ibv_send_flags)(value))
#define SET_iba_send_wr_t__sg_list(struct_name, value) ((struct_name).sg_list = (struct ibv_sge*)(value))
#define SET_iba_send_wr_t__num_sge(struct_name, value) ((struct_name).num_sge = (int)(value))
#define SET_iba_send_wr_t__imm_data(struct_name, value) ((struct_name).imm_data = (uint32_t)(value))
#define SET_iba_send_wr_t__wr(struct_name, op, field_name, value) (SET_iba_send_wr_t__wr__##op##__##field_name(struct_name, value))
#define SET_iba_send_wr_t__wr__rdma__remote_addr(struct_name, value) ((struct_name).wr.rdma.remote_addr = (uint64_t)(value))
#define SET_iba_send_wr_t__wr__rdma__rkey(struct_name, value) ((struct_name).wr.rdma.rkey = (uint32_t)(value))
#define SET_iba_send_wr_t__wr__atomic__remote_addr(struct_name, value) ((struct_name).wr.atomic.remote_addr = (uint64_t)(value))
#define SET_iba_send_wr_t__wr__atomic__compare_add(struct_name, value) ((struct_name).wr.atomic.compare_add = (uint64_t)(value))
#define SET_iba_send_wr_t__wr__atomic__swap(struct_name, value) ((struct_name).wr.atomic.swap = (uint64_t)(value))
#define SET_iba_send_wr_t__wr__atomic__rkey(struct_name, value) ((struct_name).wr.atomic.rkey = (uint32_t)(value))
#define SET_iba_send_wr_t__wr__ud__remote_ah(struct_name, value) ((struct_name).wr.ud.ah = (struct ibv_ah*)(value))
#define SET_iba_send_wr_t__wr__ud__remote_qp_num(struct_name, value) ((struct_name).wr.ud.remote_qpn = (uint32_t)(value))
#define SET_iba_send_wr_t__wr__ud__remote_qkey(struct_name, value) ((struct_name).wr.ud.remote_qkey = (uint32_t)(value))

#define GET_iba_send_wr_t__id(struct_name, dummy) ((iba_uint64_t)((struct_name).wr_id))
#define GET_iba_send_wr_t__opcode(struct_name, dummy) ((iba_wr_opcode_t)((struct_name).opcode))
#define GET_iba_send_wr_t__send_flags(struct_name, dummy) ((iba_send_flags_t)((struct_name).send_flags))
#define GET_iba_send_wr_t__sg_list(struct_name, dummy) ((iba_sge_t*)((struct_name).sg_list))
#define GET_iba_send_wr_t__num_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).num_sge))
#define GET_iba_send_wr_t__imm_data(struct_name, dummy) ((iba_uint32_t)((struct_name).imm_data))
#define GET_iba_send_wr_t__wr(struct_name, op, field_name) (GET_iba_send_wr_t__wr__##op##__##field_name(struct_name))
#define GET_iba_send_wr_t__wr__rdma__remote_addr(struct_name) ((iba_uint64_t)((struct_name).wr.rdma.remote_addr))
#define GET_iba_send_wr_t__wr__rdma__rkey(struct_name) ((iba_uint32_t)((struct_name).wr.rdma.rkey))
#define GET_iba_send_wr_t__wr__atomic__remote_addr(struct_name) ((iba_uint64_t)((struct_name).wr.atomic.remote_addr))
#define GET_iba_send_wr_t__wr__atomic__compare_add(struct_name) ((iba_uint64_t)((struct_name).wr.atomic.compare_add))
#define GET_iba_send_wr_t__wr__atomic__swap(struct_name) ((iba_uint64_t)((struct_name).wr.atomic.swap))
#define GET_iba_send_wr_t__wr__atomic__rkey(struct_name) ((iba_uint32_t)((struct_name).wr.atomic.rkey))
#define GET_iba_send_wr_t__wr__ud__remote_ah(struct_name) ((iba_av_hndl_t)((struct_name).wr.ud.ah))
#define GET_iba_send_wr_t__wr__ud__remote_qp_num(struct_name) ((iba_uint32_t)((struct_name).wr.ud.remote_qpn))
#define GET_iba_send_wr_t__wr__ud__remote_qkey(struct_name) ((iba_uint32_t)((struct_name).wr.ud.remote_qkey))

/* -------------------------------------------------------------------------------- */

typedef struct ibv_recv_wr  iba_recv_wr_t;

/*
----- INTERN -----

struct ibv_recv_wr {
	struct ibv_recv_wr  *next;
	uint64_t             wr_id;
	struct ibv_sge      *sg_list;
	int                  num_sge;
};

----- EXTERN -----

typedef struct {
  iba_uint64_t   id;
  iba_sge_t     *sg_list;
  iba_uint32_t   num_sge;
} iba_recv_wr_t;

------------------
*/

#define SET_iba_recv_wr_t__id(struct_name, value) ((struct_name).wr_id = (uint64_t)(value))
#define SET_iba_recv_wr_t__sg_list(struct_name, value) ((struct_name).sg_list = (struct ibv_sge*)(value))
#define SET_iba_recv_wr_t__num_sge(struct_name, value) ((struct_name).num_sge = (int)(value))

#define GET_iba_recv_wr_t__id(struct_name, dummy) ((iba_uint64_t)((struct_name).wr_id))
#define GET_iba_recv_wr_t__sg_list(struct_name, dummy) ((iba_sge_t*)((struct_name).sg_list))
#define GET_iba_recv_wr_t__num_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).num_sge))

/* -------------------------------------------------------------------------------- */

typedef enum ibv_wc_opcode  iba_wc_opcode_t;

#define IBA_WC_SQ_SEND           IBV_WC_SEND
#define IBA_WC_SQ_RDMA_WRITE     IBV_WC_RDMA_WRITE
#define IBA_WC_SQ_RDMA_READ      IBV_WC_RDMA_READ
#define IBA_WC_SQ_COMP_SWAP      IBV_WC_COMP_SWAP
#define IBA_WC_SQ_FETCH_ADD      IBV_WC_FETCH_ADD
#define IBA_WC_SQ_BIND_MW        IBV_WC_BIND_MW
#define IBA_WC_RQ_SEND           IBV_WC_RECV
#define IBA_WC_RQ_RDMA_WITH_IMM  IBV_WC_RECV_RDMA_WITH_IMM

/* -------------------------------------------------------------------------------- */

typedef enum ibv_wc_status  iba_wc_status_t;

#define IBA_WC_SUCCESS             IBV_WC_SUCCESS
#define IBA_WC_LOC_LEN_ERR         IBV_WC_LOC_LEN_ERR
#define IBA_WC_LOC_QP_OP_ERR       IBV_WC_LOC_QP_OP_ERR
#define IBA_WC_LOC_EEC_OP_ERR      IBV_WC_LOC_EEC_OP_ERR
#define IBA_WC_LOC_PROT_ERR        IBV_WC_LOC_PROT_ERR
#define IBA_WC_WR_FLUSH_ERR        IBV_WC_WR_FLUSH_ERR
#define IBA_WC_MW_BIND_ERR         IBV_WC_MW_BIND_ERR
#define IBA_WC_BAD_RESP_ERR        IBV_WC_BAD_RESP_ERR
#define IBA_WC_LOC_ACCESS_ERR      IBV_WC_LOC_ACCESS_ERR
#define IBA_WC_REM_INV_REQ_ERR     IBV_WC_REM_INV_REQ_ERR
#define IBA_WC_REM_ACCESS_ERR      IBV_WC_REM_ACCESS_ERR
#define IBA_WC_REM_OP_ERR          IBV_WC_REM_OP_ERR
#define IBA_WC_RETRY_EXC_ERR       IBV_WC_RETRY_EXC_ERR
#define IBA_WC_RNR_RETRY_EXC_ERR   IBV_WC_RNR_RETRY_EXC_ERR
#define IBA_WC_LOC_RDD_VIOL_ERR    IBV_WC_LOC_RDD_VIOL_ERR
#define IBA_WC_REM_INV_RD_REQ_ERR  IBV_WC_REM_INV_RD_REQ_ERR
#define IBA_WC_REM_ABORT_ERR       IBV_WC_REM_ABORT_ERR
#define IBA_WC_INV_EECN_ERR        IBV_WC_INV_EECN_ERR
#define IBA_WC_INV_EEC_STATE_ERR   IBV_WC_INV_EEC_STATE_ERR
#define IBA_WC_FATAL_ERR           IBV_WC_FATAL_ERR
#define IBA_WC_GENERAL_ERR         IBV_WC_GENERAL_ERR

/* -------------------------------------------------------------------------------- */

typedef struct ibv_wc  iba_wc_t;   /* read only */

/*
----- INTERN -----

enum ibv_wc_flags {
	IBV_WC_GRH		= 1 << 0,
	IBV_WC_WITH_IMM		= 1 << 1
};

struct ibv_wc {
	uint64_t		wr_id;
	enum ibv_wc_status	status;
	enum ibv_wc_opcode	opcode;
	uint32_t		vendor_err;
	uint32_t		byte_len;
	uint32_t		imm_data;
	uint32_t		qp_num;
	uint32_t		src_qp;
	enum ibv_wc_flags	wc_flags;
	uint16_t		pkey_index;
	uint16_t		slid;
	uint8_t			sl;
	uint8_t			dlid_path_bits;
};

----- EXTERN -----

typedef struct {
  iba_uint64_t     id;
  iba_wc_opcode_t  opcode;
  iba_wc_status_t  status;
  iba_uint32_t     vendor_err;
  iba_uint32_t     qp_num;
  iba_uint32_t     byte_len;
  iba_uint32_t     imm_data;
  iba_bool_t       imm_data_valid;
  iba_bool_t       grh_flag;
  iba_uint16_t     pkey_index;
  struct {
    iba_uint32_t   qp;
    iba_uint16_t   slid;
    iba_uint8_t    sl;
    iba_uint8_t    dlid_path_bits;
  } ud;
} iba_wc_t;

------------------
*/

#define GET_iba_wc_t__id(struct_name, dummy) ((iba_uint64_t)((struct_name).wr_id))
#define GET_iba_wc_t__opcode(struct_name, dummy) ((iba_wc_opcode_t)((struct_name).opcode))
#define GET_iba_wc_t__status(struct_name, dummy) ((iba_wc_status_t)((struct_name).status))
#define GET_iba_wc_t__vendor_err(struct_name, dummy) ((iba_uint32_t)((struct_name).vendor_err))
#define GET_iba_wc_t__qp_num(struct_name, dummy) ((iba_uint32_t)((struct_name).qp_num))
#define GET_iba_wc_t__byte_len(struct_name, dummy) ((iba_uint32_t)((struct_name).byte_len))
#define GET_iba_wc_t__imm_data(struct_name, dummy) ((iba_uint32_t)((struct_name).imm_data))
#define GET_iba_wc_t__imm_data_valid(struct_name, dummy) ((iba_bool_t)(((struct_name).wc_flags & IBV_WC_WITH_IMM) != 0))
#define GET_iba_wc_t__grh_flag(struct_name, dummy) ((iba_bool_t)(((struct_name).wc_flags & IBV_WC_GRH) != 0))
#define GET_iba_wc_t__pkey_index(struct_name, dummy) ((iba_uint16_t)((struct_name).pkey_index))
#define GET_iba_wc_t__ud(struct_name, field_name) (GET_iba_wc_t__ud__##field_name(struct_name))
#define GET_iba_wc_t__ud__qp(struct_name) ((iba_uint32_t)((struct_name).src_qp))
#define GET_iba_wc_t__ud__slid(struct_name) ((iba_uint16_t)((struct_name).slid))
#define GET_iba_wc_t__ud__sl(struct_name) ((iba_uint8_t)((struct_name).sl))
#define GET_iba_wc_t__ud__dlid_path_bits(struct_name) ((iba_uint8_t)((struct_name).dlid_path_bits))

/* -------------------------------------------------------------------------------- */





/* -------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                        */
/* -------------------------------------------------------------------------------- */


/*
 *  iba_get_hcas - Get HCA List
 */

static iba_status_t iba_get_hcas(iba_uint32_t num_of_entries, iba_device_hndl_t *devices_p, iba_uint32_t *ret_num_of_entries_p)
{
  struct ibv_device **dev_list;
  int num_of_devices = 0;

  *ret_num_of_entries_p = 0;
  dev_list = ibv_get_device_list(&num_of_devices);
  if(!dev_list) return IBA_ERROR; 
 
  *ret_num_of_entries_p = (iba_uint32_t)num_of_devices;
  if (num_of_entries < *ret_num_of_entries_p) {
	ibv_free_device_list(dev_list);
	return IBA_EAGAIN;
  }
  *ret_num_of_entries_p = 0;
  
  /* iterate through the array and copy/count the entries
   * maybe useless, but necessary for the iba style */
  while (*dev_list)
  {
    devices_p[(*ret_num_of_entries_p)++] = *dev_list; 
    ++dev_list;
  }

  return IBA_OK;
}


/*
 *  iba_query_gid_table - Get GID Table
 */

static iba_status_t iba_query_gid_table(iba_hca_hndl_t hca_hndl, iba_uint8_t port_num, iba_uint16_t table_len_in, iba_uint16_t *table_len_out_p, iba_gid_t *gid_table_p)
{
  int i;
  struct ibv_port_attr port_attr;
  if (ibv_query_port((struct ibv_context*)hca_hndl, (uint8_t)port_num, &port_attr) != 0) return IBA_ERROR;
  *table_len_out_p = (iba_uint16_t)port_attr.gid_tbl_len;
  if (table_len_in < *table_len_out_p) return IBA_EAGAIN;
  for (i = 0; i < *table_len_out_p; i++)
  {
    if (ibv_query_gid((struct ibv_context*)hca_hndl, (uint8_t)port_num, i, &((union ibv_gid)gid_table_p[i])) != 0) return IBA_ERROR;
  }
  return IBA_OK;  
}


/*
 *  iba_query_pkey_table - Get PKEY Table
 */

static iba_status_t iba_query_pkey_table(iba_hca_hndl_t hca_hndl, iba_uint8_t port_num, iba_uint16_t table_len_in, iba_uint16_t *table_len_out_p, iba_pkey_t *pkey_table_p)
{
  int i;
  struct ibv_port_attr port_attr;
  if (ibv_query_port((struct ibv_context*)hca_hndl, (uint8_t)port_num, &port_attr) != 0) return IBA_ERROR;
  *table_len_out_p = (iba_uint16_t)port_attr.pkey_tbl_len;
  if (table_len_in < *table_len_out_p) return IBA_EAGAIN;
  for (i = 0; i < *table_len_out_p; i++)
  {
    if (ibv_query_pkey((struct ibv_context*)hca_hndl, (uint8_t)port_num, i, &((uint16_t)pkey_table_p[i])) != 0) return IBA_ERROR;
  }
  return IBA_OK;  
}


/*
 *  iba_get_hca_hndl - Initialize HCA
 */

#define iba_get_hca_hndl(device_hndl, hca_hndl_p) ( \
  RES1(*(hca_hndl_p) = ibv_open_device( (struct ibv_device*)(device_hndl))))


/*
 *  iba_release_hca_hndl - Release HCA
 */

#define iba_release_hca_hndl(hca_hndl) ( \
    RES2( ibv_close_device( (struct ibv_context*)(hca_hndl))))


/*
 *  iba_query_hca - Get HCA Properties
 */

#define iba_query_hca(hca_hndl, hca_cap_p) ( \
  RES2( ibv_query_device( (struct ibv_context*)(hca_hndl), \
                          (struct ibv_device_attr*)(hca_cap_p))))


/*
 *  iba_query_hca - Get HCA Port Properties
 */

#define iba_query_hca_port(hca_hndl, port_num, hca_port_p) (   \
  RES2( ibv_query_port( (struct ibv_context*)(hca_hndl),       \
                        (uint8_t)(port_num),                   \
                        (struct ibv_port_attr*)(hca_port_p))))


/*
 *  iba_alloc_pd - Allocate Protection Domain
 */

#define iba_alloc_pd(hca_hndl, pd_hndl_p) ( \
  RES1(*(pd_hndl_p) = (iba_pd_hndl_t)ibv_alloc_pd( (struct ibv_context*)(hca_hndl))))


/*
 *  iba_dealloc_pd - Free Protection Domain
 */

#define iba_dealloc_pd(hca_hndl, pd_hndl) ( \
  RES2( ibv_dealloc_pd( (struct ibv_pd*)(pd_hndl))))


/*
 *  iba_create_cq - Create Completion Queue
 */

static iba_status_t iba_create_cq(iba_hca_hndl_t hca_hndl, iba_uint32_t cqe, iba_cq_hndl_t *cq_hndl_p, iba_uint32_t *ret_cqe_p)
{
  if ((*cq_hndl_p = (iba_cq_hndl_t)ibv_create_cq((struct ibv_context*)hca_hndl, (int)cqe, NULL, NULL, 0)) == NULL) return IBA_ERROR;
  *ret_cqe_p = (iba_uint32_t)((*cq_hndl_p)->cqe);
  return IBA_OK;
}


/*
 *  iba_destroy_cq - Destroy Completion Queue
 */

#define iba_destroy_cq(hca_hndl, cq_hndl) ( \
  RES2( ibv_destroy_cq( (struct ibv_cq *)(cq_hndl))))


/*
 *  iba_set_priv_context4cq - Set Completion Queue Private Context
 */

static iba_status_t iba_set_priv_context4cq(iba_hca_hndl_t hca_hndl, iba_cq_hndl_t cq_hndl, void *priv_context)
{
  if (hca_hndl);
  if (!cq_hndl) return IBA_ERROR;
  cq_hndl->cq_context = priv_context;
  return IBA_OK;
}


/*
 *  iba_get_priv_context4cq - Get Completion Queue Private Context
 */

static iba_status_t iba_get_priv_context4cq(iba_hca_hndl_t hca_hndl, iba_cq_hndl_t cq_hndl, void **priv_context_p)
{
  if (hca_hndl);
  if (!cq_hndl) return IBA_ERROR;
  *priv_context_p = cq_hndl->cq_context;
  return IBA_OK;
}


/*
 *  iba_create_qp - Create Queue Pair
 */

static iba_status_t iba_create_qp(iba_hca_hndl_t hca_hndl, iba_qp_init_attr_t *qp_init_attr_p, iba_qp_hndl_t *qp_hndl_p, iba_qp_prop_t *qp_prop_p)
{
  if (hca_hndl);
  qp_init_attr_p->openib_init_attr.cap.max_send_wr = qp_init_attr_p->cap.max_send_wr;
  qp_init_attr_p->openib_init_attr.cap.max_recv_wr = qp_init_attr_p->cap.max_recv_wr;
  qp_init_attr_p->openib_init_attr.cap.max_send_sge = qp_init_attr_p->cap.max_send_sge;
  qp_init_attr_p->openib_init_attr.cap.max_recv_sge = qp_init_attr_p->cap.max_recv_sge;
  qp_init_attr_p->openib_init_attr.cap.max_inline_data = qp_init_attr_p->cap.max_inline_data;
  if ((*qp_hndl_p = (iba_qp_hndl_t)ibv_create_qp(qp_init_attr_p->pd, &(qp_init_attr_p->openib_init_attr))) == NULL) return IBA_ERROR;
  qp_prop_p->qp_num = (*qp_hndl_p)->qp_num;
  qp_prop_p->cap.max_send_wr = qp_init_attr_p->openib_init_attr.cap.max_send_wr;
  qp_prop_p->cap.max_recv_wr = qp_init_attr_p->openib_init_attr.cap.max_recv_wr;
  qp_prop_p->cap.max_send_sge = qp_init_attr_p->openib_init_attr.cap.max_send_sge;
  qp_prop_p->cap.max_recv_sge = qp_init_attr_p->openib_init_attr.cap.max_recv_sge;
  qp_prop_p->cap.max_inline_data = qp_init_attr_p->openib_init_attr.cap.max_inline_data;
  return IBA_OK;
}


/*
 *  iba_destroy_qp - Destroy Queue Pair
 */

#define iba_destroy_qp(hca_hndl, qp_hndl) ( \
  RES2( ibv_destroy_qp( (struct ibv_qp*)(qp_hndl))))


/*
 *  iba_modify_qp - Modify Queue Pair
 */

#define iba_modify_qp(hca_hndl, qp_hndl, qp_attr_p, qp_attr_mask_p) (  \
  RES2( ibv_modify_qp( (struct ibv_qp*)(qp_hndl),                      \
                       (struct ibv_qp_attr*)(qp_attr_p),               \
                       *((enum ibv_qp_attr_mask*)(qp_attr_mask_p)))))


/*
 *  iba_set_priv_context4qp - Set Queue Pair Private Context
 */

static iba_status_t iba_set_priv_context4qp(iba_hca_hndl_t hca_hndl, iba_qp_hndl_t qp_hndl, void *priv_context)
{
  if (hca_hndl);
  if (!qp_hndl) return IBA_ERROR;
  qp_hndl->qp_context = priv_context;
  return IBA_OK;
}


/*
 *  iba_get_priv_context4qp - Get Queue Pair Private Context
 */

static iba_status_t iba_get_priv_context4qp(iba_hca_hndl_t hca_hndl, iba_qp_hndl_t qp_hndl, void **priv_context_p)
{
  if (hca_hndl);
  if (!qp_hndl) return IBA_ERROR;
  *priv_context_p = qp_hndl->qp_context;
  return IBA_OK;
}


/*
 *  iba_reg_mr - Register Memory Region
 */

static iba_status_t iba_reg_mr(iba_hca_hndl_t hca_hndl, iba_mr_attr_t *mr_attr_p, iba_mr_hndl_t *mr_hndl_p, iba_mr_attr_t *ret_mr_attr_p)
{
  if (hca_hndl);
  if ((*mr_hndl_p = (iba_mr_hndl_t)ibv_reg_mr(mr_attr_p->pd_hndl, mr_attr_p->addr, mr_attr_p->size, mr_attr_p->acl)) == NULL) return IBA_ERROR;
  ret_mr_attr_p->addr = mr_attr_p->addr;
  ret_mr_attr_p->size = mr_attr_p->size;
  ret_mr_attr_p->pd_hndl = mr_attr_p->pd_hndl;
  ret_mr_attr_p->acl = mr_attr_p->acl;
  ret_mr_attr_p->lkey = (*mr_hndl_p)->lkey;
  ret_mr_attr_p->rkey = (*mr_hndl_p)->rkey;
  return IBA_OK;
}


/*
 *  iba_dereg_mr - Deregister Memory Region
 */

#define iba_dereg_mr(hca_hndl, mr_hndl) ( \
  RES2( ibv_dereg_mr((struct ibv_mr*)(mr_hndl))))


/*
 *  iba_post_send - Post Send Request
 */

static inline iba_status_t iba_post_send(iba_hca_hndl_t hca_hndl, iba_qp_hndl_t qp_hndl, iba_send_wr_t *send_wr_p)
{
  struct ibv_send_wr *bad_wr_p;
  if (hca_hndl);
  send_wr_p->next = NULL;
  return RES2( ibv_post_send((struct ibv_qp*)qp_hndl, (struct ibv_send_wr*)send_wr_p, &bad_wr_p));
}


/*
 *  iba_post_send_list - Post List of Send Requests
 */

static inline iba_status_t iba_post_send_list(iba_hca_hndl_t hca_hndl, iba_qp_hndl_t qp_hndl, iba_uint32_t num_of_requests, iba_send_wr_t *send_wr_array)
{
  struct ibv_send_wr *bad_wr_p;
  int i;
  if (hca_hndl);
  for (i = 1; i < num_of_requests; i++)
    send_wr_array[i-1].next = &send_wr_array[i];
  send_wr_array[num_of_requests-1].next = NULL;
  return RES2( ibv_post_send((struct ibv_qp*)qp_hndl, (struct ibv_send_wr*)send_wr_array, &bad_wr_p));
}


/*
 *  iba_post_send - Post Inline Send Request
 */

static inline iba_status_t iba_post_inline_send(iba_hca_hndl_t hca_hndl, iba_qp_hndl_t qp_hndl, iba_send_wr_t *send_wr_p)
{
  struct ibv_send_wr *bad_wr_p;
  int status;
  if (hca_hndl);
  send_wr_p->next = NULL;
  send_wr_p->send_flags |= IBV_SEND_INLINE;
  status = ibv_post_send((struct ibv_qp*)qp_hndl, (struct ibv_send_wr*)send_wr_p, &bad_wr_p);
  send_wr_p->send_flags &= (~IBV_SEND_INLINE);
  return RES2(status);
}


/*
 *  iba_post_recv - Post Recv Request
 */

static inline iba_status_t iba_post_recv(iba_hca_hndl_t hca_hndl, iba_qp_hndl_t qp_hndl, iba_recv_wr_t *recv_wr_p)
{
  struct ibv_recv_wr *bad_wr_p;
  if (hca_hndl);
  recv_wr_p->next = NULL;
  return RES2( ibv_post_recv((struct ibv_qp*)qp_hndl, (struct ibv_recv_wr*)recv_wr_p, &bad_wr_p));
}


/*
 *  iba_post_recv_list - Post List of Recv Requests
 */

static inline iba_status_t iba_post_recv_list(iba_hca_hndl_t hca_hndl, iba_qp_hndl_t qp_hndl, iba_uint32_t num_of_requests, iba_recv_wr_t *recv_wr_array)
{
  struct ibv_recv_wr *bad_wr_p;
  int i;
  if (hca_hndl);
  for (i = 1; i < num_of_requests; i++)
    recv_wr_array[i-1].next = &recv_wr_array[i];
  recv_wr_array[num_of_requests-1].next = NULL;
  return RES2( ibv_post_recv((struct ibv_qp*)qp_hndl, (struct ibv_recv_wr*)recv_wr_array, &bad_wr_p));
}


/*
 *  iba_poll_cq - Poll CQ
 */

static inline iba_status_t iba_poll_cq(iba_hca_hndl_t hca_hndl, iba_cq_hndl_t cq_hndl, iba_wc_t *wc_p)
{
  int ret;
  if (hca_hndl);
  ret = ibv_poll_cq((struct ibv_cq*)cq_hndl, 1, (struct ibv_wc*)wc_p);
  switch (ret)
  {
    case 0:
      return IBA_CQ_EMPTY;
    case 1:
      return IBA_OK;
    default:
      return IBA_ERROR;
  }
}

/*
 *  iba_create_addr_hndl - Create Address Handle
 */

#define iba_create_addr_hndl(hca_hndl, pd_hndl, av_p, av_hndl_p) ( \
  RES1(*(av_hndl_p) = (iba_av_hndl_t)ibv_create_ah( (struct ibv_pd*)(pd_hndl), (struct ibv_ah_attr*)(av_p))))


/*
 *  iba_destroy_addr_hndl - Destroy Address Handle
 */

#define iba_destroy_addr_hndl(hca_hndl, av_hndl) ( \
  RES2( ibv_destroy_ah( (struct ibv_ah *)(av_hndl))))


/*
 *  iba_attach_multicast - Attach QP to Multicast Group
 */

#define iba_attach_multicast(hca_hndl, qp_hndl, gid_p, lid) ( \
  RES2( ibv_attach_mcast( (struct ibv_qp*)(qp_hndl),          \
                          (union ibv_gid*)(gid_p),            \
                          (uint16_t)(lid))))


/*
 *  iba_detach_multicast - Detach QP from Multicast Group
 */

#define iba_detach_multicast(hca_hndl, qp_hndl, gid_p, lid) ( \
  RES2( ibv_detach_mcast( (struct ibv_qp*)(qp_hndl),          \
                          (union ibv_gid*)(gid_p),            \
                          (uint16_t)(lid))))


#define IBA_ERROR_LIST \
IBA_ERROR_INFO( IBA_OK,		"Operation Completed Successfully") \
IBA_ERROR_INFO( IBA_EAGAIN,	"Resources temporary unavailable") \
IBA_ERROR_INFO( IBA_CQ_EMPTY,   "CQ is empty") \


static const char* iba_strerror(iba_status_t errnum)
{
  switch (errnum)
  {
#define IBA_ERROR_INFO(A, B) case A: return B;
    IBA_ERROR_LIST
#undef IBA_ERROR_INFO
    default:
      return "IBA_UNKNOWN_ERROR";
  }
}


static const char* iba_strerror_sym(iba_status_t errnum)
{
  switch (errnum)
  {
#define IBA_ERROR_INFO(A, B) case A: return #A;
    IBA_ERROR_LIST
#undef IBA_ERROR_INFO
    default:
      return "IBA_UNKNOWN_ERROR";
  }
}


static const char* iba_wc_status_sym(iba_wc_status_t status)
{
   switch(status) {
      case IBA_WC_SUCCESS: return "IBA_WC_SUCCESS";
      case IBA_WC_LOC_LEN_ERR: return "IBA_WC_LOC_LEN_ERR";
      case IBA_WC_LOC_QP_OP_ERR: return "IBA_WC_LOC_QP_OP_ERR";
      case IBA_WC_LOC_EEC_OP_ERR: return "IBA_WC_LOC_EEC_OP_ERR";
      case IBA_WC_LOC_PROT_ERR: return "IBA_WC_LOC_PROT_ERR";
      case IBA_WC_WR_FLUSH_ERR: return "IBA_WC_WR_FLUSH_ERR";
      case IBA_WC_MW_BIND_ERR: return "IBA_WC_MW_BIND_ERR";
      case IBA_WC_BAD_RESP_ERR: return "IBA_WC_BAD_RESP_ERR";
      case IBA_WC_LOC_ACCESS_ERR: return "IBA_WC_LOC_ACCESS_ERR";
      case IBA_WC_REM_INV_REQ_ERR: return "IBA_WC_REM_INV_REQ_ERR";
      case IBA_WC_REM_ACCESS_ERR: return "IBA_WC_REM_ACCESS_ERR";
      case IBA_WC_REM_OP_ERR: return "IBA_WC_REM_OP_ERR";
      case IBA_WC_RETRY_EXC_ERR: return "IBA_WC_RETRY_EXC_ERR";
      case IBA_WC_RNR_RETRY_EXC_ERR: return "IBA_WC_RNR_RETRY_EXC_ERR";
      case IBA_WC_LOC_RDD_VIOL_ERR: return "IBA_WC_LOC_RDD_VIOL_ERR";
      case IBA_WC_REM_INV_RD_REQ_ERR: return "IBA_WC_REM_INV_RD_REQ_ERR";
      case IBA_WC_REM_ABORT_ERR: return "IBA_WC_REM_ABORT_ERR";
      case IBA_WC_INV_EECN_ERR: return "IBA_WC_INV_EECN_ERR";
      case IBA_WC_INV_EEC_STATE_ERR: return "IBA_WC_INV_EEC_STATE_ERR";
      case IBA_WC_FATAL_ERR: return "IBA_WC_FATAL_ERR";
      case IBA_WC_GENERAL_ERR: return "IBA_WC_GENERAL_ERR";
      default: return "IBA_UNKNOWN_STATUS";
   }
}	    


static const char* iba_cqe_opcode_sym(iba_wc_opcode_t opcode)
{
   switch(opcode) {
      case IBA_WC_SQ_SEND: return "IBA_WC_SQ_SEND";
      case IBA_WC_SQ_RDMA_WRITE: return "IBA_WC_RDMA_WRITE";
      case IBA_WC_SQ_RDMA_READ: return "IBA_WC_RDMA_READ";
      case IBA_WC_SQ_COMP_SWAP: return "IBA_WC_COMP_SWAP";
      case IBA_WC_SQ_FETCH_ADD: return "IBA_WC_FETCH_ADD";
      case IBA_WC_SQ_BIND_MW: return "IBA_WC_BIND_MW";
      case IBA_WC_RQ_SEND: return "IBA_WC_RECV";
      case IBA_WC_RQ_RDMA_WITH_IMM: return "IBA_WC_RECV_RDMA_WITH_IMM";
      default: return "IBA_UNKNOWN_OPCODE";
   }   
}

