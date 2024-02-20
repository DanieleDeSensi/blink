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
int cell_dma_sendto(int dst, void *buffer, int size);
int cell_dma_recvfrom(int src, void *buffer, int size);
void cell_dma_usage(void);
int  cell_dma_getopt(int argc, char **argv, struct ng_options *global_opts);
int  cell_dma_init(struct ng_options *global_opts);
int cell_dma_setup_channels();


/** module registration data structure */
struct ng_module cell_dma_module = {
   .name         = "celldma",
   .desc         = "Mode celldma tests Cell BE data transmission via dma.",
   .max_datasize = CELL_BUFSIZE,
   .headerlen    = 0,         /* no extra space needed for header */
   .getopt       = cell_dma_getopt,
   .flags        = NG_MOD_RELIABLE,
   .init         = cell_dma_init,
   .shutdown     = cell_shutdown,
   .usage        = cell_dma_usage,

   .sendto       = cell_dma_sendto,
   .recvfrom     = cell_dma_recvfrom,
};


/** module private data */
struct cell_private_data module_data;


/** netgauge dma spu program */
extern spe_program_handle_t mod_cell_dma_spu;


/** module specific usage information */
void cell_dma_usage(void) {
   cell_usage();
   ng_option_usage("-S", "Transfer data between SPEs and not between SPE and memory", "");
   ng_option_usage("-a", "Enable affinity between two adjacent SPEs", "");
}

/** parse command line parameters for module-specific options */
int  cell_dma_getopt(int argc, char **argv, struct ng_options *global_opts) {
   int c;
   char *optchars = NG_OPTION_STRING "Sa"; /* additional module options */
   extern char *optarg;
   extern int optind, opterr, optopt;

   /* send global cell getopt function data */
   int failure = cell_getopt(argc, argv, global_opts);

   while ((failure == 0) && ((c = getopt(argc, argv, optchars)) >= 0)) {
      switch (c) {
         case '?': /* unrecognized or badly used option */
            if (!strchr(optchars, optopt))
               continue; /* unrecognized */
            ng_error("Option %c requires an argument", optopt);
            failure = 1;
            break;
         case 's': /* server / client */
            ng_error("Option -s doesn't work in cell mode");
            failure = 1;
            break;
         case 'S': /* spe <-> spe mode and not ppu <-> spe */
            module_data.num_spes = 2;
            module_data.tmode = spe_spe;
            break;
         case 'a':
            module_data.affinity = 1;
            break;
      }
   }
   
   /* reset option index */
   optind = 1;
   
   if (module_data.affinity == 1 && module_data.num_spes == 1) {
      ng_error("Option -a can only be used with -S");
      failure = 1;
   }

   /* set mod_cell_dma_spu as spu_progam */
   module_data.spu_program = &mod_cell_dma_spu;
   
   /* check for necessary parameters, apply default values where possible */
   /* if (!failure) {
    }
   */
   /* report success or failure */
   return failure;
}


/** module specific benchmark initialization */
int  cell_dma_init(struct ng_options *global_opts) {
   int ret;

   /* Run global initialisation for cell */
   ret = cell_init(global_opts);

   if (ret == 0) {
      /* Setup channels here because setup_channels was removed in ng_module */
      ret = cell_dma_setup_channels();
   }

   /* everything went fine */
   return ret;
}


/** sets up channels for use by sendto and recvfrom functions */
int cell_dma_setup_channels() {
   unsigned int i;
   uint64_t pointer;
   uint32_t pointer_high, pointer_low;
   
   /* send pointer of buffer to spu which it can write to */
   switch (module_data.tmode) {
   default:
   case ppu_spe:
      pointer = (uint64_t)(module_data.buffer); /* this does only work because we tested for 32/64-bit mode */
      break;
   case spe_spe:
      pointer = (uint64_t)(module_data.local_store[1]); /* this does only work because we tested for 32/64-bit mode */
      break;
   }
   
   pointer_low = (uint32_t)(pointer);
   
   /* Cell will add 0xffffffff in front of a pointer when cast to 64 bit */
   /* we need to remove it when working in 32 bit mode */
   if (sizeof(void*) == 4) {
      pointer_high = 0x00000000;
   } else {
      pointer_high = (uint32_t)(pointer >> 32);
   }

   /* send to all spus information about their buffers - even if they will not do anything
      (otherwise the will not be able to receive the exit task)                            */
   for (i = 0; i < module_data.num_spes; i++) {
      if (spe_in_mbox_write(module_data.ctx[i], &pointer_high, 1, SPE_MBOX_ALL_BLOCKING) != 1) {
         ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
         return -1;
      }
      if (spe_in_mbox_write(module_data.ctx[i], &pointer_low, 1, SPE_MBOX_ALL_BLOCKING) != 1) {
         ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
         return -1;
      }
   }
   
   /* everything went fine */
   return 0;
}


/** module function for sending a single buffer of data */
int cell_dma_sendto(int dst, void *buffer __attribute__((unused)), int size) {
   cell_task task;
   int ret;
   uint32_t data;

   /* calculate how often a full dma transfer should be made */
   task.dma_description.full_transfers = size / CELL_MAX_DMA;
   
   /* calculate how much data the should send in last transfer */
   task.dma_description.small_transfer = size % CELL_MAX_DMA;
   task.dma_description.type = CELL_TASK_DMA_READ;
   
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
int cell_dma_recvfrom(int src, void *buffer __attribute__((unused)), int size) {
   cell_task task;
   int ret;
   uint32_t data;

   /* calculate how often a full dma transfer should be made */
   task.dma_description.full_transfers = size / CELL_MAX_DMA;
   
   /* calculate how much data the should send in last transfer */
   task.dma_description.small_transfer = size % CELL_MAX_DMA;
   task.dma_description.type = CELL_TASK_DMA_WRITE;
   
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
