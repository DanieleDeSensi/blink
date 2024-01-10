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

int main(int speid __attribute__((unused)), uint64_t argp __attribute__((unused)), uint64_t envp __attribute__((unused))) {
   cell_task task;
   unsigned int shutdown = 0;
   unsigned int i;
   unsigned int pos;
   uint32_t data;
   unsigned int uint_transfers;

   unsigned char buffer[CELL_BUFSIZE] __attribute__((aligned(128)));
   uint32_t* ibuffer = (uint32_t*)buffer;

   /* send relative position of buffer to ppu (this must be done by every spu programm for netgauge) */
   spu_write_out_mbox((uint32_t)buffer);

   while (shutdown == 0) {
      /* wait for mail with task informations */
      task.mail = spu_read_in_mbox();

      switch (task.description.type) {
         case CELL_TASK_MAIL_READ: /* receive data over many mails */
            /* receive data in buffer as 32 bit chunks as mail */
            uint_transfers = task.description.size / sizeof(data);
            for (i = 0; i < uint_transfers; i ++) {
               ibuffer[i] = spu_read_in_mbox();
            }
            
            /* send missing data */
            i *= sizeof(data);
            if (i < task.description.size) {
               data = spu_read_in_mbox();
               
               /* decode missing bytes from 32 bit integer */
               pos = task.description.size;
               for (i = i; i < task.description.size; i++) {
                  pos--;
                  buffer[pos] = (unsigned char)data;
                  data >>= 8;
               }
            }
            break;

         case CELL_TASK_MAIL_WRITE: /* send data over many mails */
            /* send data in buffer as 32 bit chunks as mail */
            uint_transfers = task.description.size / sizeof(data);
            for (i = 0; i < uint_transfers; i ++) {
               spu_write_out_mbox(ibuffer[i]);
            }
            
            /* send missing data */
            i *= sizeof(data);
            if (i < task.description.size) {
               /* encoding missing bytes in 32 bit integer */
               data = 0;
               for (i = i; i < task.description.size; i++) {
                  data <<= 8;
                  data |= buffer[i];
               }
               
               spu_write_out_mbox(data);
            }
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
