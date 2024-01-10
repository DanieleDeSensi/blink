/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef MOD_CELL_H_
#define MOD_CELL_H_

#include <stdint.h>
#include <pthread.h>

#include "mod_cell_task.h"

typedef enum  {
   ppu_spe = 0, /**< dma(list) transfer between ppu and spe local store */
   spe_spe = 1 /**< dma(list) transfer between two spe local stores */
} cell_transfer_mode;

/** module private data */
struct cell_private_data {
   /** spu program which should run on all spus */
   spe_program_handle_t *spu_program;
   
   /** number of spes which should be initialised */
   unsigned int       num_spes;
   
   /** if spes should have a affinity for their precessor (bool) */
   int affinity;
   
   /** mode of transfer */
   cell_transfer_mode tmode;
   
   /** array of threads which run the spe context */
   pthread_t          *pts;
   
   /** array of spe context */
   spe_context_ptr_t  *ctx;
   
   /** gang context for affinity */
   spe_gang_context_ptr_t gang_ctx;
   
   /** array of pointer to buffer in memory mapped local stores */
   void               **local_store;

   /* We cannot use normal buffer in sendto und recvfrom, because we need a 16 byte aligned one */
   /** private buffer of ppu */
   unsigned char buffer[CELL_BUFSIZE] __attribute__((aligned(16)));
};

/** module function prototypes */
int  cell_getopt(int argc, char **argv, struct ng_options *global_opts);
int  cell_init(struct ng_options *global_opts);
void cell_shutdown(struct ng_options *global_opts);
void cell_usage(void);

int cell_sendto(int dst, void *buffer, int size);
int cell_recvfrom(int src, void *buffer, int size);

#endif /*MOD_CELL_H_*/
