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
int cell_mail_sendto(int dst, void *buffer, int size);
int cell_mail_recvfrom(int src, void *buffer, int size);
void cell_mail_usage(void);
int  cell_mail_getopt(int argc, char **argv, struct ng_options *global_opts);
int cell_mail_setup_channels();


/** module registration data structure */
struct ng_module cell_mail_module = {
   .name         = "cellmail",
   .desc         = "Mode cellmail tests Cell BE data transmission via mailboxes.",
   .max_datasize = CELL_BUFSIZE,
   .headerlen    = 0,         /* no extra space needed for header */
   .getopt       = cell_getopt,
   .flags        = NG_MOD_RELIABLE,
   .init         = cell_init,
   .shutdown     = cell_shutdown,
   .usage        = cell_usage,

   .sendto       = cell_mail_sendto,
   .recvfrom     = cell_mail_recvfrom,
};


/** module private data */
struct cell_private_data module_data;


/** module function for sending a single buffer of data */
int cell_mail_sendto(int dst, void *buffer, int size) {
   cell_task task;
   int ret;
   uint32_t data;
   unsigned int i;
   unsigned int uint_transfers;
   
   unsigned char* bbuffer = (unsigned char*)buffer;
   uint32_t* ibuffer = (uint32_t*)buffer;

   /* send read task to spu and wait until it has finished the transfer */
   task.description.size = size;
   task.description.type = CELL_TASK_MAIL_READ;
   
   ret = spe_in_mbox_write(module_data.ctx[0], &task.mail, 1, SPE_MBOX_ALL_BLOCKING);
   if (ret != 1) {
      ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
      return -1;
   }
   
   /* send data in buffer as 32 bit chunks as mail */
   uint_transfers = size / sizeof(data);
   for (i = 0; i < uint_transfers; i++) {
      /* send mail to ingoing mailbox of spe */
      ret = spe_in_mbox_write(module_data.ctx[0], (void*)&ibuffer[i], 1, SPE_MBOX_ALL_BLOCKING);
      if (ret != 1) {
         ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
         return -1;
      }
   }
   
   /* send missing data */
   i *= sizeof(data);
   if (i < size) {
      /* encoding missing bytes in 32 bit integer */
      data = 0;
      for (i = i; i < size; i++) {
         data <<= 8;
         data |= bbuffer[i];
      }

      /* send mail to ingoing mailbox of spe */
      ret = spe_in_mbox_write(module_data.ctx[0], (void*)&bbuffer[i], 1, SPE_MBOX_ALL_BLOCKING);
      if (ret != 1) {
         ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
         return -1;
      }
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
int cell_mail_recvfrom(int src, void *buffer, int size) {
   cell_task task;
   int ret;
   uint32_t data;
   unsigned int i;
   unsigned int pos;
   unsigned int uint_transfers;
   
   unsigned char* bbuffer = (unsigned char*)buffer;
   uint32_t* ibuffer = (uint32_t*)buffer;

   /* send read task to spu and wait until it has finished the transfer */
   task.description.size = size;
   task.description.type = CELL_TASK_MAIL_WRITE;
   if (spe_in_mbox_write(module_data.ctx[0], &task.mail, 1, SPE_MBOX_ALL_BLOCKING) != 1) {
      ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
      return -1;
   }
   
   /* receive data in buffer as 32 bit chunks as mail */
   uint_transfers = size / sizeof(data);
   for (i = 0; i < uint_transfers; i++) {
      /* wait for spe outgoing message */
      while (spe_out_mbox_status(module_data.ctx[0]) == 0) {
         if (stop_tests || errno == EINTR) {
            ng_error("spe_out_mbox_status() interrupted");
            return -1;
         }
      }
      
      /* receive mail from outgoing mailbox of spe */
      ret = spe_out_mbox_read(module_data.ctx[0], (void*)&ibuffer[i], 1);
      if (ret == 0) {
         ng_error("spe_out_mbox_read() failed: %s", strerror(errno));
         return -1;
      }
   }
   
   /* receive missing data */
   i *= sizeof(data);
   if (i < size) {
      /* wait for spe outgoing message */
      while (spe_out_mbox_status(module_data.ctx[0]) == 0) {
         if (stop_tests || errno == EINTR) {
            ng_error("spe_out_mbox_status() interrupted");
            return -1;
         }
      }
      
      /* receive mail from outgoing mailbox of spe */
      ret = spe_out_mbox_read(module_data.ctx[0], &data, 1);
      if (ret == 0) {
         ng_error("spe_out_mbox_read() failed: %s", strerror(errno));
         return -1;
      }

      /* decode missing bytes from 32 bit integer */
      pos = size;
      for (i = i; i < size; i++) {
         pos--;
         bbuffer[pos] = (unsigned char)data;
         data >>= 8;
      }
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
