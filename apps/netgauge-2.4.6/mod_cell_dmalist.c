/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#include <errno.h>
#include <string.h> /* strerror */

#ifdef NG_CELL

#include <ppu_intrinsics.h>
#include <libspe2.h>
#include "mod_cell.h"

/** module function prototypes */
int cell_dmalist_sendto(int dst, void *buffer, int size);
int cell_dmalist_recvfrom(int src, void *buffer, int size);

extern void cell_dma_usage(void);
extern int cell_dma_getopt(int argc, char **argv, struct ng_options *global_opts);
extern int cell_dma_init(struct ng_options *global_opts);


/** module registration data structure */
struct ng_module cell_dmalist_module = {
   .name         = "celldmalist",
   .desc         = "Mode celldmalist tests Cell BE data transmission via dma lists.",
   .max_datasize = CELL_BUFSIZE,
   .headerlen    = 0,         /* no extra space needed for header */
   .getopt       = cell_dma_getopt,
   .flags        = NG_MOD_RELIABLE,
   .init         = cell_dma_init,
   .shutdown     = cell_shutdown,
   .usage        = cell_dma_usage,

   .sendto       = cell_dmalist_sendto,
   .recvfrom     = cell_dmalist_recvfrom,
};


/** module private data */
struct cell_private_data module_data;


/** module function for sending a single buffer of data */
int cell_dmalist_sendto(int dst, void *buffer __attribute__((unused)), int size) {
   cell_task task;
   int ret;
   uint32_t data;

   /* calculate how often a full dma transfer should be made */
   task.dma_description.full_transfers = size / CELL_MAX_DMA;
   
   /* calculate how much data the should send in last transfer */
   task.dma_description.small_transfer = size % CELL_MAX_DMA;
   task.dma_description.type = CELL_TASK_LIST_READ;
   
   /* send read task to spu and wait until it has finished the transfer */
   if (spe_in_mbox_write(module_data.ctx[0], &task.mail, 1, SPE_MBOX_ALL_BLOCKING) != 1) {
      ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
      return -1;
   }

   /* wait for spe signaling end of task */
   while (spe_out_mbox_status(module_data.ctx[0]) == 0) {
      if (stop_tests || errno == EINTR) {
         ng_error("spe_out_mbox_status() interrupted");
         return -1;
      }
   }
   ret = spe_out_mbox_read(module_data.ctx[0], &data, 1);
   if (ret == 0) {
      ng_error("spe_out_mbox_read() failed: %s", strerror(errno));
      return -1;
   }
   
   /* send size to indicate that everything went fine */
   return size;
}


/** module function for receiving a single buffer of data */
int cell_dmalist_recvfrom(int src, void *buffer __attribute__((unused)), int size) {
   cell_task task;
   int ret;
   uint32_t data;

   /* calculate how often a full dma transfer should be made */
   task.dma_description.full_transfers = size / CELL_MAX_DMA;
   
   /* calculate how much data the should send in last transfer */
   task.dma_description.small_transfer = size % CELL_MAX_DMA;
   task.dma_description.type = CELL_TASK_LIST_WRITE;
   
   /* send read task to spu and wait until it has finished the transfer */
   if (spe_in_mbox_write(module_data.ctx[0], &task.mail, 1, SPE_MBOX_ALL_BLOCKING) != 1) {
      ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
      return -1;
   }

   /* wait for spe signaling end of task */
   while (spe_out_mbox_status(module_data.ctx[0]) == 0) {
      if (stop_tests || errno == EINTR) {
         ng_error("spe_out_mbox_status() interrupted");
         return -1;
      }
   }
   ret = spe_out_mbox_read(module_data.ctx[0], &data, 1);
   if (ret == 0) {
      ng_error("spe_out_mbox_read() failed: %s", strerror(errno));
      return -1;
   }
   
   /* send size to indicate that everything went fine */
   return size;
}

#endif  /* #ifdef NG_CELL */
