/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */


#include <vapi.h>
#include <evapi.h>
#include <vapi_common.h>

/* -------------------------------------------------------------------------------- */

typedef int iba_status_t;

#define IBA_OK       0
#define IBA_ERROR    1
#define IBA_EAGAIN   VAPI_EAGAIN
#define IBA_CQ_EMPTY VAPI_CQ_EMPTY

#define RES1(x) ((x) == VAPI_OK ? IBA_OK : IBA_ERROR)

/* -------------------------------------------------------------------------------- */

typedef unsigned char  iba_bool_t;

/* -------------------------------------------------------------------------------- */

typedef u_int8_t  iba_uint8_t;

/* -------------------------------------------------------------------------------- */

typedef u_int16_t  iba_uint16_t;

/* -------------------------------------------------------------------------------- */

typedef u_int32_t  iba_uint32_t;

/* -------------------------------------------------------------------------------- */

typedef u_int64_t  iba_uint64_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_hca_id_t  iba_device_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_hca_hndl_t  iba_hca_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_cq_hndl_t  iba_cq_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_qp_hndl_t  iba_qp_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_srq_hndl_t  iba_srq_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_pd_hndl_t  iba_pd_hndl_t;

/* -------------------------------------------------------------------------------- */

#define IBA_INVAL_HNDL  VAPI_INVAL_HNDL

/* -------------------------------------------------------------------------------- */

typedef VAPI_ud_av_hndl_t  iba_av_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_pkey_t  iba_pkey_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_sig_type_t  iba_sig_type_t;

#define IBA_SIGNAL_ALL_WR  VAPI_SIGNAL_ALL_WR
#define IBA_SIGNAL_REQ_WR  VAPI_SIGNAL_REQ_WR

/* -------------------------------------------------------------------------------- */

typedef VAPI_ts_type_t  iba_qp_type_t;

#define IBA_QPT_RC   VAPI_TS_RC
#define IBA_QPT_UC   VAPI_TS_UC
#define IBA_QPT_UD   VAPI_TS_UD

/* -------------------------------------------------------------------------------- */

typedef VAPI_qp_cap_t  iba_qp_cap_t;

/*
----- INTERN -----

typedef struct {
  u_int32_t  max_oust_wr_sq;
  u_int32_t  max_oust_wr_rq;
  u_int32_t  max_sg_size_sq;
  u_int32_t  max_sg_size_rq;
  u_int32_t  max_inline_data_sq;
} VAPI_qp_cap_t;

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

#define SET_iba_qp_cap_t__max_send_wr(struct_name, value) ((struct_name).max_oust_wr_sq = (u_int32_t)(value))
#define SET_iba_qp_cap_t__max_recv_wr(struct_name, value) ((struct_name).max_oust_wr_rq = (u_int32_t)(value))
#define SET_iba_qp_cap_t__max_send_sge(struct_name, value) ((struct_name).max_sg_size_sq = (u_int32_t)(value))
#define SET_iba_qp_cap_t__max_recv_sge(struct_name, value) ((struct_name).max_sg_size_rq = (u_int32_t)(value))
#define SET_iba_qp_cap_t__max_inline_data(struct_name, value) ((struct_name).max_inline_data_sq = (u_int32_t)(value))

#define GET_iba_qp_cap_t__max_send_wr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_oust_wr_sq))
#define GET_iba_qp_cap_t__max_recv_wr(struct_name, dummy) ((iba_uint32_t)((struct_name).max_oust_wr_rq))
#define GET_iba_qp_cap_t__max_send_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).max_sg_size_sq))
#define GET_iba_qp_cap_t__max_recv_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).max_sg_size_rq))
#define GET_iba_qp_cap_t__max_inline_data(struct_name, dummy) ((iba_uint32_t)((struct_name).max_inline_data_sq))

/* -------------------------------------------------------------------------------- */

typedef VAPI_qp_prop_t  iba_qp_prop_t; /* read only */

/*
----- INTERN -----

typedef struct {
  VAPI_qp_num_t   qp_num;
  VAPI_qp_cap_t   cap;
} VAPI_qp_prop_t;

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
  VAPI_qp_init_attr_t      vapi_init_attr;
  VAPI_qp_init_attr_ext_t  vapi_init_attr_ext;
} iba_qp_init_attr_t;

/*
----- INTERN -----

typedef struct {
	VAPI_cq_hndl_t  sq_cq_hndl;
	VAPI_cq_hndl_t  rq_cq_hndl;
	VAPI_qp_cap_t   cap;
	VAPI_rdd_hndl_t rdd_hndl;
	VAPI_sig_type_t sq_sig_type;
	VAPI_sig_type_t rq_sig_type;
	VAPI_pd_hndl_t  pd_hndl;
	VAPI_ts_type_t  ts_type;
} VAPI_qp_init_attr_t;

typedef struct {
  VAPI_srq_hndl_t srq_hndl;
} VAPI_qp_init_attr_ext_t;

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

#define SET_iba_qp_init_attr_t__qp_type(struct_name, value) ((struct_name).vapi_init_attr.ts_type = (VAPI_ts_type_t)(value))
#define SET_iba_qp_init_attr_t__sq_cq_hndl(struct_name, value) ((struct_name).vapi_init_attr.sq_cq_hndl = (VAPI_cq_hndl_t)(value))
#define SET_iba_qp_init_attr_t__rq_cq_hndl(struct_name, value) ((struct_name).vapi_init_attr.rq_cq_hndl = (VAPI_cq_hndl_t)(value))
#define SET_iba_qp_init_attr_t__srq_hndl(struct_name, value) ((struct_name).vapi_init_attr_ext.srq_hndl = (VAPI_srq_hndl_t)(value))
#define SET_iba_qp_init_attr_t__pd_hndl(struct_name, value) ((struct_name).vapi_init_attr.pd_hndl = (VAPI_pd_hndl_t)(value))
#define SET_iba_qp_init_attr_t__sq_sig_type(struct_name, value) { (struct_name).vapi_init_attr.sq_sig_type = (VAPI_sig_type_t)(value); (struct_name).vapi_init_attr.rq_sig_type = VAPI_SIGNAL_ALL_WR; }
#define SET_iba_qp_init_attr_t__cap(struct_name, field_name, ...) (SET_iba_qp_cap_t__##field_name((struct_name).vapi_init_attr.cap, __VA_ARGS__))

#define GET_iba_qp_init_attr_t__qp_type(struct_name, dummy) ((iba_qp_type_t)((struct_name).vapi_init_attr.ts_type))
#define GET_iba_qp_init_attr_t__sq_cq_hndl(struct_name, dummy) ((iba_cq_hndl_t)((struct_name).vapi_init_attr.sq_cq_hndl))
#define GET_iba_qp_init_attr_t__rq_cq_hndl(struct_name, dummy) ((iba_cq_hndl_t)((struct_name).vapi_init_attr.rq_cq_hndl))
#define GET_iba_qp_init_attr_t__srq_hndl(struct_name, dummy) ((iba_srq_hndl_t)((struct_name).vapi_init_attr_ext.srq_hndl))
#define GET_iba_qp_init_attr_t__pd_hndl(struct_name, dummy) ((iba_pd_hndl_t)((struct_name).vapi_init_attr.pd_hndl))
#define GET_iba_qp_init_attr_t__sq_sig_type(struct_name, dummy) ((iba_sig_type_t)((struct_name).vapi_init_attr.sq_sig_type))
#define GET_iba_qp_init_attr_t__cap(struct_name, field_name, ...) (GET_iba_qp_cap_t__##field_name((struct_name).vapi_init_attr.cap, __VA_ARGS__))

/* -------------------------------------------------------------------------------- */

typedef VAPI_hca_vendor_t  iba_hca_vendor_t; /* read only */

/*
----- INTERN -----

typedef struct {
  u_int32_t  vendor_id;
  u_int32_t  vendor_part_id;
  u_int32_t  hw_ver;
  u_int64_t  fw_ver;
  u_int8_t   pci_bus;
  u_int8_t   pci_devfn;
} VAPI_hca_vendor_t; 
	    
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

typedef u_int32_t  iba_hca_cap_flags_t; /* read only */

/*
----- INTERN -----

typedef enum {
  VAPI_RESIZE_OUS_WQE_CAP     = 1,
  VAPI_BAD_PKEY_COUNT_CAP     = (1<<1),
  VAPI_BAD_QKEY_COUNT_CAP     = (1<<2),
  VAPI_RAW_MULTI_CAP          = (1<<3),
  VAPI_AUTO_PATH_MIG_CAP      = (1<<4),
  VAPI_CHANGE_PHY_PORT_CAP    = (1<<5),
  VAPI_UD_AV_PORT_ENFORCE_CAP = (1<<6),
  VAPI_CURR_QP_STATE_MOD_CAP  = (1<<7),
  VAPI_SHUTDOWN_PORT_CAP      = (1<<8),
  VAPI_INIT_TYPE_CAP          = (1<<9),
  VAPI_PORT_ACTIVE_EV_CAP     = (1<<10),
  VAPI_SYS_IMG_GUID_CAP       = (1<<11),
  VAPI_RC_RNR_NAK_GEN_CAP     = (1<<12)
} VAPI_hca_cap_flags_t;

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

#define GET_iba_hca_cap_flags_t__resize_max_wr(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_RESIZE_OUS_WQE_CAP) != 0))
#define GET_iba_hca_cap_flags_t__bad_pkey_counter(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_BAD_PKEY_COUNT_CAP) != 0))
#define GET_iba_hca_cap_flags_t__bad_qkey_counter(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_BAD_QKEY_COUNT_CAP) != 0))
#define GET_iba_hca_cap_flags_t__raw_multicast(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_RAW_MULTI_CAP) != 0))
#define GET_iba_hca_cap_flags_t__auto_path_mig(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_AUTO_PATH_MIG_CAP) != 0))
#define GET_iba_hca_cap_flags_t__change_phy_port(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_CHANGE_PHY_PORT_CAP) != 0))
#define GET_iba_hca_cap_flags_t__ud_av_port_enforce(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_UD_AV_PORT_ENFORCE_CAP) != 0))
#define GET_iba_hca_cap_flags_t__curr_qp_state_mod(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_CURR_QP_STATE_MOD_CAP) != 0))
#define GET_iba_hca_cap_flags_t__shutdown_port(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_SHUTDOWN_PORT_CAP) != 0))
#define GET_iba_hca_cap_flags_t__init_type(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_INIT_TYPE_CAP) != 0))
#define GET_iba_hca_cap_flags_t__port_active_event(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_PORT_ACTIVE_EV_CAP) != 0))
#define GET_iba_hca_cap_flags_t__sys_image_guid(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_SYS_IMG_GUID_CAP) != 0))
#define GET_iba_hca_cap_flags_t__rc_rnr_nak_gen(struct_name, dummy) ((iba_bool_t)(((struct_name) & VAPI_RC_RNR_NAK_GEN_CAP) != 0))
/* srq_resize stored in iba_hca_cap_t */
#define GET_iba_hca_cap_flags_t__srq_resize(struct_name, dummy) ((iba_bool_t)(0))

/* -------------------------------------------------------------------------------- */

typedef VAPI_atomic_cap_t  iba_atomic_cap_t;

#define IBA_ATOMIC_CAP_NONE  VAPI_ATOMIC_CAP_NONE
#define IBA_ATOMIC_CAP_HCA   VAPI_ATOMIC_CAP_HCA
#define IBA_ATOMIC_CAP_GLOB  VAPI_ATOMIC_CAP_GLOB

/* -------------------------------------------------------------------------------- */

typedef struct {
  VAPI_hca_vendor_t  hca_vendor;
  VAPI_hca_cap_t     hca_cap;
} iba_hca_cap_t;

/*
----- INTERN -----

typedef u_int8_t  IB_port_t;
typedef u_int8_t  IB_guid_t[8];
typedef u_int8_t  VAPI_timeout_t;
typedef unsigned char MT_bool;

typedef struct {
  u_int32_t          max_num_qp;
  u_int32_t          max_qp_ous_wr;
  u_int32_t          flags;
  u_int32_t          max_num_sg_ent;
  u_int32_t          max_num_sg_ent_rd;
  u_int32_t          max_num_cq;
  u_int32_t          max_num_ent_cq;
  u_int32_t          max_num_mr;
  u_int64_t          max_mr_size;
  u_int32_t          max_pd_num;
  u_int32_t          page_size_cap;
  IB_port_t          phys_port_num;
  u_int16_t          max_pkeys;
  IB_guid_t          node_guid;
  VAPI_timeout_t     local_ca_ack_delay;
  u_int8_t           max_qp_ous_rd_atom;
  u_int8_t           max_ee_ous_rd_atom;
  u_int8_t           max_res_rd_atom;
  u_int8_t           max_qp_init_rd_atom;
  u_int8_t           max_ee_init_rd_atom;
  VAPI_atomic_cap_t  atomic_cap;
  u_int32_t          max_ee_num;
  u_int32_t          max_rdd_num;
  u_int32_t          max_mw_num;
  u_int32_t          max_raw_ipv6_qp;
  u_int32_t          max_raw_ethy_qp;
  u_int32_t          max_mcast_grp_num;
  u_int32_t          max_mcast_qp_attach_num;
  u_int32_t          max_total_mcast_qp_attach_num;
  u_int32_t          max_ah_num;
  u_int32_t          max_num_fmr;
  u_int32_t          max_num_map_per_fmr;
  u_int32_t          max_num_srq;
  u_int32_t          max_wqe_per_srq;
  u_int32_t          max_srq_sentries;
  MT_bool            srq_resize_supported;
} VAPI_hca_cap_t;

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

#define GET_iba_hca_cap_t__vendor(struct_name, field_name, ...) (GET_iba_hca_vendor_t__##field_name((struct_name).hca_vendor, __VA_ARGS__))
#define GET_iba_hca_cap_t__node_guid(struct_name, dummy) (*((iba_uint64_t*)((struct_name).hca_cap.node_guid)))
#define GET_iba_hca_cap_t__sys_image_guid(struct_name, dummy) ((iba_uint64_t)(0))
#define GET_iba_hca_cap_t__phys_port_num(struct_name, dummy) ((iba_uint8_t)((struct_name).hca_cap.phys_port_num))
#define GET_iba_hca_cap_t__max_mr_size(struct_name, dummy) ((iba_uint64_t)((struct_name).hca_cap.max_mr_size))
#define GET_iba_hca_cap_t__page_size_cap(struct_name, dummy) ((iba_uint64_t)((struct_name).hca_cap.page_size_cap))
#define GET_iba_hca_cap_t__flags(struct_name, field_name, ...) (GET_iba_hca_cap_t__flags__##field_name((struct_name).hca_cap, field_name, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__resize_max_wr(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__bad_pkey_counter(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__bad_qkey_counter(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__raw_multicast(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__auto_path_mig(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__change_phy_port(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__ud_av_port_enforce(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__curr_qp_state_mod(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__shutdown_port(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__init_type(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__port_active_event(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__sys_image_guid(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__rc_rnr_nak_gen(struct_name, field_name, ...) (GET_iba_hca_cap_flags_t__##field_name((struct_name).flags, __VA_ARGS__))
#define GET_iba_hca_cap_t__flags__srq_resize(struct_name, field_name, ...) ((iba_bool_t)((struct_name).srq_resize_supported))
#define GET_iba_hca_cap_t__max_qp(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_qp))
#define GET_iba_hca_cap_t__max_qp_wr(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_qp_ous_wr))
#define GET_iba_hca_cap_t__max_cq(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_cq))
#define GET_iba_hca_cap_t__max_cqe(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_ent_cq))
#define GET_iba_hca_cap_t__max_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_sg_ent))
#define GET_iba_hca_cap_t__max_sge_rd(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_sg_ent_rd))
#define GET_iba_hca_cap_t__max_mr(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_mr))
#define GET_iba_hca_cap_t__max_pd(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_pd_num))
#define GET_iba_hca_cap_t__max_pkeys(struct_name, dummy) ((iba_uint16_t)((struct_name).hca_cap.max_pkeys))
#define GET_iba_hca_cap_t__atomic_cap(struct_name, dummy) ((iba_atomic_cap_t)((struct_name).hca_cap.atomic_cap))
#define GET_iba_hca_cap_t__max_qp_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_qp_ous_rd_atom))
#define GET_iba_hca_cap_t__max_ee_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_ee_ous_rd_atom))
#define GET_iba_hca_cap_t__max_res_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_res_rd_atom))
#define GET_iba_hca_cap_t__max_qp_init_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_qp_init_rd_atom))
#define GET_iba_hca_cap_t__max_ee_init_rd_atom(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_ee_init_rd_atom))
#define GET_iba_hca_cap_t__max_ee(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_ee_num))
#define GET_iba_hca_cap_t__max_rdd(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_rdd_num))
#define GET_iba_hca_cap_t__max_mw(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_mw_num))
#define GET_iba_hca_cap_t__max_raw_ipv6_qp(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_raw_ipv6_qp))
#define GET_iba_hca_cap_t__max_raw_ethy_qp(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_raw_ethy_qp))
#define GET_iba_hca_cap_t__max_mcast_grp(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_mcast_grp_num))
#define GET_iba_hca_cap_t__max_mcast_qp_attach(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_mcast_qp_attach_num))
#define GET_iba_hca_cap_t__max_total_mcast_qp_attach(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_total_mcast_qp_attach_num))
#define GET_iba_hca_cap_t__max_ah(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_ah_num))
#define GET_iba_hca_cap_t__max_fmr(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_fmr))
#define GET_iba_hca_cap_t__max_map_per_fmr(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_map_per_fmr))
#define GET_iba_hca_cap_t__max_srq(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_num_srq))
#define GET_iba_hca_cap_t__max_srq_wr(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_wqe_per_srq))
#define GET_iba_hca_cap_t__max_srq_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).hca_cap.max_srq_sentries))
#define GET_iba_hca_cap_t__local_ca_ack_delay(struct_name, dummy) ((iba_uint8_t)((struct_name).hca_cap.local_ca_ack_delay))

/* -------------------------------------------------------------------------------- */

typedef IB_port_state_t  iba_port_state_t;

#define IBA_PORT_NOP            PORT_NOP
#define IBA_PORT_DOWN           PORT_DOWN
#define IBA_PORT_INIT           PORT_INITIALIZE
#define IBA_PORT_ARMED          PORT_ARMED
#define IBA_PORT_ACTIVE         PORT_ACTIVE
#define IBA_PORT_ACTIVE_DEFER  (PORT_ACTIVE+1)

/* -------------------------------------------------------------------------------- */

typedef u_int32_t  iba_port_cap_flags_t;   /* read only */

/*
----- INTERN -----

typedef enum {
  IB_CAP_MASK_IS_SM               = (1<<1),
  IB_CAP_MASK_IS_NOTICE_SUP       = (1<<2),
  IB_CAP_MASK_IS_TRAP_SUP         = (1<<3),
  IB_CAP_MASK_IS_AUTO_MIGR_SUP    = (1<<5),
  IB_CAP_MASK_IS_SL_MAP_SUP       = (1<<6),
  IB_CAP_MASK_IS_MKEY_NVRAM       = (1<<7),
  IB_CAP_MASK_IS_PKEY_NVRAM       = (1<<8),
  IB_CAP_MASK_IS_LED_INFO_SUP     = (1<<9),
  IB_CAP_MASK_IS_SM_DISABLED      = (1<<10),
  IB_CAP_MASK_IS_SYS_IMAGE_GUID_SUP = (1<<11),
  IB_CAP_MASK_IS_PKEY_SW_EXT_PORT_TRAP_SUP = (1<<12),
  IB_CAP_MASK_IS_CONN_MGMT_SUP    = (1<<16),
  IB_CAP_MASK_IS_SNMP_TUNN_SUP    = (1<<17),
  IB_CAP_MASK_IS_REINIT_SUP       = (1<<18),
  IB_CAP_MASK_IS_DEVICE_MGMT_SUP  = (1<<19),
  IB_CAP_MASK_IS_VENDOR_CLS_SUP   = (1<<20),
  IB_CAP_MASK_IS_DR_NOTICE_SUP    = (1<<21),
  IB_CAP_MASK_IS_CAP_MASK_NOTICE_SUP = (1<<22),
  IB_CAP_MASK_IS_BOOT_MGMT_SUP    = (1<<23)
} IB_capability_mask_bits_t; 

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

#define GET_iba_port_cap_flags_t__sm(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_SM) != 0))
#define GET_iba_port_cap_flags_t__notice_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_NOTICE_SUP) != 0))
#define GET_iba_port_cap_flags_t__trap_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_TRAP_SUP) != 0))
#define GET_iba_port_cap_flags_t__auto_migr_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_AUTO_MIGR_SUP) != 0))
#define GET_iba_port_cap_flags_t__sl_map_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_SL_MAP_SUP) != 0))
#define GET_iba_port_cap_flags_t__mkey_nvram(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_MKEY_NVRAM) != 0))
#define GET_iba_port_cap_flags_t__pkey_nvram(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_PKEY_NVRAM) != 0))
#define GET_iba_port_cap_flags_t__led_info_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_LED_INFO_SUP) != 0))
#define GET_iba_port_cap_flags_t__sm_disabled(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_SM_DISABLED) != 0))
#define GET_iba_port_cap_flags_t__sys_image_guid_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_SYS_IMAGE_GUID_SUP) != 0))
#define GET_iba_port_cap_flags_t__pkey_sw_ext_port_trap_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_PKEY_SW_EXT_PORT_TRAP_SUP) != 0))
#define GET_iba_port_cap_flags_t__conn_mgmt_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_CONN_MGMT_SUP) != 0))
#define GET_iba_port_cap_flags_t__snmp_tunnel_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_SNMP_TUNN_SUP) != 0))
#define GET_iba_port_cap_flags_t__reinit_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_REINIT_SUP) != 0))
#define GET_iba_port_cap_flags_t__device_mgmt_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_DEVICE_MGMT_SUP) != 0))
#define GET_iba_port_cap_flags_t__vendor_class_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_VENDOR_CLS_SUP) != 0))
#define GET_iba_port_cap_flags_t__dr_notice_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_DR_NOTICE_SUP) != 0))
#define GET_iba_port_cap_flags_t__cap_mask_notice_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_CAP_MASK_NOTICE_SUP) != 0))
#define GET_iba_port_cap_flags_t__boot_mgmt_sup(struct_name, dummy) ((iba_bool_t)(((struct_name) & IB_CAP_MASK_IS_BOOT_MGMT_SUP) != 0))

/* -------------------------------------------------------------------------------- */

typedef IB_mtu_t  iba_mtu_t;

#define IBA_MTU_256   MTU256
#define IBA_MTU_512   MTU512
#define IBA_MTU_1024  MTU1024
#define IBA_MTU_2048  MTU2048
#define IBA_MTU_4096  MTU4096

/* -------------------------------------------------------------------------------- */

typedef VAPI_hca_port_t  iba_hca_port_t;

/*
----- INTERN -----

typedef struct {
  IB_mtu_t            max_mtu;
  u_int32_t           max_msg_sz;
  IB_lid_t            lid;
  u_int8_t            lmc;
  IB_port_state_t     state;
  IB_port_cap_mask_t  capability_mask;
  u_int8_t            max_vl_num;
  VAPI_key_counter_t  bad_pkey_counter;
  VAPI_key_counter_t  qkey_viol_counter;
  IB_lid_t            sm_lid;
  IB_sl_t             sm_sl;
  u_int16_t           pkey_tbl_len;
  u_int16_t           gid_tbl_len;
  VAPI_timeout_t      subnet_timeout;
  u_int8_t            initTypeReply;
} VAPI_hca_port_t;

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
#define GET_iba_hca_port_t__flags(struct_name, field_name, ...) (GET_iba_port_cap_flags_t__##field_name((struct_name).capability_mask, __VA_ARGS__))
#define GET_iba_hca_port_t__max_mtu(struct_name, dummy) ((iba_mtu_t)((struct_name).max_mtu))
#define GET_iba_hca_port_t__max_msg_size(struct_name, dummy) ((iba_uint32_t)((struct_name).max_msg_sz))
#define GET_iba_hca_port_t__lid(struct_name, dummy) ((iba_uint16_t)((struct_name).lid))
#define GET_iba_hca_port_t__lmc(struct_name, dummy) ((iba_uint8_t)((struct_name).lmc))
#define GET_iba_hca_port_t__max_vl_num(struct_name, dummy) ((iba_uint8_t)((struct_name).max_vl_num))
#define GET_iba_hca_port_t__bad_pkey_counter(struct_name, dummy) ((iba_uint32_t)((struct_name).bad_pkey_counter))
#define GET_iba_hca_port_t__qkey_viol_counter(struct_name, dummy) ((iba_uint32_t)((struct_name).qkey_viol_counter))
#define GET_iba_hca_port_t__gid_tbl_len(struct_name, dummy) ((iba_uint32_t)((struct_name).gid_tbl_len))
#define GET_iba_hca_port_t__pkey_tbl_len(struct_name, dummy) ((iba_uint16_t)((struct_name).pkey_tbl_len))
#define GET_iba_hca_port_t__sm_lid(struct_name, dummy) ((iba_uint16_t)((struct_name).sm_lid))
#define GET_iba_hca_port_t__sm_sl(struct_name, dummy) ((iba_uint8_t)((struct_name).sm_sl))
#define GET_iba_hca_port_t__subnet_timeout(struct_name, dummy) ((iba_uint8_t)((struct_name).subnet_timeout))
#define GET_iba_hca_port_t__init_type_reply(struct_name, dummy) ((iba_uint8_t)((struct_name).initTypeReply))

/* -------------------------------------------------------------------------------- */

typedef VAPI_qp_state_t  iba_qp_state_t;

#define IBA_QPS_RESET  VAPI_RESET
#define IBA_QPS_INIT   VAPI_INIT
#define IBA_QPS_RTR    VAPI_RTR
#define IBA_QPS_RTS    VAPI_RTS
#define IBA_QPS_SQD    VAPI_SQD
#define IBA_QPS_SQE    VAPI_SQE
#define IBA_QPS_ERR    VAPI_ERR

/* -------------------------------------------------------------------------------- */

typedef VAPI_mig_state_t  iba_mig_state_t;

#define IBA_MIG_MIGRATED  VAPI_MIGRATED
#define IBA_MIG_REARM     VAPI_REARM
#define IBA_MIG_ARMED     VAPI_ARMED

/* -------------------------------------------------------------------------------- */

typedef VAPI_rdma_atom_acl_t  iba_rdma_atom_acl_t;

#define IBA_ENABLE_REMOTE_WRITE   VAPI_EN_REM_WRITE
#define IBA_ENABLE_REMOTE_READ    VAPI_EN_REM_READ
#define IBA_ENABLE_REMOTE_ATOMIC  VAPI_EN_REM_ATOMIC_OP

/* -------------------------------------------------------------------------------- */

typedef IB_gid_t  iba_gid_t;

/*
----- INTERN -----

typedef u_int8_t IB_gid_t[16];

----- EXTERN -----

typedef struct {
  iba_uint64_t	subnet_prefix;
  iba_uint64_t	interface_id;
} iba_gid_t;

------------------
*/

#define SET_iba_gid_t__subnet_prefix(struct_name, value) (*((u_int64_t*)(struct_name)) = (u_int64_t)(value))
#define SET_iba_gid_t__interface_id(struct_name, value) (*((u_int64_t*)((struct_name)+8)) = (u_int64_t)(value))

#define GET_iba_gid_t__subnet_prefix(struct_name, dummy) (*((iba_uint64_t*)(struct_name)))
#define GET_iba_gid_t__interface_id(struct_name, dummy) (*((iba_uint64_t*)((struct_name)+8)))

/* -------------------------------------------------------------------------------- */

typedef VAPI_ud_av_t  iba_av_t;

/*
----- INTERN -----

typedef u_int16_t IB_lid_t;
typedef u_int8_t  IB_sl_t;
typedef u_int8_t  IB_port_t;
typedef u_int8_t  IB_static_rate_t;

typedef struct {                              
  IB_gid_t            dgid;
  IB_sl_t             sl;
  IB_lid_t            dlid;
  u_int8_t            src_path_bits;
  IB_static_rate_t    static_rate;
  MT_bool             grh_flag;
  u_int8_t            traffic_class;
  u_int8_t            hop_limit;
  u_int32_t           flow_label;
  u_int8_t            sgid_index;
  IB_port_t           port;
} VAPI_ud_av_t;

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

#define SET_iba_av_t__dlid(struct_name, value) ((struct_name).dlid = (IB_lid_t)(value))
#define SET_iba_av_t__sl(struct_name, value) ((struct_name).sl = (IB_sl_t)(value))
#define SET_iba_av_t__src_path_bits(struct_name, value) ((struct_name).src_path_bits = (u_int8_t)(value))
#define SET_iba_av_t__static_rate(struct_name, value) ((struct_name).static_rate = (IB_static_rate_t)(value))
#define SET_iba_av_t__is_global(struct_name, value) ((struct_name).grh_flag = (MT_bool)(value))
#define SET_iba_av_t__dgid(struct_name, field_name, ...) (SET_iba_gid_t__##field_name((struct_name).dgid, __VA_ARGS__))
#define SET_iba_av_t__flow_label(struct_name, value) ((struct_name).flow_label = (u_int32_t)(value))
#define SET_iba_av_t__sgid_index(struct_name, value) ((struct_name).sgid_index = (u_int8_t)(value))
#define SET_iba_av_t__hop_limit(struct_name, value) ((struct_name).hop_limit = (u_int8_t)(value))
#define SET_iba_av_t__traffic_class(struct_name, value) ((struct_name).traffic_class = (u_int8_t)(value))
#define SET_iba_av_t__port(struct_name, value) ((struct_name).port = (IB_port_t)(value))


#define GET_iba_av_t__dlid(struct_name, dummy) ((iba_uint16_t)((struct_name).dlid))
#define GET_iba_av_t__sl(struct_name, dummy) ((iba_uint8_t)((struct_name).sl))
#define GET_iba_av_t__src_path_bits(struct_name, dummy) ((iba_uint8_t)((struct_name).src_path_bits))
#define GET_iba_av_t__static_rate(struct_name, dummy) ((iba_uint8_t)((struct_name).static_rate))
#define GET_iba_av_t__is_global(struct_name, dummy) ((iba_bool_t)((struct_name).grh_flag))
#define GET_iba_av_t__dgid(struct_name, field_name, ...) (GET_iba_gid_t__##field_name((struct_name).dgid, __VA_ARGS__))
#define GET_iba_av_t__flow_label(struct_name, dummy) ((iba_uint32_t)((struct_name).flow_label))
#define GET_iba_av_t__sgid_index(struct_name, dummy) ((iba_uint8_t)((struct_name).sgid_index))
#define GET_iba_av_t__hop_limit(struct_name, dummy) ((iba_uint8_t)((struct_name).hop_limit))
#define GET_iba_av_t__traffic_class(struct_name, dummy) ((iba_uint8_t)((struct_name).traffic_class))
#define GET_iba_av_t__port(struct_name, dummy) ((iba_uint8_t)((struct_name).port))

/* -------------------------------------------------------------------------------- */

typedef VAPI_qp_attr_t  iba_qp_attr_t;

/*
----- INTERN -----

typedef struct {
  VAPI_qp_state_t          qp_state;
  MT_bool                  en_sqd_asyn_notif;
  MT_bool                  sq_draining;
  VAPI_qp_num_t            qp_num;
  VAPI_rdma_atom_acl_t     remote_atomic_flags;
  VAPI_qkey_t              qkey;
  IB_mtu_t                 path_mtu;
  VAPI_mig_state_t         path_mig_state;
  VAPI_psn_t               rq_psn;
  VAPI_psn_t               sq_psn;
  u_int8_t                 qp_ous_rd_atom;
  u_int8_t                 ous_dst_rd_atom;
  IB_rnr_nak_timer_code_t  min_rnr_timer;
  VAPI_qp_cap_t            cap;
  VAPI_qp_num_t            dest_qp_num;
  VAPI_sched_queue_t       sched_queue;
  VAPI_pkey_ix_t           pkey_ix;
  IB_port_t                port;
  VAPI_ud_av_t             av;
  VAPI_timeout_t           timeout;
  VAPI_retry_count_t       retry_count;
  VAPI_retry_count_t       rnr_retry;
  VAPI_pkey_ix_t           alt_pkey_ix;
  IB_port_t                alt_port;
  VAPI_ud_av_t             alt_av;
  VAPI_timeout_t           alt_timeout;
} VAPI_qp_attr_t;

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

#define SET_iba_qp_attr_t__qp_state(struct_name, value) ((struct_name).qp_state = (VAPI_qp_state_t)(value))
#define SET_iba_qp_attr_t__path_mtu(struct_name, value) ((struct_name).path_mtu = (IB_mtu_t)(value))
#define SET_iba_qp_attr_t__path_mig_state(struct_name, value) ((struct_name).path_mig_state = (VAPI_mig_state_t)(value))
#define SET_iba_qp_attr_t__qkey(struct_name, value) ((struct_name).qkey = (VAPI_qkey_t)(value))
#define SET_iba_qp_attr_t__rq_psn(struct_name, value) ((struct_name).rq_psn = (VAPI_psn_t)(value))
#define SET_iba_qp_attr_t__sq_psn(struct_name, value) ((struct_name).sq_psn = (VAPI_psn_t)(value))
#define SET_iba_qp_attr_t__dest_qp_num(struct_name, value) ((struct_name).dest_qp_num = (VAPI_qp_num_t)(value))
#define SET_iba_qp_attr_t__remote_atomic_flags(struct_name, value) ((struct_name).remote_atomic_flags = (VAPI_rdma_atom_acl_t)(value))
#define SET_iba_qp_attr_t__cap(struct_name, field_name, ...) (SET_iba_qp_cap_t__##field_name((struct_name).cap, __VA_ARGS__))
#define SET_iba_qp_attr_t__en_sqd_async_notify(struct_name, value) ((struct_name).en_sqd_asyn_notif = (MT_bool)(value))
/* sq_draining read only */
#define SET_iba_qp_attr_t__max_rd_atomic(struct_name, value) ((struct_name).qp_ous_rd_atom = (u_int8_t)(value))
#define SET_iba_qp_attr_t__max_dest_rd_atomic(struct_name, value) ((struct_name).ous_dst_rd_atom = (u_int8_t)(value))
#define SET_iba_qp_attr_t__min_rnr_timer(struct_name, value) ((struct_name).min_rnr_timer = (IB_rnr_nak_timer_code_t)(value))
#define SET_iba_qp_attr_t__pkey_index(struct_name, value) ((struct_name).pkey_ix = (VAPI_pkey_ix_t)(value))
#define SET_iba_qp_attr_t__port(struct_name, value) ((struct_name).port = (IB_port_t)(value))
#define SET_iba_qp_attr_t__av(struct_name, field_name, ...) (SET_iba_av_t__##field_name((struct_name).av, __VA_ARGS__))
#define SET_iba_qp_attr_t__timeout(struct_name, value) ((struct_name).timeout = (VAPI_timeout_t)(value))
#define SET_iba_qp_attr_t__retry_count(struct_name, value) ((struct_name).retry_count = (VAPI_retry_count_t)(value))
#define SET_iba_qp_attr_t__rnr_retry(struct_name, value) ((struct_name).rnr_retry = (VAPI_retry_count_t)(value))
#define SET_iba_qp_attr_t__alt_pkey_index(struct_name, value) ((struct_name).alt_pkey_ix = (VAPI_pkey_ix_t)(value))
#define SET_iba_qp_attr_t__alt_port(struct_name, value) ((struct_name).alt_port = (IB_port_t)(value))
#define SET_iba_qp_attr_t__alt_av(struct_name, field_name, ...) (SET_iba_av_t__##field_name((struct_name).alt_av, __VA_ARGS__))
#define SET_iba_qp_attr_t__alt_timeout(struct_name, value) ((struct_name).alt_timeout = (VAPI_timeout_t)(value))


#define GET_iba_qp_attr_t__qp_state(struct_name, dummy) ((iba_qp_state_t)((struct_name).qp_state))
#define GET_iba_qp_attr_t__path_mtu(struct_name, dummy) ((iba_mtu_t)((struct_name).path_mtu))
#define GET_iba_qp_attr_t__path_mig_state(struct_name, dummy) ((iba_mig_state_t)((struct_name).path_mig_state))
#define GET_iba_qp_attr_t__qkey(struct_name, dummy) ((iba_uint32_t)((struct_name).qkey))
#define GET_iba_qp_attr_t__rq_psn(struct_name, dummy) ((iba_uint32_t)((struct_name).rq_psn))
#define GET_iba_qp_attr_t__sq_psn(struct_name, dummy) ((iba_uint32_t)((struct_name).sq_psn))
#define GET_iba_qp_attr_t__dest_qp_num(struct_name, dummy) ((iba_uint32_t)((struct_name).dest_qp_num))
#define GET_iba_qp_attr_t__remote_atomic_flags(struct_name, dummy) ((iba_rdma_atom_acl_t)((struct_name).remote_atomic_flags))
#define GET_iba_qp_attr_t__cap(struct_name, field_name, ...) (GET_iba_qp_cap_t__##field_name((struct_name).cap, __VA_ARGS__))
#define GET_iba_qp_attr_t__en_sqd_async_notify(struct_name, dummy) ((iba_bool_t)((struct_name).en_sqd_asyn_notif))
#define GET_iba_qp_attr_t__sq_draining(struct_name, dummy) ((iba_bool_t)((struct_name).sq_draining))
#define GET_iba_qp_attr_t__max_rd_atomic(struct_name, dummy) ((iba_uint8_t)((struct_name).qp_ous_rd_atom))
#define GET_iba_qp_attr_t__max_dest_rd_atomic(struct_name, dummy) ((iba_uint8_t)((struct_name).ous_dst_rd_atom))
#define GET_iba_qp_attr_t__min_rnr_timer(struct_name, dummy) ((iba_uint8_t)((struct_name).min_rnr_timer))
#define GET_iba_qp_attr_t__pkey_index(struct_name, dummy) ((iba_uint16_t)((struct_name).pkey_ix))
#define GET_iba_qp_attr_t__port(struct_name, dummy) ((iba_uint8_t)((struct_name).port))
#define GET_iba_qp_attr_t__av(struct_name, field_name, ...) (GET_iba_av_t__##field_name((struct_name).av, __VA_ARGS__))
#define GET_iba_qp_attr_t__timeout(struct_name, dummy) ((iba_uint8_t)((struct_name).timeout))
#define GET_iba_qp_attr_t__retry_count(struct_name, dummy) ((iba_uint8_t)((struct_name).retry_count))
#define GET_iba_qp_attr_t__rnr_retry(struct_name, dummy) ((iba_uint8_t)((struct_name).rnr_retry))
#define GET_iba_qp_attr_t__alt_pkey_index(struct_name, dummy) ((iba_uint16_t)((struct_name).alt_pkey_ix))
#define GET_iba_qp_attr_t__alt_port(struct_name, dummy) ((iba_uint8_t)((struct_name).alt_port))
#define GET_iba_qp_attr_t__alt_av(struct_name, field_name, ...) (GET_iba_av_t__##field_name((struct_name).alt_av, __VA_ARGS__))
#define GET_iba_qp_attr_t__alt_timeout(struct_name, dummy) ((iba_uint8_t)((struct_name).alt_timeout))

/* -------------------------------------------------------------------------------- */

typedef iba_uint32_t  iba_qp_attr_mask_t;

#define IBA_QP_STATE                QP_ATTR_QP_STATE
#define IBA_QP_PATH_MTU             QP_ATTR_PATH_MTU
#define IBA_QP_PATH_MIG_STATE       QP_ATTR_PATH_MIG_STATE
#define IBA_QP_QKEY                 QP_ATTR_QKEY
#define IBA_QP_RQ_PSN               QP_ATTR_RQ_PSN
#define IBA_QP_SQ_PSN               QP_ATTR_SQ_PSN
#define IBA_QP_DEST_QP_NUM          QP_ATTR_DEST_QP_NUM
#define IBA_QP_REMOTE_ATOMIC_FLAGS  QP_ATTR_REMOTE_ATOMIC_FLAGS
#define IBA_QP_CAP                  QP_ATTR_CAP
#define IBA_QP_EN_SQD_ASYNC_NOTIFY  QP_ATTR_EN_SQD_ASYN_NOTIF
#define IBA_QP_MAX_RD_ATOMIC        QP_ATTR_QP_OUS_RD_ATOM
#define IBA_QP_MAX_DEST_RD_ATOMIC   QP_ATTR_OUS_DST_RD_ATOM
#define IBA_QP_MIN_RNR_TIMER        QP_ATTR_MIN_RNR_TIMER
#define IBA_QP_PKEY_INDEX           QP_ATTR_PKEY_IX
#define IBA_QP_PORT                 QP_ATTR_PORT
#define IBA_QP_AV                   QP_ATTR_AV
#define IBA_QP_TIMEOUT              QP_ATTR_TIMEOUT
#define IBA_QP_RETRY_COUNT          QP_ATTR_RETRY_COUNT
#define IBA_QP_RNR_RETRY            QP_ATTR_RNR_RETRY
#define IBA_QP_ALT_PATH             QP_ATTR_ALT_PATH

#define IBA_QP_ATTR_MASK_CLR_ALL(mask)  ((mask)=0)
#define IBA_QP_ATTR_MASK_SET_ALL(mask)  ((mask)=(0x07E07FFB))
#define IBA_QP_ATTR_MASK_SET(mask,attr) ((mask)=((mask)|(attr)))
#define IBA_QP_ATTR_MASK_CLR(mask,attr) ((mask)=((mask)&(~(attr))))
#define IBA_QP_ATTR_MASK_IS_SET(mask,attr) (((mask)&(attr))!=0)

/* -------------------------------------------------------------------------------- */

typedef VAPI_mr_hndl_t  iba_mr_hndl_t;

/* -------------------------------------------------------------------------------- */

typedef VAPI_mrw_acl_t  iba_mr_acl_t;

#define IBA_ACCESS_LOCAL_WRITE    VAPI_EN_LOCAL_WRITE
#define IBA_ACCESS_REMOTE_WRITE   VAPI_EN_REMOTE_WRITE
#define IBA_ACCESS_REMOTE_READ    VAPI_EN_REMOTE_READ
#define IBA_ACCESS_REMOTE_ATOMIC  VAPI_EN_REMOTE_ATOM
#define IBA_ACCESS_MW_BIND        VAPI_EN_MEMREG_BIND

/* -------------------------------------------------------------------------------- */

typedef VAPI_mr_t  iba_mr_attr_t;

/*
----- INTERN -----

typedef struct {
  VAPI_mrw_type_t     type;
  VAPI_lkey_t         l_key; 
  VAPI_rkey_t         r_key;
  VAPI_virt_addr_t    start;
  VAPI_size_t         size;
  VAPI_pd_hndl_t      pd_hndl;
  VAPI_mrw_acl_t      acl; 
  MT_size_t           pbuf_list_len;
  VAPI_phy_buf_t      *pbuf_list_p;
  VAPI_phy_addr_t     iova_offset;
} VAPI_mr_t;

typedef VAPI_mr_t  VAPI_mrw_t;

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

#define SET_iba_mr_attr_t__addr(struct_name, value) ((struct_name).start = (VAPI_virt_addr_t)(value))
#define SET_iba_mr_attr_t__size(struct_name, value) ((struct_name).size = (VAPI_size_t)(value))
#define SET_iba_mr_attr_t__pd_hndl(struct_name, value) { (struct_name).pd_hndl = (VAPI_pd_hndl_t)(value); (struct_name).type = VAPI_MR; }
#define SET_iba_mr_attr_t__acl(struct_name, value) ((struct_name).acl = (VAPI_mrw_acl_t)(value))
#define SET_iba_mr_attr_t__lkey(struct_name, value) ((struct_name).l_key = (VAPI_lkey_t)(value))
#define SET_iba_mr_attr_t__rkey(struct_name, value) ((struct_name).r_key = (VAPI_rkey_t)(value))

#define GET_iba_mr_attr_t__addr(struct_name, dummy) ((iba_uint64_t)((struct_name).start))
#define GET_iba_mr_attr_t__size(struct_name, dummy) ((iba_uint64_t)((struct_name).size))
#define GET_iba_mr_attr_t__pd_hndl(struct_name, dummy) ((iba_pd_hndl_t)((struct_name).pd_hndl))
#define GET_iba_mr_attr_t__acl(struct_name, dummy) ((iba_mr_acl_t)((struct_name).acl))
#define GET_iba_mr_attr_t__lkey(struct_name, dummy) ((iba_uint32_t)((struct_name).l_key))
#define GET_iba_mr_attr_t__rkey(struct_name, dummy) ((iba_uint32_t)((struct_name).r_key))

/* -------------------------------------------------------------------------------- */

typedef VAPI_wr_opcode_t  iba_wr_opcode_t;

#define IBA_WR_RDMA_WRITE            VAPI_RDMA_WRITE
#define IBA_WR_RDMA_WRITE_WITH_IMM   VAPI_RDMA_WRITE_WITH_IMM
#define IBA_WR_SEND                  VAPI_SEND
#define IBA_WR_SEND_WITH_IMM         VAPI_SEND_WITH_IMM
#define IBA_WR_RDMA_READ             VAPI_RDMA_READ
#define IBA_WR_ATOMIC_CMP_AND_SWP    VAPI_ATOMIC_CMP_AND_SWP
#define IBA_WR_ATOMIC_FETCH_AND_ADD  VAPI_ATOMIC_FETCH_AND_ADD

/* -------------------------------------------------------------------------------- */

typedef int  iba_send_flags_t;

#define IBA_SEND_FENCE      1
#define IBA_SEND_SIGNALED  (1 << 1)
#define IBA_SEND_SOLICITED (1 << 2)

/* -------------------------------------------------------------------------------- */

typedef VAPI_sg_lst_entry_t  iba_sge_t;

/*
----- INTERN -----

typedef IB_virt_addr_t  VAPI_virt_addr_t;
typedef u_int64_t       IB_virt_addr_t;
typedef u_int32_t       VAPI_lkey_t;

typedef struct {
  VAPI_virt_addr_t    addr;
  u_int32_t           len;
  VAPI_lkey_t         lkey;
} VAPI_sg_lst_entry_t;

----- EXTERN -----

typedef struct {
  iba_uint64_t   addr;
  iba_uint32_t   length;
  iba_uint32_t   lkey;
} iba_sge_t;

------------------
*/

#define SET_iba_sge_t__addr(struct_name, value) ((struct_name).addr = (VAPI_virt_addr_t)(value))
#define SET_iba_sge_t__length(struct_name, value) ((struct_name).len = (u_int32_t)(value))
#define SET_iba_sge_t__lkey(struct_name, value) ((struct_name).lkey = (VAPI_lkey_t)(value))

#define GET_iba_sge_t__addr(struct_name, dummy) ((iba_uint64_t)((struct_name).addr))
#define GET_iba_sge_t__length(struct_name, dummy) ((iba_uint32_t)((struct_name).len))
#define GET_iba_sge_t__lkey(struct_name, dummy) ((iba_uint32_t)((struct_name).lkey))

/* -------------------------------------------------------------------------------- */

typedef VAPI_sr_desc_t  iba_send_wr_t;

/*
----- INTERN -----

typedef struct {
  VAPI_wr_id_t         id;
  VAPI_wr_opcode_t     opcode;
  VAPI_comp_type_t     comp_type;
  VAPI_sg_lst_entry_t *sg_lst_p;
  u_int32_t            sg_lst_len;
  VAPI_imm_data_t      imm_data;
  MT_bool              fence;
  VAPI_ud_av_hndl_t    remote_ah;
  VAPI_qp_num_t        remote_qp;
  VAPI_qkey_t          remote_qkey;
  VAPI_ethertype_t     ethertype;
  IB_eecn_t            eecn;
  MT_bool              set_se;
  VAPI_virt_addr_t     remote_addr;
  VAPI_rkey_t          r_key;
  u_int64_t            compare_add;
  u_int64_t            swap;
} VAPI_sr_desc_t;

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

#define SET_iba_send_wr_t__id(struct_name, value) ((struct_name).id = (VAPI_wr_id_t)(value))
#define SET_iba_send_wr_t__opcode(struct_name, value) ((struct_name).opcode = (VAPI_wr_opcode_t)(value))
#define SET_iba_send_wr_t__send_flags(struct_name, value) { \
	(struct_name).fence = (MT_bool)(((value) & IBA_SEND_FENCE) != 0); \
	(struct_name).comp_type = ((((value) & IBA_SEND_SIGNALED) != 0) ? VAPI_SIGNALED : VAPI_UNSIGNALED); \
	(struct_name).set_se = (MT_bool)(((value) & IBA_SEND_SOLICITED) != 0); \
}
#define SET_iba_send_wr_t__sg_list(struct_name, value) ((struct_name).sg_lst_p = (VAPI_sg_lst_entry_t*)(value))
#define SET_iba_send_wr_t__num_sge(struct_name, value) ((struct_name).sg_lst_len = (u_int32_t)(value))
#define SET_iba_send_wr_t__imm_data(struct_name, value) ((struct_name).imm_data = (VAPI_imm_data_t)(value))
#define SET_iba_send_wr_t__wr(struct_name, op, field_name, value) (SET_iba_send_wr_t__wr__##op##__##field_name(struct_name, value))
#define SET_iba_send_wr_t__wr__rdma__remote_addr(struct_name, value) ((struct_name).remote_addr = (VAPI_virt_addr_t)(value))
#define SET_iba_send_wr_t__wr__rdma__rkey(struct_name, value) ((struct_name).r_key = (VAPI_rkey_t)(value))
#define SET_iba_send_wr_t__wr__atomic__remote_addr(struct_name, value) ((struct_name).remote_addr = (VAPI_virt_addr_t)(value))
#define SET_iba_send_wr_t__wr__atomic__compare_add(struct_name, value) ((struct_name).compare_add = (u_int64_t)(value))
#define SET_iba_send_wr_t__wr__atomic__swap(struct_name, value) ((struct_name).swap = (u_int64_t)(value))
#define SET_iba_send_wr_t__wr__atomic__rkey(struct_name, value) ((struct_name).r_key = (VAPI_rkey_t)(value))
#define SET_iba_send_wr_t__wr__ud__remote_ah(struct_name, value) ((struct_name).remote_ah = (VAPI_ud_av_hndl_t)(value))
#define SET_iba_send_wr_t__wr__ud__remote_qp_num(struct_name, value) ((struct_name).remote_qp = (VAPI_qp_num_t)(value))
#define SET_iba_send_wr_t__wr__ud__remote_qkey(struct_name, value) ((struct_name).remote_qkey = (VAPI_qkey_t)(value))

#define GET_iba_send_wr_t__id(struct_name, dummy) ((iba_uint64_t)((struct_name).id))
#define GET_iba_send_wr_t__opcode(struct_name, dummy) ((iba_wr_opcode_t)((struct_name).opcode))
#define GET_iba_send_wr_t__send_flags(struct_name, dummy) (((struct_name).fence ? IBA_SEND_FENCE : 0) | ((struct_name).comp_type == VAPI_SIGNALED ? IBA_SEND_SIGNALED : 0) | ((struct_name).set_se ? IBA_SEND_SOLICITED : 0))
#define GET_iba_send_wr_t__sg_list(struct_name, dummy) ((iba_sge_t*)((struct_name).sg_lst_p))
#define GET_iba_send_wr_t__num_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).sg_lst_len))
#define GET_iba_send_wr_t__imm_data(struct_name, dummy) ((iba_uint32_t)((struct_name).imm_data))
#define GET_iba_send_wr_t__wr(struct_name, op, field_name) (GET_iba_send_wr_t__wr__##op##__##field_name(struct_name))
#define GET_iba_send_wr_t__wr__rdma__remote_addr(struct_name) ((iba_uint64_t)((struct_name).remote_addr))
#define GET_iba_send_wr_t__wr__rdma__rkey(struct_name) ((iba_uint32_t)((struct_name).r_key))
#define GET_iba_send_wr_t__wr__atomic__remote_addr(struct_name) ((iba_uint64_t)((struct_name).remote_addr))
#define GET_iba_send_wr_t__wr__atomic__compare_add(struct_name) ((iba_uint64_t)((struct_name).compare_add))
#define GET_iba_send_wr_t__wr__atomic__swap(struct_name) ((iba_uint64_t)((struct_name).swap))
#define GET_iba_send_wr_t__wr__atomic__rkey(struct_name) ((iba_uint32_t)((struct_name).r_key))
#define GET_iba_send_wr_t__wr__ud__remote_ah(struct_name) ((iba_av_hndl_t)((struct_name).remote_ah))
#define GET_iba_send_wr_t__wr__ud__remote_qp_num(struct_name) ((iba_uint32_t)((struct_name).remote_qp))
#define GET_iba_send_wr_t__wr__ud__remote_qkey(struct_name) ((iba_uint32_t)((struct_name).remote_qkey))

/* -------------------------------------------------------------------------------- */

typedef VAPI_rr_desc_t  iba_recv_wr_t;

/*
----- INTERN -----

typedef struct {
  VAPI_wr_id_t          id;
  VAPI_wr_opcode_t      opcode;
  VAPI_comp_type_t      comp_type;
  VAPI_sg_lst_entry_t  *sg_lst_p;
  u_int32_t             sg_lst_len;
} VAPI_rr_desc_t;

----- EXTERN -----

typedef struct {
  iba_uint64_t   id;
  iba_sge_t     *sg_list;
  iba_uint32_t   num_sge;
} iba_recv_wr_t;

------------------
*/

#define SET_iba_recv_wr_t__id(struct_name, value) ((struct_name).id = (VAPI_wr_id_t)(value))
#define SET_iba_recv_wr_t__sg_list(struct_name, value) ((struct_name).sg_lst_p = (VAPI_sg_lst_entry_t*)(value))
#define SET_iba_recv_wr_t__num_sge(struct_name, value) { (struct_name).sg_lst_len = (u_int32_t)(value); (struct_name).opcode = VAPI_RECEIVE; }

#define GET_iba_recv_wr_t__id(struct_name, dummy) ((iba_uint64_t)((struct_name).id))
#define GET_iba_recv_wr_t__sg_list(struct_name, dummy) ((iba_sge_t*)((struct_name).sg_lst_p))
#define GET_iba_recv_wr_t__num_sge(struct_name, dummy) ((iba_uint32_t)((struct_name).sg_lst_len))

/* -------------------------------------------------------------------------------- */

typedef VAPI_cqe_opcode_t  iba_wc_opcode_t;

#define IBA_WC_SQ_SEND           VAPI_CQE_SQ_SEND_DATA
#define IBA_WC_SQ_RDMA_WRITE     VAPI_CQE_SQ_RDMA_WRITE
#define IBA_WC_SQ_RDMA_READ      VAPI_CQE_SQ_RDMA_READ
#define IBA_WC_SQ_COMP_SWAP      VAPI_CQE_SQ_COMP_SWAP
#define IBA_WC_SQ_FETCH_ADD      VAPI_CQE_SQ_FETCH_ADD
#define IBA_WC_SQ_BIND_MW        VAPI_CQE_SQ_BIND_MRW
#define IBA_WC_RQ_SEND           VAPI_CQE_RQ_SEND_DATA
#define IBA_WC_RQ_RDMA_WITH_IMM  VAPI_CQE_RQ_RDMA_WITH_IMM

/* -------------------------------------------------------------------------------- */

typedef VAPI_wc_status_t  iba_wc_status_t;

#define IBA_WC_SUCCESS             VAPI_SUCCESS
#define IBA_WC_LOC_LEN_ERR         VAPI_LOC_LEN_ERR
#define IBA_WC_LOC_QP_OP_ERR       VAPI_LOC_QP_OP_ERR
#define IBA_WC_LOC_EEC_OP_ERR      VAPI_LOC_EE_OP_ERR
#define IBA_WC_LOC_PROT_ERR        VAPI_LOC_PROT_ERR
#define IBA_WC_WR_FLUSH_ERR        VAPI_WR_FLUSH_ERR
#define IBA_WC_MW_BIND_ERR         VAPI_MW_BIND_ERR
#define IBA_WC_BAD_RESP_ERR        VAPI_BAD_RESP_ERR
#define IBA_WC_LOC_ACCESS_ERR      VAPI_LOC_ACCS_ERR
#define IBA_WC_REM_INV_REQ_ERR     VAPI_REM_INV_REQ_ERR
#define IBA_WC_REM_ACCESS_ERR      VAPI_REM_ACCESS_ERR
#define IBA_WC_REM_OP_ERR          VAPI_REM_OP_ERR
#define IBA_WC_RETRY_EXC_ERR       VAPI_RETRY_EXC_ERR
#define IBA_WC_RNR_RETRY_EXC_ERR   VAPI_RNR_RETRY_EXC_ERR
#define IBA_WC_LOC_RDD_VIOL_ERR    VAPI_LOC_RDD_VIOL_ERR
#define IBA_WC_REM_INV_RD_REQ_ERR  VAPI_REM_INV_RD_REQ_ERR
#define IBA_WC_REM_ABORT_ERR       VAPI_REM_ABORT_ERR
#define IBA_WC_INV_EECN_ERR        VAPI_INV_EECN_ERR
#define IBA_WC_INV_EEC_STATE_ERR   VAPI_INV_EEC_STATE_ERR
#define IBA_WC_FATAL_ERR           VAPI_COMP_FATAL_ERR
#define IBA_WC_GENERAL_ERR         VAPI_COMP_GENERAL_ERR

/* -------------------------------------------------------------------------------- */

typedef VAPI_wc_desc_t  iba_wc_t;   /* read only */

/*
----- INTERN -----

typedef struct {
  VAPI_remote_node_addr_type_t  type;
  IB_lid_t                      slid;
  IB_sl_t                       sl;
  union {
    VAPI_qp_num_t      qp;
    VAPI_ethertype_t   ety;
  } qp_ety;
  union {
    VAPI_eec_num_t     loc_eecn;
    u_int8_t           dst_path_bits;
  } ee_dlid;
} VAPI_remote_node_addr_t;

typedef struct {
  VAPI_wc_status_t             status;
  VAPI_wr_id_t                 id;
  IB_wqpn_t                    local_qp_num;
  VAPI_cqe_opcode_t            opcode;
  u_int32_t                    byte_len;
  MT_bool                      imm_data_valid;
  VAPI_imm_data_t              imm_data;
  VAPI_remote_node_addr_t      remote_node_addr;
  MT_bool                      grh_flag;
  VAPI_pkey_ix_t               pkey_ix;
  EVAPI_vendor_err_syndrome_t  vendor_err_syndrome;
  u_int32_t                    free_res_count;
} VAPI_wc_desc_t;

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

#define GET_iba_wc_t__id(struct_name, dummy) ((iba_uint64_t)((struct_name).id))
#define GET_iba_wc_t__opcode(struct_name, dummy) ((iba_wc_opcode_t)((struct_name).opcode))
#define GET_iba_wc_t__status(struct_name, dummy) ((iba_wc_status_t)((struct_name).status))
#define GET_iba_wc_t__vendor_err(struct_name, dummy) ((iba_uint32_t)((struct_name).vendor_err_syndrome))
#define GET_iba_wc_t__qp_num(struct_name, dummy) ((iba_uint32_t)((struct_name).local_qp_num))
#define GET_iba_wc_t__byte_len(struct_name, dummy) ((iba_uint32_t)((struct_name).byte_len))
#define GET_iba_wc_t__imm_data(struct_name, dummy) ((iba_uint32_t)((struct_name).imm_data))
#define GET_iba_wc_t__imm_data_valid(struct_name, dummy) ((iba_bool_t)((struct_name).imm_data_valid))
#define GET_iba_wc_t__grh_flag(struct_name, dummy) ((iba_bool_t)((struct_name).grh_flag))
#define GET_iba_wc_t__pkey_index(struct_name, dummy) ((iba_uint16_t)((struct_name).pkey_ix))
#define GET_iba_wc_t__ud(struct_name, field_name) (GET_iba_wc_t__ud__##field_name(struct_name))
#define GET_iba_wc_t__ud__qp(struct_name) ((iba_uint32_t)((struct_name).remote_node_addr.qp_ety.qp))
#define GET_iba_wc_t__ud__slid(struct_name) ((iba_uint16_t)((struct_name).remote_node_addr.slid))
#define GET_iba_wc_t__ud__sl(struct_name) ((iba_uint8_t)((struct_name).remote_node_addr.sl))
#define GET_iba_wc_t__ud__dlid_path_bits(struct_name) ((iba_uint8_t)((struct_name).remote_node_addr.ee_dlid.dst_path_bits))

/* -------------------------------------------------------------------------------- */






/* -------------------------------------------------------------------------------- */
/* FUNCTIONS                                                                        */
/* -------------------------------------------------------------------------------- */


/*
 *  iba_get_hcas - Get HCA List
 */

static iba_status_t iba_get_hcas(iba_uint32_t num_of_entries, iba_device_hndl_t *devices_p, iba_uint32_t *ret_num_of_entries_p)
{
  VAPI_ret_t ret;
  if (num_of_entries == 0)
    ret = EVAPI_list_hcas(0, (u_int32_t*)ret_num_of_entries_p, NULL);
  else
    ret = EVAPI_list_hcas((u_int32_t)num_of_entries, (u_int32_t*)ret_num_of_entries_p, (VAPI_hca_id_t*)devices_p);
  switch (ret)
  {
    case VAPI_OK:
      return IBA_OK;
    case VAPI_EAGAIN:
      return IBA_EAGAIN;
    default:
      return IBA_ERROR;
  }	
}


/*
 *  iba_query_gid_table - Get GID Table
 */

static iba_status_t iba_query_gid_table(iba_hca_hndl_t hca_hndl, iba_uint8_t port_num, iba_uint16_t table_len_in, iba_uint16_t *table_len_out_p, iba_gid_t *gid_table_p)
{
  VAPI_ret_t ret;
  if (table_len_in == 0)
    ret = VAPI_query_hca_gid_tbl((VAPI_hca_hndl_t)hca_hndl, (IB_port_t)port_num, 0, (u_int16_t*)table_len_out_p, NULL);
  else    
    ret = VAPI_query_hca_gid_tbl((VAPI_hca_hndl_t)hca_hndl, (IB_port_t)port_num, (u_int16_t)table_len_in, (u_int16_t*)table_len_out_p, (IB_gid_t*)gid_table_p);
  switch (ret)
  {
    case VAPI_OK:
      return IBA_OK;
    case VAPI_EAGAIN:
      return IBA_EAGAIN;
    default:
      return IBA_ERROR;
  }	
}


/*
 *  iba_query_pkey_table - Get PKEY Table
 */

static iba_status_t iba_query_pkey_table(iba_hca_hndl_t hca_hndl, iba_uint8_t port_num, iba_uint16_t table_len_in, iba_uint16_t *table_len_out_p, iba_pkey_t *pkey_table_p)
{
  VAPI_ret_t ret;
  if (table_len_in == 0)
    ret = VAPI_query_hca_pkey_tbl((VAPI_hca_hndl_t)hca_hndl, (IB_port_t)port_num, 0, (u_int16_t*)table_len_out_p, NULL);
  else    
    ret = VAPI_query_hca_pkey_tbl((VAPI_hca_hndl_t)hca_hndl, (IB_port_t)port_num, (u_int16_t)table_len_in, (u_int16_t*)table_len_out_p, (VAPI_pkey_t*)pkey_table_p);
  switch (ret)
  {
    case VAPI_OK:
      return IBA_OK;
    case VAPI_EAGAIN:
      return IBA_EAGAIN;
    default:
      return IBA_ERROR;
  }	
}


/*
 *  iba_get_hca_hndl - Initialize HCA
 */

#define iba_get_hca_hndl(device_hndl, hca_hndl_p) (           \
  RES1( EVAPI_get_hca_hndl( (char*)(device_hndl),             \
                            (VAPI_hca_hndl_t*)(hca_hndl_p))))


/*
 *  iba_release_hca_hndl - Release HCA
 */

#define iba_release_hca_hndl(hca_hndl) ( \
  RES1( EVAPI_release_hca_hndl( (VAPI_hca_hndl_t)(hca_hndl))))


/*
 *  iba_query_hca - Get HCA Properties
 */

#define iba_query_hca(hca_hndl, hca_cap_p) (             \
  RES1( VAPI_query_hca_cap( (VAPI_hca_hndl_t)(hca_hndl), \
                            &((hca_cap_p)->hca_vendor),  \
                            &((hca_cap_p)->hca_cap))))


/*
 *  iba_query_hca - Get HCA Port Properties
 */

#define iba_query_hca_port(hca_hndl, port_num, hca_port_p) (        \
  RES1( VAPI_query_hca_port_prop( (VAPI_hca_hndl_t)(hca_hndl),      \
                                  (IB_port_t)(port_num),            \
                                  (VAPI_hca_port_t*)(hca_port_p))))


/*
 *  iba_alloc_pd - Allocate Protection Domain
 */

#define iba_alloc_pd(hca_hndl, pd_hndl_p) (           \
  RES1( VAPI_alloc_pd( (VAPI_hca_hndl_t)(hca_hndl),   \
                      (VAPI_pd_hndl_t*)(pd_hndl_p))))


/*
 *  iba_dealloc_pd - Free Protection Domain
 */

#define iba_dealloc_pd(hca_hndl, pd_hndl) (           \
  RES1( VAPI_dealloc_pd( (VAPI_hca_hndl_t)(hca_hndl), \
                         (VAPI_pd_hndl_t)(pd_hndl))))


/*
 *  iba_create_cq - Create Completion Queue
 */

#define iba_create_cq(hca_hndl, cqe, cq_hndl_p, ret_cqe_p) ( \
  RES1( VAPI_create_cq( (VAPI_hca_hndl_t)(hca_hndl),         \
                        (VAPI_cqe_num_t)(cqe),               \
                        (VAPI_cq_hndl_t*)(cq_hndl_p),        \
                        (VAPI_cqe_num_t*)(ret_cqe_p))))


/*
 *  iba_destroy_cq - Destroy Completion Queue
 */

#define iba_destroy_cq(hca_hndl, cq_hndl) (           \
  RES1( VAPI_destroy_cq( (VAPI_hca_hndl_t)(hca_hndl), \
                         (VAPI_cq_hndl_t)(cq_hndl))))


/*
 *  iba_set_priv_context4cq - Set Completion Queue Private Context
 */

#define iba_set_priv_context4cq(hca_hndl, cq_hndl, priv_context) ( \
  RES1( EVAPI_set_priv_context4cq( (VAPI_hca_hndl_t)(hca_hndl),    \
                                   (VAPI_cq_hndl_t)(cq_hndl),      \
                                   (void*)(priv_context))))


/*
 *  iba_get_priv_context4cq - Get Completion Queue Private Context
 */

#define iba_get_priv_context4cq(hca_hndl, cq_hndl, priv_context_p) ( \
  RES1( EVAPI_get_priv_context4cq( (VAPI_hca_hndl_t)(hca_hndl),      \
                                   (VAPI_cq_hndl_t)(cq_hndl),        \
                                   (void**)(priv_context_p))))


/* 
 *  iba_create_qp - Create Queue Pair 
 */

#define iba_create_qp(hca_hndl, qp_init_attr_p, qp_hndl_p, qp_prop_p) ( \
  RES1( VAPI_create_qp_ext( (VAPI_hca_hndl_t)(hca_hndl),                \
                           &((qp_init_attr_p)->vapi_init_attr),         \
                           &((qp_init_attr_p)->vapi_init_attr_ext),     \
                           (VAPI_qp_hndl_t*)(qp_hndl_p),                \
                           (VAPI_qp_prop_t*)(qp_prop_p))))


/*
 *  iba_destroy_qp - Destroy Queue Pair
 */

#define iba_destroy_qp(hca_hndl, qp_hndl) (           \
  RES1( VAPI_destroy_qp( (VAPI_hca_hndl_t)(hca_hndl), \
                        (VAPI_qp_hndl_t)(qp_hndl))))


/*
 *  iba_modify_qp - Modify Queue Pair
 */

static iba_status_t iba_modify_qp(iba_hca_hndl_t hca_hndl, iba_qp_hndl_t qp_hndl, iba_qp_attr_t *qp_attr_p, iba_qp_attr_mask_t *qp_attr_mask_p)
{
  VAPI_qp_cap_t  cap;
  return RES1(VAPI_modify_qp((VAPI_hca_hndl_t)(hca_hndl), (VAPI_qp_hndl_t)(qp_hndl), (VAPI_qp_attr_t*)(qp_attr_p), (VAPI_qp_attr_mask_t*)(qp_attr_mask_p), &cap));
}


/*
 *  iba_set_priv_context4qp - Set Queue Pair Private Context
 */

#define iba_set_priv_context4qp(hca_hndl, qp_hndl, priv_context) ( \
  RES1( EVAPI_set_priv_context4qp( (VAPI_hca_hndl_t)(hca_hndl),    \
                                   (VAPI_qp_hndl_t)(qp_hndl),      \
                                   (void*)(priv_context))))


/*
 *  iba_get_priv_context4qp - Get Queue Pair Private Context
 */

#define iba_get_priv_context4qp(hca_hndl, qp_hndl, priv_context_p) ( \
  RES1( EVAPI_get_priv_context4qp( (VAPI_hca_hndl_t)(hca_hndl),      \
                                   (VAPI_qp_hndl_t)(qp_hndl),        \
                                   (void**)(priv_context_p))))


/*
 *  iba_reg_mr - Register Memory Region
 */

#define iba_reg_mr(hca_hndl, mr_attr_p, mr_hndl_p, ret_mr_attr_p) ( \
  RES1( VAPI_register_mr( (VAPI_hca_hndl_t)(hca_hndl),              \
                          (VAPI_mrw_t*)(mr_attr_p),                 \
                          (VAPI_mr_hndl_t*)(mr_hndl_p),             \
                          (VAPI_mrw_t*)(ret_mr_attr_p))))


/*
 *  iba_dereg_mr - Deregister Memory Region
 */

#define iba_dereg_mr(hca_hndl, mr_hndl) (                \
  RES1( VAPI_deregister_mr( (VAPI_hca_hndl_t)(hca_hndl), \
                            (VAPI_mr_hndl_t)(mr_hndl))))


/*
 *  iba_post_send - Post Send Request
 */

#define iba_post_send(hca_hndl, qp_hndl, send_wr_p) ( \
  RES1( VAPI_post_sr( (VAPI_hca_hndl_t)(hca_hndl),    \
                      (VAPI_qp_hndl_t)(qp_hndl),      \
                      (VAPI_sr_desc_t*)(send_wr_p))))


/*
 *  iba_post_send_list - Post List of Send Requests
 */

#define iba_post_send_list(hca_hndl, qp_hndl, num_of_requests, send_wr_array) ( \
  RES1( EVAPI_post_sr_list( (VAPI_hca_hndl_t)(hca_hndl),                        \
                            (VAPI_qp_hndl_t)(qp_hndl),                          \
                            (u_int32_t)(num_of_requests),                       \
                            (VAPI_sr_desc_t*)(send_wr_array))))


/*
 *  iba_post_send - Post Inline Send Request
 */

#define iba_post_inline_send(hca_hndl, qp_hndl, send_wr_p) (  \
  RES1( EVAPI_post_inline_sr( (VAPI_hca_hndl_t)(hca_hndl),    \
                              (VAPI_qp_hndl_t)(qp_hndl),      \
                              (VAPI_sr_desc_t*)(send_wr_p))))


/*
 *  iba_post_recv - Post Recv Request
 */

#define iba_post_recv(hca_hndl, qp_hndl, recv_wr_p) ( \
  RES1( VAPI_post_rr( (VAPI_hca_hndl_t)(hca_hndl),    \
                      (VAPI_qp_hndl_t)(qp_hndl),      \
                      (VAPI_rr_desc_t*)(recv_wr_p))))


/*
 *  iba_post_recv_list - Post List of Recv Requests
 */

#define iba_post_recv_list(hca_hndl, qp_hndl, num_of_requests, recv_wr_array) ( \
  RES1( EVAPI_post_rr_list( (VAPI_hca_hndl_t)(hca_hndl),                        \
                            (VAPI_qp_hndl_t)(qp_hndl),                          \
                            (u_int32_t)(num_of_requests),                       \
                            (VAPI_rr_desc_t*)(recv_wr_array))))


/*
 *  iba_poll_cq - Poll CQ
 */

static inline iba_status_t iba_poll_cq(iba_hca_hndl_t hca_hndl, iba_cq_hndl_t cq_hndl, iba_wc_t *wc_p)
{
  VAPI_ret_t ret;
  ret = VAPI_poll_cq((VAPI_hca_hndl_t)hca_hndl, (VAPI_cq_hndl_t)cq_hndl, (VAPI_wc_desc_t*)wc_p);
  switch (ret)
  {
    case VAPI_CQ_EMPTY:
      return IBA_CQ_EMPTY;
    case VAPI_OK:
      return IBA_OK;
    default:
      return IBA_ERROR;
  }
}

/*
 *  iba_create_addr_hndl - Create Address Handle
 */

#define iba_create_addr_hndl(hca_hndl, pd_hndl, av_p, av_hndl_p) ( \
  RES1( VAPI_create_addr_hndl( (VAPI_hca_hndl_t)(hca_hndl),        \
                               (VAPI_pd_hndl_t)(pd_hndl),          \
                               (VAPI_ud_av_t*)(av_p),              \
                               (VAPI_ud_av_hndl_t*)(av_hndl_p))))


/*
 *  iba_destroy_addr_hndl - Destroy Address Handle
 */

#define iba_destroy_addr_hndl(hca_hndl, av_hndl) (              \
  RES1( VAPI_destroy_addr_hndl( (VAPI_hca_hndl_t)(hca_hndl),    \
                                (VAPI_ud_av_hndl_t)(av_hndl))))


/*
 *  iba_attach_multicast - Attach QP to Multicast Group
 */

#define iba_attach_multicast(hca_hndl, qp_hndl, gid_p, lid) (  \
  RES1( VAPI_attach_to_multicast( (VAPI_hca_hndl_t)(hca_hndl), \
                                  *((IB_gid_t*)(gid_p)),       \
                                  (VAPI_qp_hndl_t)(qp_hndl),   \
                                  (IB_lid_t)(lid))))


/*
 *  iba_detach_multicast - Detach QP from Multicast Group
 */

#define iba_detach_multicast(hca_hndl, qp_hndl, gid_p, lid) (    \
  RES1( VAPI_detach_from_multicast( (VAPI_hca_hndl_t)(hca_hndl), \
                                    *((IB_gid_t*)(gid_p)),       \
                                    (VAPI_qp_hndl_t)(qp_hndl),   \
                                    (IB_lid_t)(lid))))


#define iba_strerror VAPI_strerror
#define iba_strerror_sym VAPI_strerror_sym
#define iba_wc_status_sym VAPI_wc_status_sym
#define iba_cqe_opcode_sym VAPI_cqe_opcode_sym

