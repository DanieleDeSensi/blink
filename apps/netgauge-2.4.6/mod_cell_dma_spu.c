/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include <spu_intrinsics.h>
#include <spu_mfcio.h>
#include "mod_cell_task.h"

typedef struct {
   uint32_t length;
   uint32_t address;
}
dma_list_elem;

int main(int speid __attribute__((unused)), uint64_t argp __attribute__((unused)), uint64_t envp __attribute__((unused))) {
   cell_task task;
   unsigned int tag = 10;
   unsigned int external_buffer_high, external_buffer_low;
   unsigned int shutdown = 0;
   unsigned int i;

   dma_list_elem dma_list[CELL_LISTS] __attribute__((aligned(8)));
   unsigned char buffer[CELL_BUFSIZE] __attribute__((aligned(128)));
   unsigned int list_size;

   /* send relative position of buffer to ppu (this must be done by every spu programm for netgauge) */
   spu_write_out_mbox((uint32_t)buffer);
   
   /* receive pointer to ppu buffer (needed in every spu program for netgauge) */
   external_buffer_high = spu_read_in_mbox();
   external_buffer_low = spu_read_in_mbox();


   /* indicate that we want to wait for transfers with this tag */
   mfc_write_tag_mask(1 << tag);

   while (shutdown == 0) {
      /* wait for mail with task informations */
      task.mail = spu_read_in_mbox();

      list_size = 0;
      
      /* if we have more data than our buffer can hold - just cut it down */
      if (task.dma_description.full_transfers > CELL_LISTS) {
         task.dma_description.full_transfers = CELL_LISTS;
         task.dma_description.small_transfer = 0;
      }

      /* fill list with informations about the full sized dma transfers */
      for (i = 0; i < task.dma_description.full_transfers; i++) {
         dma_list[i].length = CELL_MAX_DMA;
         dma_list[i].address = i * CELL_MAX_DMA + external_buffer_low;
         list_size++;
      }

      /* if we have a size that can not divided by 16*1024 we must insert a last, smaller transfer */
      if (task.dma_description.small_transfer != 0) {
         dma_list[i].address = list_size * CELL_MAX_DMA + external_buffer_low;
         /* we cannot send data with a size not a multiple of 16 */
         dma_list[list_size].length = (task.dma_description.small_transfer + 15) & ~0x0F;
         list_size++;
      }


      switch (task.dma_description.type) {
         case CELL_TASK_DMA_READ: /* create many dma transfers to read from external buffer */
            for (i = 0; i < list_size; i++) {
               spu_mfcdma64((void*) &buffer, external_buffer_high, dma_list[i].address, dma_list[i].length,
                            tag, MFC_GET_CMD);
            }
            mfc_read_tag_status_all();
            break;

         case CELL_TASK_DMA_WRITE: /* create many dma transfers to write to external buffer */
            for (i = 0; i < list_size; i++) {
               spu_mfcdma64((void*) &buffer, external_buffer_high, dma_list[i].address, dma_list[i].length,
                            tag, MFC_PUT_CMD);
            }
            mfc_read_tag_status_all();
            break;

         case CELL_TASK_LIST_READ: /* create a dma list transfers to read from external buffer */
            spu_mfcdma64((void*) &buffer, external_buffer_high, (uint32_t)dma_list, sizeof(dma_list_elem)*list_size,
                         tag, MFC_GETL_CMD);
            mfc_read_tag_status_all();
            break;

         case CELL_TASK_LIST_WRITE: /* create a dma list transfers to write to external buffer */
            spu_mfcdma64((void*) &buffer, external_buffer_high, (uint32_t)dma_list, sizeof(dma_list_elem)*list_size,
                         tag, MFC_PUTL_CMD);
            mfc_read_tag_status_all();
            break;

         case CELL_TASK_EXIT: /* finish task loop (needed in every spu program for netgauge) */
            shutdown = 1;
            break;
      }
      
      /* send mail that we finished the task (needs to be implemented in every spu program) */
      spu_write_out_mbox(1);
   }

   return 0;
}
