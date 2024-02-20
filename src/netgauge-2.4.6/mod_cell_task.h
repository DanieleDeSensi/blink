/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#ifndef MOD_CELL_TASK_H_
#define MOD_CELL_TASK_H_

#define CELL_MAX_DMA 16384
#define CELL_LISTS 15
#define CELL_BUFSIZE CELL_MAX_DMA*CELL_LISTS

#define CELL_TASK_EXIT 0
#define CELL_TASK_DMA_READ 1
#define CELL_TASK_DMA_WRITE 2
#define CELL_TASK_LIST_READ 3
#define CELL_TASK_LIST_WRITE 4
#define CELL_TASK_MAIL_READ 5
#define CELL_TASK_MAIL_WRITE 6

/**
Task information to be send to the SPEs which then start a transfer
or stop their running loop
*/
typedef union {
   /** representation of task to be send as mail to spe mailbox */
   uint32_t mail;

   /** real data structure to be worked with in dma mode */
   struct {
      /** type of task to be executed by SPE - see CELL_TASK_* */
      uint8_t type;

      /**
      Number of full (16kB) transfers to be initiate by SPE

      \remarks can be calculated by size_to_transfer DIV MAX_DMA
         due the size of the local store only LISTS many are
         allowed 
      */
      uint8_t full_transfers;

      /**
      size of last transfer

      \remarks can be calculated by size_to_transfer MOD MAX_DMA
         due the restrictions of the MFCs only MAX_DMA bytes are
         allowed
      */
      uint16_t small_transfer;
   }
   dma_description;

   /** real data structure to be worked with */
   struct {
      /** type of task to be executed by SPE - see CELL_TASK_* */
      uint8_t type;

      /**
      Number of bytes to transfer to/from SPE
      */
      uint32_t size : 24;
   }
   description;

} cell_task;

#endif /*MOD_CELL_TASK_H_*/
