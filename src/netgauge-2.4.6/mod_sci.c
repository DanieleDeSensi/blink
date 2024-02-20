/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_SISCI && defined NG_MOD_SCI

#include "sisci_api.h"
#include <string.h>

/*
 * Macro Definitions
 */

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0
#define NO_OFFSET 0
#define NO_SUGGESTED_ADDRESS 0
#define ADAPTER_NO 0
#define SEG_ID 0
#define SEG_ID_DMA 1
#define NUM_LOCAL_SEGMENTS 2                   // # of local SCI segments       
#define DEFAULT_LOCAL_SEGMENT_SIZE 1*1024*1024 // 1 MB
#define TIMEOUT SCI_INFINITE_TIMEOUT
#define TRIES 4                                // # attempts to connect to a remote segment
#define SCI_RELIABLE 1                         // Reliable data transfer?
#define LOCAL_MEMCPY 1                         // Copy local memory when necessary?     
#define MAX_NUM_DMA_QUEUE_ENTRIES 4
#define MIN_NUM_DMA_OFFSET_ALIGNMENTS 3        // Minimum number of how many
                                               // "private_data.info_dma_offset_alignment"s
                                               // a mapped remote segment must consist of


/*
 * Type Definitions
 */

/* Queue for data transfer requests */
typedef struct queue {
  struct queue_element *head;
  struct queue_element *tail;
  struct queue         *next;
} Queue;

/* Queue element */
typedef struct queue_element {
  struct queue_element *next;
  struct queue         *queue;

  /* 
   * Queue specific function to forward the request
   * such a queue element stands for via DMA
   * (Direct Memory Access)
   */
  int (*forward_via_DMA)(struct queue_element *element);

  /* 
   * Queue specific function to forward the request
   * such a queue element stands for via PIO
   * (Programmed I/O)           
   */
  int (*forward_via_PIO)(struct queue_element *element);
  
  /* Yields if the request is finished or not */
  int (*done)(struct queue_element *element);

  /* Frees the request's memory (destroys it) */
  void (*destroy)(struct queue_element *element);

  /* Queue specific data element */
  void *data;
} Queue_element;

/* Send queue data element */
typedef struct sendq_data_element {
  int        dst;
  int        size;
  void       *buffer;

  int        payload_transfer_size;   // Size of the actual payload of next transfer
  int        DMA_transfer_size;       // Size of next transfer 
  char       DMA_buffer_sent;         // Flag, indicating: DMA buffer was sent 
  char       DMA_buffer_to_send;      // Flag, indicating: DMA buffer needs to be sent
  char       done;                    // Flag, set if request done (for debugging) 
} Sendq_data_element;

/* Receive queue data element */
typedef struct recvq_data_element {
  int        src;
  int        size;
  void       *buffer;                 

  char       done;                    // Flag, set if request done (for debugging)
} Recvq_data_element;

/* Node in the SCI network */
typedef struct node {

  /* Handles to remote SCI resources */
  sci_desc_t v_dev;                        // virtual SCI device to remote segment
  sci_remote_segment_t remote_seg_res;
  sci_sequence_t remote_seg_seq;
  sci_map_t remote_map;

  /* Handle to DMA queue */
  sci_dma_queue_t dma_queue;

  /* SCI node ID of the remote node */
  int sci_id;

  /* Pointers to local buffers of the protocol */
  void *local_datab_addr;
  void *local_idb_addr;
  void *local_ackb_addr;

  /* Pointer to local DMA buffer */
  void *local_dmab_addr;

  /* Pointers to remote buffers of the protocol */
  void *remote_datab_addr;
  void *remote_idb_addr;
  void *remote_ackb_addr;
  volatile void *remote_seg_addr;        // 'volatile' according to SISCI API

  /* Offset into the remote segment */
  int remote_seg_offset;

  /* Status information for the protocol */
  u_int8_t next_id_to_send;
  u_int8_t next_ack_to_recv;
  u_int8_t next_id_to_recv;

  Queue send_queue;
  Queue recv_queue;

} Node;

/* Module private data */
typedef struct private_data {

  /* Handles to local SCI resources */
  sci_desc_t v_dev;                        // virtual SCI device to local segment
  sci_local_segment_t local_seg_res;
  sci_map_t local_map;
  void *local_seg_addr;

  /* Handles to local SCI DMA resources */
  sci_desc_t v_dev_dma;                   // virtual SCI device to local segment
                                          // used for DMA
  sci_local_segment_t local_seg_res_dma;
  sci_map_t local_map_dma;
  void *local_seg_addr_dma;

  /* Info about the underlying SCI System */
  int info_dma_size_alignment;
  int info_dma_offset_alignment;
  int info_dma_mtu;
  int info_suggested_min_dma_size;
  int info_suggested_min_block_size;

  /* Size of a remote segment mapped into addressable space */
  int remote_seg_mapped_size;

  /*
   * Pointer to array with information for
   * communicating with every node in the SCI network
   */
  Node *node;

  /* Information about MPI_COMM_WORLD */
  int worldrank;
  int worldsize;

  /* Command line options */
  int opt_local_seg_size;

  /* 
   * This pointer is assigned to the 
   * "forward_via_PIO" variable in
   * send request structures and determines 
   * the type of PIO data transfers
   */  
  int (*opt_pio_mode)(Queue_element *element); 

  /* The protocol's MTU */
  int protocol_mtu;

} Private_data;

/*
 * Function Declarations
 */

static inline int my_getopt(int argc, char **argv, struct ng_options *global_opts);
static inline int my_init(struct ng_options *global_opts);
static inline void my_shutdown(struct ng_options *global_opts);
static inline void my_usage(void);

/* Blocking sends/recv */
static inline int sendto_via_SCIMemCpy(int dst, void *buffer, int size);
static inline int sendto_via_SCIMemWrite(int dst, void *buffer, int size);
static inline int sendto_via_SCITransferBlock(int dst, void *buffer, int size);
static inline int sendto_via_memcpy(int dst, void *buffer, int size);
static inline int my_recvfrom(int src, void *buffer, int size);

/* Non-blocking send/recv */
static inline int my_isendto(int dst, void *buffer, int size, NG_Request *req);
static inline int my_irecvfrom(int src, void *buffer, int size, NG_Request *req);

static inline int test_via_DMA(NG_Request *req);
static inline int test_via_PIO(NG_Request *req);

/* Additional functions */

/*
 * ONLY apply forward_sendto/recvfrom*() to the first request in a queue
 * because otherwise a request could be finished
 * or send data out of order
 */
static inline int forward_sendto_via_DMA(Queue_element *element);
static inline int forward_sendto_via_SCIMemCpy(Queue_element *element);
static inline int forward_sendto_via_SCIMemWrite(Queue_element *element);
static inline int forward_sendto_via_SCITransferBlock(Queue_element *element);
static inline int forward_sendto_via_memcpy(Queue_element *element);
static inline int forward_recvfrom(Queue_element *element);
static inline void forward_any_via_PIO(Queue *queue);

static inline int assure_reliable_memcpy_transfer(int dst, void *dst_buf, void *src_buf, int size);
static inline int assure_reliable_SCIMemWrite_transfer(int dst, void *dst_buf, void *src_buf, int size);
static inline int assure_reliable_SCITransferBlock_transfer(int dst, void *dst_buf, void *src_buf, int size);

static inline Queue_element *get_next_non_empty_queues_head(Queue_element *element);
static inline Queue *get_next_non_empty_queue(Queue *queue);
static inline void enqueue(Queue *queue, Queue_element *element);
static inline void dequeue(Queue *queue);
static inline int done(Queue_element *element);
static inline void destroy(Queue_element *element);
static inline char *get_sci_error(int error);

/*
 * Global Variables
 */

/* Module registration data structure */
static struct ng_module sci_module = {
  .name                  = "sci",
  .desc                  = "Mode sci uses the SISCI API for distributed shared memory "
                           "data transmission.",
  .max_datasize          = -1,                /* Can send data of arbitrary size */
  .headerlen             = 0,                 /* No extra space needed for header */
  .flags                 = NG_MOD_MEMREG | NG_MOD_RELIABLE | NG_MOD_CHANNEL,
  .malloc                = NULL, /* implement */
  .getopt                = my_getopt,
  .init                  = my_init,
  .shutdown              = my_shutdown,
  .usage                 = my_usage,
  .sendto                = NULL,
  .recvfrom              = my_recvfrom,
  .isendto               = my_isendto,
  .irecvfrom             = my_irecvfrom,
  .test                  = test_via_DMA,
};

/* Module's private data */
static Private_data private_data = {0};

/*
 * Function Definitions
 */

static inline int my_getopt (int argc, char **argv, struct ng_options *global_opts) {
  int c;
  char *optstring = NG_OPTION_STRING "M:P:";    // Additional module options
  extern char *optarg;
  extern int optind, opterr, optopt;
   
  int failure = 0;
  int memsize_available_set = 0;
  int pio_mode_set = 0;
   
  /* 
   * SCI can only be used with MPI support enabled.
   * MPI is used, among others, to communicate the SCI node IDs.
   */
  if (!global_opts->mpi) {
    ng_error("Mode %s requires the global option '-p' (execution via MPI)",
             global_opts->mode);
    failure = 1;
  }
   
  while (!failure && ((c = getopt(argc, argv, optstring)) >= 0)) {
    switch (c) {
      case '?': 
        /*
         *  Unrecognized or badly used option
         */
        // Unrecognized option
        if (!strchr(optstring, optopt)) {
          continue;     
        }
        // Badly used option
        ng_error("Option '-%c' requires an argument", optopt);
        failure = 1;
        break;
      case 'M':
         /*
          * Amount of memory available for 
          * creating local SCI segments
          */
        // Size of one local segment
        private_data.opt_local_seg_size = atoi(optarg) / NUM_LOCAL_SEGMENTS;    
        memsize_available_set = 1;
        break;
      case 'P':
        /*
         * PIO (Programmed I/O) mode
         * for PIO data transmissions
         */
        // SCIMemCpy
        if (!strcmp("SCIMemCpy", optarg)) {
          sci_module.sendto = sendto_via_SCIMemCpy;
          private_data.opt_pio_mode = forward_sendto_via_SCIMemCpy;
          pio_mode_set = 1;
        }
        // SCIMemWrite
        else if (!strcmp("SCIMemWrite", optarg)) {
          sci_module.sendto = sendto_via_SCIMemWrite;
          private_data.opt_pio_mode = forward_sendto_via_SCIMemWrite;
          pio_mode_set = 1;
        }
        // SCITransferBlock
        else if (!strcmp("SCITransferBlock", optarg)) {
          sci_module.sendto = sendto_via_SCITransferBlock;
          private_data.opt_pio_mode = forward_sendto_via_SCITransferBlock;
          pio_mode_set = 1;
        }
        // memcpy
        else if (!strcmp("memcpy", optarg)) {
          sci_module.sendto = sendto_via_memcpy;
          private_data.opt_pio_mode = forward_sendto_via_memcpy;
          pio_mode_set = 1;
        }
        else {
          ng_error("Only 'SCIMemCpy', 'SCIMemWrite', 'SCITranferBlock'");
          ng_error("or 'memcpy' are possible arguments for option '-P'");
          failure = 1;
        }
        break;
    } // End switch
  } // End while
          
  /* 
   * Set default values where necessary
   */
  if (!failure) {
    if (!memsize_available_set) {
      private_data.opt_local_seg_size = DEFAULT_LOCAL_SEGMENT_SIZE;
      ng_info(NG_VLEV1, "SCI INFO: Amount of memory available for creating local SCI");
      ng_info(NG_VLEV1, "SCI INFO: segments not specified - defaulting to: %u B", 
                        DEFAULT_LOCAL_SEGMENT_SIZE * NUM_LOCAL_SEGMENTS);
    }
    if (!pio_mode_set) {
      sci_module.sendto = sendto_via_SCIMemCpy; 
      private_data.opt_pio_mode = forward_sendto_via_SCIMemCpy;
      ng_info(NG_VLEV1, "SCI INFO: PIO (Programmed I/O) mode for PIO data transmissions"); 
      ng_info(NG_VLEV1, "SCI INFO: not specified - defaulting to 'SCIMemCpy'");
    }
  } 
  /* Report success or failure */
  return failure;
}

static inline int my_init (struct ng_options *global_opts) {
  sci_error_t error;
  sci_query_adapter_t query;
  int recvcounts[global_opts->mpi_opts->worldsize];
  int displs[global_opts->mpi_opts->worldsize];
  int i, num1, num2, tmp;

  /* Get info about MPI_COMM_WORLD */
  private_data.worldrank = global_opts->mpi_opts->worldrank;
  private_data.worldsize = global_opts->mpi_opts->worldsize;

  /* Allocate memory for array with information for communicating with every SCI node */
  private_data.node = (Node *) calloc (private_data.worldsize, sizeof (Node));
  if (private_data.node == NULL) {
    ng_error ("Could not allocate %d B for node array",
              private_data.worldsize * sizeof (Node));
    return 1;
  }

  /*
   * Init some data in the array allocated above
   */
  for (i = 0; i < private_data.worldsize; i++) {
    /* Info specific to the protocol implemented */
    private_data.node[i].next_id_to_send  = 1;
    private_data.node[i].next_id_to_recv  = 1;
    private_data.node[i].next_ack_to_recv = 0;

    /* Build the ring of send and receive queues */
    // Not my node id?
    if (i != private_data.worldrank) {
      /* 
       * Pointer from send to receive 
       * queue of the same node
       */
      private_data.node[i].send_queue.next = 
        &private_data.node[i].recv_queue;

      if ((i+1) % private_data.worldsize ==
          private_data.worldrank) {
        tmp = (i+2) % private_data.worldsize;
      }
      else {
        tmp = (i+1) % private_data.worldsize;
      }

      /* 
       * Pointer from receive queue to the
       * next valid send queue
       */
      private_data.node[i].recv_queue.next =
        &private_data.node[tmp].send_queue;
    }
  }

  /* Init the SCI environment */
  SCIInitialize (NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIInitialize() failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  // Open virtual SCI device for local segment
  SCIOpen (&private_data.v_dev, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIOpen() for local device failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  // Open virtual SCI device for local DMA segment
  SCIOpen (&private_data.v_dev_dma, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIOpen() for local DMA device failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  // Open virtual SCI devices for remote segments from all SCI nodes
  for (i = 0; i < private_data.worldsize; i++) {

    // Not my node id?
    if (i != private_data.worldrank) {

      SCIOpen (&private_data.node[i].v_dev, NO_FLAGS, &error);
      if (error != SCI_ERR_OK) {
        ng_error("SCIOpen() for remote device failed with error: %s",
                 get_sci_error (error));
        return 1;
      }
    }
  }

  /* Get info about the underlying SCI System */
  query.localAdapterNo = ADAPTER_NO;
  query.subcommand = SCI_Q_ADAPTER_NODEID;
  query.data = &private_data.node[private_data.worldrank].sci_id;

  // Get my SCI node ID
  query.subcommand = SCI_Q_ADAPTER_NODEID;
  SCIQuery (SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIQuery() for SCI node id failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  // Get all SCI node IDs
  for (i = 0; i < private_data.worldsize; i++) {
    displs[i] = &private_data.node[i].sci_id - &private_data.node[0].sci_id;
    recvcounts[i] = 1;
  }
  if (MPI_Allgatherv(&private_data.node[private_data.worldrank].sci_id, 1, MPI_INT,
                     &private_data.node[0].sci_id, recvcounts, displs, MPI_INT,
                     MPI_COMM_WORLD) != MPI_SUCCESS) {
    ng_error("Could not send/receive my SCI node ID/the remaining SCI node IDs");
    return 1;
  }

  // Get alignment requirement for a segment size
  query.subcommand = SCI_Q_ADAPTER_DMA_SIZE_ALIGNMENT;
  query.data = &private_data.info_dma_size_alignment;
  SCIQuery (SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error("SCIQuery() for segment size alignment failed with error: %s",
             get_sci_error (error));
    return 1;
  }

  // Get alignment requirement for a segment offset
  query.subcommand = SCI_Q_ADAPTER_DMA_OFFSET_ALIGNMENT;
  query.data = &private_data.info_dma_offset_alignment;
  SCIQuery (SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error("SCIQuery() for segment offset alignment failed with error: %s",
             get_sci_error (error));
    return 1;
  }

  // Get maximum unit size in bytes for a DMA transfer
  query.subcommand = SCI_Q_ADAPTER_DMA_MTU;
  query.data = &private_data.info_dma_mtu;
  SCIQuery (SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIQuery() for DMA MTU failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  // Get suggested minimum size for a DMA transfer
  query.subcommand = SCI_Q_ADAPTER_SUGGESTED_MIN_DMA_SIZE;
  query.data = &private_data.info_suggested_min_dma_size;
  SCIQuery (SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIQuery() for suggested min. size of DMA transfer "
              "failed with error: %s", get_sci_error (error));
    return 1;
  }

  // Get suggested minimum size for a block transfer
  query.subcommand = SCI_Q_ADAPTER_SUGGESTED_MIN_BLOCK_SIZE;
  query.data = &private_data.info_suggested_min_block_size;
  SCIQuery (SCI_Q_ADAPTER, &query, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIQuery() for suggested min. size of block transfer "
              "failed with error: %s", get_sci_error (error));
    return 1;
  }

  /* 
   * Compute the protocol's characteristics (e.g. payload size)
   * based on the SCI environment (e.g. alignment requirements, 
   * #nodes to communicate with, ...) and check if these 
   * characteristics meet the protocol's actual requirements
   */

  // To be on the safe side regarding alignment constraints
  if (private_data.info_dma_size_alignment !=
      private_data.info_dma_offset_alignment) {
    ng_error ("DMA size alignment != DMA offset alignment, "
              "but the protocol implemented expects them to be equal.");
    return 1;
  }
  
  //
  // Compute the size of how much of a remote segment, depending on the
  // local segment size and the number of nodes, is mapped into addressable space:
  //
  // 1. How often does "private_data.info_dma_offset_alignment" fit
  //    into the size of the local segment? (=num1)
  // 2. How often does (#nodes - 1) fit into "num1"? (=num2)
  // 3. The result is "num2" * "private_data.info_dma_offset_alignment"
  //    (=private_data.remote_seg_mapped_size)
  //
  num1 = private_data.opt_local_seg_size / private_data.info_dma_offset_alignment;
  num2 = num1 / (private_data.worldsize - 1);
  private_data.remote_seg_mapped_size = num2 * private_data.info_dma_offset_alignment;

  //
  // Check if the mapped remote segment size ("private_data.remote_seg_mapped_size")
  // satisfies the protocol's minimum space requirements
  //
  if (num2 < MIN_NUM_DMA_OFFSET_ALIGNMENTS) {
    ng_error ("Not enough space in the local segment to run the protocol implemented.");
    ng_error ("Possible Solutions:");
    ng_error ("-------------------");
    ng_error ("1. Increase the local segment size (command line option '-M').");
    ng_error ("2. Decrease the number of nodes taking part in the benchmark.");
    return 1;
  }
  // Everything is all right
  ng_info (NG_VLEV1, "SCI INFO: Size of a mapped remote segment: %d B",
           private_data.remote_seg_mapped_size);

  for (i = 0; i < private_data.worldsize; i++) {

    // Not my node id?
    if (i != private_data.worldrank) {

      /* Compute the offset into each remote segment */

      // My node id < node id to connect to?
      if (private_data.worldrank < i) {
        private_data.node[i].remote_seg_offset =
          private_data.worldrank *
          private_data.remote_seg_mapped_size;
      }
      // My node id > node id to connect to!
      else {
        private_data.node[i].remote_seg_offset =
          (private_data.worldrank - 1) *
          private_data.remote_seg_mapped_size;
      }
    }
  }

  //
  // Compute the protocol's MTU:
  // 2 stands for the following protocol buffers: id, ack
  //
  private_data.protocol_mtu = private_data.remote_seg_mapped_size -
    2 * private_data.info_dma_offset_alignment;

#if 0
  /* Print SCI node IDs */
  for (i = 0; i < private_data.worldsize; i++) {
      ng_info (NG_VNORM /* | NG_VPALL */ , "sci_id[%d]: %u",
               i, private_data.node[i].sci_id);
  }
#endif

  /* Create local segment */
  SCICreateSegment(private_data.v_dev, &private_data.local_seg_res, SEG_ID,
                   private_data.opt_local_seg_size, NO_CALLBACK, NO_ARG,
                   NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCICreateSegment() failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  /* Create local DMA segment */
  SCICreateSegment(private_data.v_dev_dma, &private_data.local_seg_res_dma, SEG_ID_DMA,
                   private_data.opt_local_seg_size, NO_CALLBACK, NO_ARG,
                   NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCICreateSegment() for local DMA segment failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  /* Map local segment into addressable space */
  private_data.local_seg_addr =
    SCIMapLocalSegment(private_data.local_seg_res, &private_data.local_map,
                       NO_OFFSET, private_data.opt_local_seg_size,
                       NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIMapLocalSegment() failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  /* Map local DMA segment into addressable space */
  private_data.local_seg_addr_dma =
    SCIMapLocalSegment(private_data.local_seg_res_dma, &private_data.local_map_dma,
                       NO_OFFSET, private_data.opt_local_seg_size,
                       NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIMapLocalSegment() for local DMA segment failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  /* Init local segment appropriately */
  memset (private_data.local_seg_addr, 0, private_data.opt_local_seg_size);

  /* Init local DMA segment appropriately */
  memset (private_data.local_seg_addr_dma, 0, private_data.opt_local_seg_size);

  /* Set pointers according to the protocol to buffers in the local segment */
  for (i = 0; i < private_data.worldsize; i++) {

    // Not my node id?
    if (i != private_data.worldrank) {

      // Node id connecting to me < my node id?
      if (i < private_data.worldrank) {
        private_data.node[i].local_datab_addr =
          (u_int8_t *) private_data.local_seg_addr +
          (i * private_data.remote_seg_mapped_size);
      }
      // Node id connecting to me > my node id!
      else  {
        private_data.node[i].local_datab_addr =
          (u_int8_t *) private_data.local_seg_addr +
          ((i - 1) * private_data.remote_seg_mapped_size);
      }

      private_data.node[i].local_ackb_addr =
        (u_int8_t *) private_data.node[i].local_datab_addr +
        (private_data.remote_seg_mapped_size -
         private_data.info_dma_offset_alignment);

      private_data.node[i].local_idb_addr =
        (u_int8_t *) private_data.node[i].local_ackb_addr -
        private_data.info_dma_offset_alignment;
    }
  }

  /* Set pointers to buffers in the local DMA segment */
  for (i = 0; i < private_data.worldsize; i++) {

    // Not my node id?
    if (i != private_data.worldrank) {

      // Node id connecting to me < my node id?
      if (i < private_data.worldrank) {
        private_data.node[i].local_dmab_addr =
          (u_int8_t *) private_data.local_seg_addr_dma +
          (i * private_data.remote_seg_mapped_size);
      }
      // Node id connecting to me > my node id!
      else  {
        private_data.node[i].local_dmab_addr =
          (u_int8_t *) private_data.local_seg_addr_dma +
          ((i - 1) * private_data.remote_seg_mapped_size);
      }
    }
  }

  /* Make local segment available to the SCI network */
  SCIPrepareSegment(private_data.local_seg_res, ADAPTER_NO, NO_FLAGS,
                    &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIPrepareSegment() failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  SCISetSegmentAvailable(private_data.local_seg_res, ADAPTER_NO, NO_FLAGS,
                         &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCISetSegmentAvailable() failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  /* Prepare local DMA segment for being available to the SCI adapter */
  SCIPrepareSegment(private_data.local_seg_res_dma, ADAPTER_NO, NO_FLAGS,
                    &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIPrepareSegment() for local DMA segment failed with error: %s",
              get_sci_error (error));
    return 1;
  }

  /* Create DMA queues to all nodes */
  for (i = 0; i < private_data.worldsize; i++) {

    // Not my node id?
    if (i != private_data.worldrank) {

      SCICreateDMAQueue(private_data.v_dev_dma,
                        &private_data.node[i].dma_queue,
                        ADAPTER_NO,
                        MAX_NUM_DMA_QUEUE_ENTRIES,
                        NO_FLAGS,
                        &error);
      if (error != SCI_ERR_OK) {
        ng_error("SCICreateDMAQueue() to node %d failed with error: %s",
                 i, get_sci_error (error));
        return 1;
      }
    }
  }

  /* Wait until all nodes have made their segments available */
  MPI_Barrier (MPI_COMM_WORLD);

  return my_setup_channels();
}

static inline int my_setup_channels (void) {
  sci_error_t error;
  sci_sequence_status_t status;
  int num_errors = 0;
  int i, j;

  /* Connect to all remote segments in the SCI network */
  for (i = 0; i < private_data.worldsize; i++) {
    
    // Not my node id?
    if (i != private_data.worldrank) {

      // Return an error after TRIES unsuccessful tries to connect
      for (j = 0; j < TRIES; j++) {
        sleep (j);
        SCIConnectSegment(private_data.node[i].v_dev,
                          &private_data.node[i].remote_seg_res,
                          private_data.node[i].sci_id,
                          SEG_ID, ADAPTER_NO, NO_CALLBACK,
                          NO_ARG, TIMEOUT, NO_FLAGS, &error);
        if (error == SCI_ERR_OK) {
          // Leave this loop
          break;
        }
      }

      // Not connected?
      if (error != SCI_ERR_OK) {
        ng_error("SCIConnectSegment() to node %d failed with error: %s",
                 i, get_sci_error (error));
        num_errors++;
      }
      else {
        ng_info (NG_VLEV1 | NG_VPALL, 
                 "SCI INFO: SCIGetRemoteSegmentSize() of node %d: %d B", i, 
                 SCIGetRemoteSegmentSize (private_data.node[i].remote_seg_res));
      }
    }
  }
  // Any remote segments I could not connect to?
  if (num_errors != 0) {
    return 1;
  }

  /*
   * Map only those parts of the remote segments
   * into addressable space which are important to me
   */
  for (i = 0; i < private_data.worldsize; i++) {
    
    // Not my node id?
    if (i != private_data.worldrank) {
      private_data.node[i].remote_seg_addr =
        SCIMapRemoteSegment(private_data.node[i].remote_seg_res,
                            &private_data.node[i].remote_map,
                            private_data.node[i].remote_seg_offset,
                            private_data.remote_seg_mapped_size,
                            NO_SUGGESTED_ADDRESS,
                            NO_FLAGS,
                            &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCIMapRemoteSegment() to node %d "
                  "failed with error: %s", i, get_sci_error (error));
        return 1;
      }
    }
  }

  /* Set pointers according to the protocol to buffers in the remote segments */
  for (i = 0; i < private_data.worldsize; i++) {
   
    // Not my node id?
    if (i != private_data.worldrank) {
      private_data.node[i].remote_datab_addr =
        (u_int8_t *) private_data.node[i].remote_seg_addr;

      private_data.node[i].remote_ackb_addr =
        (u_int8_t *) private_data.node[i].remote_datab_addr +
        (private_data.remote_seg_mapped_size -
         private_data.info_dma_offset_alignment);

      private_data.node[i].remote_idb_addr =
        (u_int8_t *) private_data.node[i].remote_ackb_addr -
        private_data.info_dma_offset_alignment;
    }
  }

  /*
   * Create sequences for all remote segments
   * NOTE: Remote segments must be mapped into addressable space, first.
   */
  for (i = 0; i < private_data.worldsize; i++) {
    
    // Not my node id?
    if (i != private_data.worldrank)        {
      SCICreateMapSequence(private_data.node[i].remote_map,
                           &private_data.node[i].remote_seg_seq,
                           NO_FLAGS, &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCICreateMapSequence() to node %d "
                  "failed with error: %s", i, get_sci_error (error));
        return 1;
      }
    }
  }

  /* Start sequences for all remote segments */
  for (i = 0; i < private_data.worldsize; i++) {
    
    // Not my node id?
    if (i != private_data.worldrank) {
      do {
        status = SCIStartSequence (private_data.node[i].remote_seg_seq,
                                   NO_FLAGS, &error);
        if (error != SCI_ERR_OK) {
          ng_error ("SCIStartSequence() to node %d  "
                    "failed with error: %s",
                    i, get_sci_error (error));
        }
      } while (status != SCI_ERR_OK);
    }
  }

  return 0;
}

static inline int sendto_via_SCIMemCpy(int dst, void *buffer, int size) {
  int size_old = size;
  int size_to_send = 0;
  unsigned int alignment_displ, padding;
  Queue_element *queue_element_ptr = NULL;
  Sendq_data_element *data_ptr = NULL;
  sci_error_t error;
  void *src_buffer;
  
  /* Send queue empty and allowed to send? */
  if (!private_data.node[dst].send_queue.head &&
      *((volatile u_int8_t *) private_data.node[dst].local_ackb_addr) ==
      private_data.node[dst].next_ack_to_recv) {

    /* Size of useful data to be sent */
    size_to_send = min(size, private_data.protocol_mtu);

    /* Check "size_to_send" for SCI size alignment requirements */
    padding = 0;
    alignment_displ = size_to_send % private_data.info_dma_size_alignment;
    if (alignment_displ) {
      padding = private_data.info_dma_size_alignment - alignment_displ;
    }

    /* Check for SCI size and offset alignment requirements */
    if (padding ||  
        (unsigned long) buffer % (unsigned long) private_data.info_dma_offset_alignment) {

#if LOCAL_MEMCPY 
      /* Copy data to be sent to DMA buffer */ 
      memcpy(private_data.node[dst].local_dmab_addr, buffer, size_to_send);
#endif
    
      /* Buffer from which to copy is DMA buffer */
      src_buffer = private_data.node[dst].local_dmab_addr;
    }
    else {
      /* Buffer from which to copy is user buffer */
      src_buffer = buffer;
    }

    /* Update next ACK to receive */
    if (private_data.node[dst].next_ack_to_recv == 0)
      private_data.node[dst].next_ack_to_recv = 1;
    else
      private_data.node[dst].next_ack_to_recv = 0;

    /* Send data */
    SCIMemCpy(private_data.node[dst].remote_seg_seq,
              src_buffer,
              private_data.node[dst].remote_map,
              NO_OFFSET,
              size_to_send + padding,
              SCI_FLAG_ERROR_CHECK,
              &error);

#if SCI_RELIABLE
    /* Make sure that the DATA sent arrives */
    while (error == SCI_ERR_TRANSFER_FAILED) {
      SCIMemCpy(private_data.node[dst].remote_seg_seq,
                src_buffer,
                private_data.node[dst].remote_map,
                NO_OFFSET,
                size_to_send + padding,
                SCI_FLAG_ERROR_CHECK,
                &error);
    }
#endif

    if (error != SCI_ERR_OK) {
      ng_error("SCIMemCpy() failed with error: %s",
               get_sci_error (error));
      return -1;
    }

    /* Send ID, notifying the receiver that new data has arrived */
    *((volatile u_int8_t *) private_data.node[dst].remote_idb_addr) =
      private_data.node[dst].next_id_to_send;

#if SCI_RELIABLE
    /* Make sure that the ID sent arrives */
    assure_reliable_memcpy_transfer(dst,
                                    private_data.node[dst].remote_idb_addr,
                                    &private_data.node[dst].next_id_to_send,
                                    1);
#endif

    /* Update next ID to send */
    if (private_data.node[dst].next_id_to_send == 0)
      private_data.node[dst].next_id_to_send = 1;
    else
      private_data.node[dst].next_id_to_send = 0;

    /* Update input parameters */
    size -= size_to_send;
    buffer = (u_int8_t *) buffer + size_to_send;
  }

  /* 
   * Anything left to send?
   */
  if (size) {
    /* Allocate memory for request structure (new send queue element) */
    queue_element_ptr = (Queue_element *) malloc(sizeof(Queue_element));
    data_ptr = (Sendq_data_element *) malloc(sizeof(Sendq_data_element));
    if (!queue_element_ptr || !data_ptr) {
      ng_error("Could not allocate %d B for send queue element",
               sizeof(Queue_element) + sizeof(Sendq_data_element));
      return -1;
    }
  
    /* Init the new queue element */
    data_ptr->dst                   = dst;
    data_ptr->buffer                = buffer;
    data_ptr->size                  = size;
    data_ptr->payload_transfer_size = 0;
    data_ptr->DMA_transfer_size     = 0;
    data_ptr->DMA_buffer_sent       = 0;
    data_ptr->DMA_buffer_to_send    = 0;
    data_ptr->done                  = 0;
    queue_element_ptr->destroy         = destroy;
    queue_element_ptr->done            = done;
    queue_element_ptr->forward_via_DMA = forward_sendto_via_DMA;
    queue_element_ptr->forward_via_PIO = private_data.opt_pio_mode; 
    queue_element_ptr->data            = data_ptr;
    queue_element_ptr->next            = NULL;
    queue_element_ptr->queue           = &private_data.node[dst].send_queue;

    /* Enqueue the element in send queue */
    enqueue(&private_data.node[dst].send_queue, queue_element_ptr);

    /* Wait for completion */
    while(test_via_PIO((NG_Request *) &queue_element_ptr));
  }
    
  return size_old;
}

static inline int sendto_via_SCIMemWrite(int dst, void *buffer, int size) {
  int size_old = size;
  int size_to_send = 0;
  unsigned int alignment_displ, padding;
  Queue_element *queue_element_ptr = NULL;
  Sendq_data_element *data_ptr = NULL;
  sci_error_t error;
  void *src_buffer;
  
  /* Send queue empty and allowed to send? */
  if (!private_data.node[dst].send_queue.head &&
      *((volatile u_int8_t *) private_data.node[dst].local_ackb_addr) ==
      private_data.node[dst].next_ack_to_recv) {

    /* Size of useful data to be sent */
    size_to_send = min(size, private_data.protocol_mtu);

    /* Check "size_to_send" for SCI size alignment requirements */
    padding = 0;
    alignment_displ = size_to_send % private_data.info_dma_size_alignment;
    if (alignment_displ) {
      padding = private_data.info_dma_size_alignment - alignment_displ;
    }

    /* Check for SCI size and offset alignment requirements */
    if (padding ||  
        (unsigned long) buffer % (unsigned long) private_data.info_dma_offset_alignment) {

#if LOCAL_MEMCPY 
      /* Copy data to be sent to DMA buffer */ 
      memcpy(private_data.node[dst].local_dmab_addr, buffer, size_to_send);
#endif
    
      /* Buffer from which to copy is DMA buffer */
      src_buffer = private_data.node[dst].local_dmab_addr;
    }
    else {
      /* Buffer from which to copy is user buffer */
      src_buffer = buffer;
    }

    /* Update next ACK to receive */
    if (private_data.node[dst].next_ack_to_recv == 0)
      private_data.node[dst].next_ack_to_recv = 1;
    else
      private_data.node[dst].next_ack_to_recv = 0;

    /* Send data */
    SCIMemWrite(src_buffer,
                private_data.node[dst].remote_datab_addr,
                size_to_send + padding,
                NO_FLAGS,
                &error);
    if (error != SCI_ERR_OK) {
      ng_error ("SCIMemWrite() failed with error: %s",
                get_sci_error (error));
      return -1;
    }

#if SCI_RELIABLE
    /* Make sure that the DATA sent arrives */
    assure_reliable_SCIMemWrite_transfer(dst,
                                         private_data.node[dst].remote_datab_addr,
                                         src_buffer,
                                         size_to_send + padding);
#endif

    /* Send ID, notifying the receiver that new data has arrived */
    *((volatile u_int8_t *) private_data.node[dst].remote_idb_addr) =
      private_data.node[dst].next_id_to_send;

#if SCI_RELIABLE
    /* Make sure that the ID sent arrives */
    assure_reliable_memcpy_transfer(dst,
                                    private_data.node[dst].remote_idb_addr,
                                    &private_data.node[dst].next_id_to_send,
                                    1);
#endif

    /* Update next ID to send */
    if (private_data.node[dst].next_id_to_send == 0)
      private_data.node[dst].next_id_to_send = 1;
    else
      private_data.node[dst].next_id_to_send = 0;

    /* Update input parameters */
    size -= size_to_send;
    buffer = (u_int8_t *) buffer + size_to_send;
  }

  /* 
   * Anything left to send?
   */
  if (size) {
    /* Allocate memory for request structure (new send queue element) */
    queue_element_ptr = (Queue_element *) malloc(sizeof(Queue_element));
    data_ptr = (Sendq_data_element *) malloc(sizeof(Sendq_data_element));
    if (!queue_element_ptr || !data_ptr) {
      ng_error("Could not allocate %d B for send queue element",
               sizeof(Queue_element) + sizeof(Sendq_data_element));
      return -1;
    }
  
    /* Init the new queue element */
    data_ptr->dst                   = dst;
    data_ptr->buffer                = buffer;
    data_ptr->size                  = size;
    data_ptr->payload_transfer_size = 0;
    data_ptr->DMA_transfer_size     = 0;
    data_ptr->DMA_buffer_sent       = 0;
    data_ptr->DMA_buffer_to_send    = 0;
    data_ptr->done                  = 0;
    queue_element_ptr->destroy         = destroy;
    queue_element_ptr->done            = done;
    queue_element_ptr->forward_via_DMA = forward_sendto_via_DMA;
    queue_element_ptr->forward_via_PIO = private_data.opt_pio_mode; 
    queue_element_ptr->data            = data_ptr;
    queue_element_ptr->next            = NULL;
    queue_element_ptr->queue           = &private_data.node[dst].send_queue;

    /* Enqueue the element in send queue */
    enqueue(&private_data.node[dst].send_queue, queue_element_ptr);

    /* Wait for completion */
    while(test_via_PIO((NG_Request *) &queue_element_ptr));
  }
    
  return size_old;
}

static inline int sendto_via_SCITransferBlock(int dst, void *buffer, int size) {
  int size_old = size;
  int size_to_send = 0;
  unsigned int alignment_displ, padding;
  Queue_element *queue_element_ptr = NULL;
  Sendq_data_element *data_ptr = NULL;
  sci_error_t error;
  
  /* Send queue empty and allowed to send? */
  if (!private_data.node[dst].send_queue.head &&
      *((volatile u_int8_t *) private_data.node[dst].local_ackb_addr) ==
      private_data.node[dst].next_ack_to_recv) {

    /* Size of useful data to be sent */
    size_to_send = min(size, private_data.protocol_mtu);

    /* Check "size_to_send" for SCI size alignment requirements */
    padding = 0;
    alignment_displ = size_to_send % private_data.info_dma_size_alignment;
    if (alignment_displ) {
      padding = private_data.info_dma_size_alignment - alignment_displ;
    }

#if LOCAL_MEMCPY 
    /* Copy data to be sent to DMA buffer */ 
    memcpy(private_data.node[dst].local_dmab_addr, buffer, size_to_send);
#endif
    
    /* Update next ACK to receive */
    if (private_data.node[dst].next_ack_to_recv == 0)
      private_data.node[dst].next_ack_to_recv = 1;
    else
      private_data.node[dst].next_ack_to_recv = 0;

    /* Send data */
    SCITransferBlock(private_data.local_map_dma,
                     private_data.node[dst].local_dmab_addr -
                      private_data.local_seg_addr_dma,
                     private_data.node[dst].remote_map,
                     NO_OFFSET,
                     size_to_send + padding,
                     NO_FLAGS,
                     &error);
    if (error != SCI_ERR_OK) {
      ng_error ("SCITransferBlock() failed with error: %s",
                get_sci_error (error));
      return -1;
    }

#if SCI_RELIABLE
    /* Make sure that the DATA sent arrives */
    assure_reliable_SCITransferBlock_transfer(dst, NULL, NULL, size_to_send + padding);
#endif

    /* Send ID, notifying the receiver that new data has arrived */
    *((volatile u_int8_t *) private_data.node[dst].remote_idb_addr) =
      private_data.node[dst].next_id_to_send;

#if SCI_RELIABLE
    /* Make sure that the ID sent arrives */
    assure_reliable_memcpy_transfer(dst,
                                    private_data.node[dst].remote_idb_addr,
                                    &private_data.node[dst].next_id_to_send,
                                    1);
#endif

    /* Update next ID to send */
    if (private_data.node[dst].next_id_to_send == 0)
      private_data.node[dst].next_id_to_send = 1;
    else
      private_data.node[dst].next_id_to_send = 0;

    /* Update input parameters */
    size -= size_to_send;
    buffer = (u_int8_t *) buffer + size_to_send;
  }

  /* 
   * Anything left to send?
   */
  if (size) {
    /* Allocate memory for request structure (new send queue element) */
    queue_element_ptr = (Queue_element *) malloc(sizeof(Queue_element));
    data_ptr = (Sendq_data_element *) malloc(sizeof(Sendq_data_element));
    if (!queue_element_ptr || !data_ptr) {
      ng_error("Could not allocate %d B for send queue element",
               sizeof(Queue_element) + sizeof(Sendq_data_element));
      return -1;
    }
  
    /* Init the new queue element */
    data_ptr->dst                   = dst;
    data_ptr->buffer                = buffer;
    data_ptr->size                  = size;
    data_ptr->payload_transfer_size = 0;
    data_ptr->DMA_transfer_size     = 0;
    data_ptr->DMA_buffer_sent       = 0;
    data_ptr->DMA_buffer_to_send    = 0;
    data_ptr->done                  = 0;
    queue_element_ptr->destroy         = destroy;
    queue_element_ptr->done            = done;
    queue_element_ptr->forward_via_DMA = forward_sendto_via_DMA;
    queue_element_ptr->forward_via_PIO = private_data.opt_pio_mode; 
    queue_element_ptr->data            = data_ptr;
    queue_element_ptr->next            = NULL;
    queue_element_ptr->queue           = &private_data.node[dst].send_queue;

    /* Enqueue the element in send queue */
    enqueue(&private_data.node[dst].send_queue, queue_element_ptr);

    /* Wait for completion */
    while(test_via_PIO((NG_Request *) &queue_element_ptr));
  }
    
  return size_old;
}

static inline int sendto_via_memcpy(int dst, void *buffer, int size) {
  int size_old = size;
  int size_to_send = 0;
  Queue_element *queue_element_ptr = NULL;
  Sendq_data_element *data_ptr = NULL;
  
  /* Send queue empty and allowed to send? */
  if (!private_data.node[dst].send_queue.head &&
      *((volatile u_int8_t *) private_data.node[dst].local_ackb_addr) ==
      private_data.node[dst].next_ack_to_recv) {

    /* Size of useful data to be sent */
    size_to_send = min(size, private_data.protocol_mtu);

    /* Update next ACK to receive */
    if (private_data.node[dst].next_ack_to_recv == 0)
      private_data.node[dst].next_ack_to_recv = 1;
    else
      private_data.node[dst].next_ack_to_recv = 0;

    /* Send data */
    memcpy(private_data.node[dst].remote_datab_addr, buffer, size_to_send);

#if SCI_RELIABLE
    /* Make sure that the DATA sent arrives */
    assure_reliable_memcpy_transfer(dst,
                                    private_data.node[dst].remote_datab_addr,
                                    buffer,
                                    size_to_send);
#endif
    
    /* Send ID, notifying the receiver that new data has arrived */
    *((volatile u_int8_t *) private_data.node[dst].remote_idb_addr) =
      private_data.node[dst].next_id_to_send;

#if SCI_RELIABLE
    /* Make sure that the ID sent arrives */
    assure_reliable_memcpy_transfer(dst,
                                    private_data.node[dst].remote_idb_addr,
                                    &private_data.node[dst].next_id_to_send,
                                    1);
#endif

    /* Update next ID to send */
    if (private_data.node[dst].next_id_to_send == 0)
      private_data.node[dst].next_id_to_send = 1;
    else
      private_data.node[dst].next_id_to_send = 0;

    /* Update input parameters */
    size -= size_to_send;
    buffer = (u_int8_t *) buffer + size_to_send;
  }

  /* 
   * Anything left to send?
   */
  if (size) {
    /* Allocate memory for request structure (new send queue element) */
    queue_element_ptr = (Queue_element *) malloc(sizeof(Queue_element));
    data_ptr = (Sendq_data_element *) malloc(sizeof(Sendq_data_element));
    if (!queue_element_ptr || !data_ptr) {
      ng_error("Could not allocate %d B for send queue element",
               sizeof(Queue_element) + sizeof(Sendq_data_element));
      return -1;
    }
  
    /* Init the new queue element */
    data_ptr->dst                   = dst;
    data_ptr->buffer                = buffer;
    data_ptr->size                  = size;
    data_ptr->payload_transfer_size = 0;
    data_ptr->DMA_transfer_size     = 0;
    data_ptr->DMA_buffer_sent       = 0;
    data_ptr->DMA_buffer_to_send    = 0;
    data_ptr->done                  = 0;
    queue_element_ptr->destroy         = destroy;
    queue_element_ptr->done            = done;
    queue_element_ptr->forward_via_DMA = forward_sendto_via_DMA;
    queue_element_ptr->forward_via_PIO = private_data.opt_pio_mode; 
    queue_element_ptr->data            = data_ptr;
    queue_element_ptr->next            = NULL;
    queue_element_ptr->queue           = &private_data.node[dst].send_queue;

    /* Enqueue the element in send queue */
    enqueue(&private_data.node[dst].send_queue, queue_element_ptr);

    /* Wait for completion */
    while(test_via_PIO((NG_Request *) &queue_element_ptr));
  }
    
  return size_old;
}

static inline int my_recvfrom(int src, void *buffer, int size) {
  int size_old = size;
  int size_to_copy = 0;
  Queue_element *queue_element_ptr = NULL;
  Recvq_data_element *data_ptr = NULL;

  /*
   * Queue previously empty and
   * new data available?
   */
  if (!private_data.node[src].recv_queue.head &&
      *((volatile u_int8_t *) private_data.node[src].local_idb_addr) ==
      private_data.node[src].next_id_to_recv) {

    /* Update next ID to recv */
    if (private_data.node[src].next_id_to_recv == 0)
      private_data.node[src].next_id_to_recv = 1;
    else
      private_data.node[src].next_id_to_recv = 0;

    /* How much to copy to user buffer? */
    size_to_copy = min(size, private_data.protocol_mtu);

#if LOCAL_MEMCPY
    /* Copy received data to user buffer */
    memcpy(buffer, private_data.node[src].local_datab_addr, size_to_copy);
#endif

    /* Send ACK to sender */
    *((volatile u_int8_t *) private_data.node[src].remote_ackb_addr) =
      *((volatile u_int8_t *) private_data.node[src].local_idb_addr);

#if SCI_RELIABLE
    /* Make sure that the ACK sent arrives */
    assure_reliable_memcpy_transfer(src,
                                    private_data.node[src].remote_ackb_addr,
                                    private_data.node[src].local_idb_addr,
                                    1);
#endif

    /* Update input parameters */
    size -= size_to_copy;
    buffer = (u_int8_t *) buffer + size_to_copy;
  }

  /*
   * Anything left to receive?
   */
  if (size) {
    /* Allocate memory for request structure (new receive queue element) */
    queue_element_ptr = (Queue_element *) malloc(sizeof(Queue_element));
    data_ptr = (Recvq_data_element *) malloc(sizeof(Recvq_data_element));
    if (!queue_element_ptr || !data_ptr) {
      ng_error("Could not allocate %d B for receive queue element",
               sizeof(Queue_element) + sizeof(Recvq_data_element));
      return -1;
    }

    /* Init the new queue element */
    data_ptr->src    = src;
    data_ptr->buffer = buffer;
    data_ptr->size   = size;
    data_ptr->done   = 0;
    queue_element_ptr->destroy         = destroy;
    queue_element_ptr->done            = done;
    queue_element_ptr->forward_via_DMA = forward_recvfrom;
    queue_element_ptr->forward_via_PIO = forward_recvfrom;
    queue_element_ptr->data            = data_ptr;
    queue_element_ptr->next            = NULL;
    queue_element_ptr->queue           = &private_data.node[src].recv_queue;

    /* Enqueue the element in receive queue */
    enqueue(&private_data.node[src].recv_queue, queue_element_ptr);

    /* Wait for completion */
    while(test_via_PIO((NG_Request *) &queue_element_ptr));
  }

  return size_old;
}

static inline int my_isendto(int dst, void *buffer, int size, NG_Request * req) {
  int size_to_send = 0;
  unsigned int alignment_displ, padding;
  Queue_element *queue_element_ptr = NULL;
  Sendq_data_element *data_ptr = NULL;
  sci_error_t error;

  /* Allocate memory for request structure (new send queue element) */
  queue_element_ptr = (Queue_element *) malloc(sizeof(Queue_element));
  data_ptr = (Sendq_data_element *) malloc(sizeof(Sendq_data_element));
  if (!queue_element_ptr || !data_ptr) {
    ng_error("Could not allocate %d B for send queue element",
             sizeof(Queue_element) + sizeof(Sendq_data_element));
    return -1;
  }
  
  /* Init the new queue element */
  data_ptr->dst                   = dst;
  data_ptr->buffer                = buffer;
  data_ptr->size                  = size;
  data_ptr->payload_transfer_size = 0;
  data_ptr->DMA_transfer_size     = 0;
  data_ptr->DMA_buffer_sent       = 0;
  data_ptr->DMA_buffer_to_send    = 0;
  data_ptr->done                  = 0;
  queue_element_ptr->destroy         = destroy;
  queue_element_ptr->done            = done;
  queue_element_ptr->forward_via_DMA = forward_sendto_via_DMA;
  queue_element_ptr->forward_via_PIO = private_data.opt_pio_mode; 
  queue_element_ptr->data            = data_ptr;
  queue_element_ptr->next            = NULL;
  queue_element_ptr->queue           = &private_data.node[dst].send_queue;

  /* Queue previously empty? */
  if (!private_data.node[dst].send_queue.head) {

    /* How much to copy to DMA buffer? */
    size_to_send = min(size, private_data.protocol_mtu);

    /* 
     * Store the size of useful data to be sent in the 
     * next sub-transfer in the request structure
     */
    data_ptr->payload_transfer_size = size_to_send;

    /* Check "size_to_send" for SCI size alignment requirements */
    padding = 0;
    alignment_displ = size_to_send % private_data.info_dma_size_alignment;
    if (alignment_displ) {
      padding = private_data.info_dma_size_alignment - alignment_displ;
    }

    /* 
     * Store actual DMA transfer size of next 
     * sub-transfer in request structure 
     */
    data_ptr->DMA_transfer_size = size_to_send + padding;

#if LOCAL_MEMCPY
    /* Copy data to be sent to DMA buffer */ 
    memcpy(private_data.node[dst].local_dmab_addr, buffer, size_to_send);
#endif

    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[dst].local_ackb_addr) ==
        private_data.node[dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[dst].next_ack_to_recv == 0)
        private_data.node[dst].next_ack_to_recv = 1;
      else
        private_data.node[dst].next_ack_to_recv = 0;

      /* 
       * Send data via DMA
       */ 
      /* Specify DMA transfer */
      SCIEnqueueDMATransfer(private_data.node[dst].dma_queue,
                            private_data.local_seg_res_dma,
                            private_data.node[dst].remote_seg_res,
                            private_data.node[dst].local_dmab_addr -
                              private_data.local_seg_addr_dma,
                            private_data.node[dst].remote_seg_offset,
                            size_to_send + padding,
                            NO_FLAGS,
                            &error);
      if (error != SCI_ERR_OK) {
        ng_error("SCIEnqueueDMATransfer() failed with error: %s",
                 get_sci_error(error));
        return -1;
      }

      /* Start data transfer */
      SCIPostDMAQueue(private_data.node[dst].dma_queue,
                      NO_CALLBACK,
                      NO_ARG,
                      NO_FLAGS,
                      &error);
      if (error != SCI_ERR_OK) {
        ng_error("SCIPostDMAQueue() failed with error: %s",
                 get_sci_error(error));
        return -1;
      }

      /* Set "DMA_buffer_sent" for this request */
      data_ptr->DMA_buffer_sent = 1;
    }
    else {
      /* Set "DMA_buffer_to_send" for this request */
      data_ptr->DMA_buffer_to_send = 1;
    }
  }

  /* Enqueue the element in send queue */
  enqueue(&private_data.node[dst].send_queue, queue_element_ptr);

  /* Pointer to request structure */
  *req = queue_element_ptr;

  return 0;
}

static inline int my_irecvfrom (int src, void *buffer, int size, NG_Request *req) {
  int size_to_copy = 0;
  Queue_element *queue_element_ptr = NULL;
  Recvq_data_element *data_ptr = NULL;


  /* Allocate memory for request structure (new receive queue element) */
  queue_element_ptr = (Queue_element *) malloc(sizeof(Queue_element));
  data_ptr = (Recvq_data_element *) malloc(sizeof(Recvq_data_element));
  if (!queue_element_ptr || !data_ptr) {
    ng_error("Could not allocate %d B for receive queue element",
             sizeof(Queue_element) + sizeof(Recvq_data_element));
    return -1;
  }

  /* Init the new queue element */
  data_ptr->src    = src;
  data_ptr->buffer = buffer;
  data_ptr->size   = size;
  data_ptr->done   = 0;
  queue_element_ptr->destroy          = destroy;
  queue_element_ptr->done             = done;
  queue_element_ptr->forward_via_DMA  = forward_recvfrom;
  queue_element_ptr->forward_via_PIO  = forward_recvfrom;
  queue_element_ptr->data             = data_ptr;
  queue_element_ptr->next             = NULL;
  queue_element_ptr->queue            = &private_data.node[src].recv_queue;

  /*
   * Queue previously empty and
   * new data available?
   */
  if (!private_data.node[src].recv_queue.head &&
      *((volatile u_int8_t *) private_data.node[src].local_idb_addr) ==
      private_data.node[src].next_id_to_recv) {

    /* Update next ID to recv */
    if (private_data.node[src].next_id_to_recv == 0)
      private_data.node[src].next_id_to_recv = 1;
    else
      private_data.node[src].next_id_to_recv = 0;

    /* How much to copy to user buffer? */
    size_to_copy = min(size, private_data.protocol_mtu);

#if LOCAL_MEMCPY
    /* Copy received data to user buffer */
    memcpy(buffer, private_data.node[src].local_datab_addr, size_to_copy);
    data_ptr->buffer = (u_int8_t *) data_ptr->buffer + size_to_copy;
#endif

    /* Send ACK to sender */
    *((volatile u_int8_t *) private_data.node[src].remote_ackb_addr) =
      *((volatile u_int8_t *) private_data.node[src].local_idb_addr);

#if SCI_RELIABLE
    /* Make sure that the ACK sent arrives */
    assure_reliable_memcpy_transfer(src,
                                    private_data.node[src].remote_ackb_addr,
                                    private_data.node[src].local_idb_addr,
                                    1);
#endif

    data_ptr->size -= size_to_copy;
  }

  /* Something left to receive? */
  if (data_ptr->size) {

    /* Enqueue the element in receive queue */
    enqueue(&private_data.node[src].recv_queue, queue_element_ptr);

  }
  else {
    /*
     * Mark the request done, remove its pointer
     * to its queue and don't enqueue it in the 
     * receive queue
     */
    data_ptr->done = 1;
    queue_element_ptr->queue = NULL;
  }

  /* Pointer to request structure */
  *req = queue_element_ptr;

  /* Number of bytes received with the current call */
  return size_to_copy;
}

static inline int test_via_DMA(NG_Request *req) {
  Queue_element *element_ptr = (Queue_element *) (*req);
  Queue_element *head_element_ptr;
  Queue_element *cur_head_element_ptr;
  int ret_val;

  /* Request complete? */
  if (element_ptr->done(element_ptr)) {
      /* Free the request's memory */
      element_ptr->destroy(element_ptr);
      ret_val = 0;
  }
  else {
    head_element_ptr = element_ptr->queue->head;
    cur_head_element_ptr = element_ptr->queue->head;

    /* Try to forward the first element in the request's queue */
    ret_val = cur_head_element_ptr->forward_via_DMA(cur_head_element_ptr);

    /* No progress? */
    if (ret_val == 2) {
      do {
        /* Get the head of the next queue which is not empty */
        cur_head_element_ptr = get_next_non_empty_queues_head(cur_head_element_ptr);

        /* Try to forward that request */
        ret_val = cur_head_element_ptr->forward_via_DMA(cur_head_element_ptr);

      } 
      /* All queues considered or progress? */
      while (head_element_ptr != cur_head_element_ptr && ret_val == 2);
    }

    /* 
     * Is the last request tried to forward 
     * the request which was originally to be forwarded 
     * (i.e. the "req" input parameter) ? 
     */
    if (element_ptr == cur_head_element_ptr) {
      /* Request complete? */
      if (ret_val == 0) {
        /* Free the request's memory */
        element_ptr->destroy(element_ptr);
      }
    }
    else {
      /* 
       * The request tested for could not be finished,
       * so it must be in progress, still
       */
      ret_val = 1;
    }
  }
  return ret_val;
} 

static inline int test_via_PIO(NG_Request *req) {
  Queue_element *element_ptr = (Queue_element *) (*req);
  Queue_element *head_element_ptr;
  Queue_element *cur_head_element_ptr;
  int ret_val;

  /* Request complete? */
  if (element_ptr->done(element_ptr)) {
      /* Free the request's memory */
      element_ptr->destroy(element_ptr);
      ret_val = 0;
  }
  else {
    head_element_ptr = element_ptr->queue->head;
    cur_head_element_ptr = element_ptr->queue->head;

    /* Try to forward the first element in the request's queue */
    ret_val = cur_head_element_ptr->forward_via_PIO(cur_head_element_ptr);

    /* No progress? */
    if (ret_val == 2) {
      do {
        /* Get the head of the next queue which is not empty */
        cur_head_element_ptr = get_next_non_empty_queues_head(cur_head_element_ptr);

        /* Try to forward that request */
        ret_val = cur_head_element_ptr->forward_via_PIO(cur_head_element_ptr);

      } 
      /* All queues considered or progress? */
      while (head_element_ptr != cur_head_element_ptr && ret_val == 2);
    }

    /* 
     * Is the last request tried to forward 
     * the request which was originally to be forwarded 
     * (i.e. the "req" input parameter) ? 
     */
    if (element_ptr == cur_head_element_ptr) {
      /* Request complete? */
      if (ret_val == 0) {
        /* Free the request's memory */
        element_ptr->destroy(element_ptr);
      }
    }
    else {
      /* 
       * The request tested for could not be finished,
       * so it must be in progress, still
       */
      ret_val = 1;
    }
  }
  return ret_val;
} 

static inline void my_shutdown (struct ng_options *global_opts) {
  sci_error_t error;
  int i;

  /* Remove sequences for all remote segments */
  for (i = 0; i < private_data.worldsize; i++) {
    
    // Not my node id?
    if (i != private_data.worldrank) {
      SCIRemoveSequence (private_data.node[i].remote_seg_seq,
                         NO_FLAGS, &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCIRemoveSequence() to node %d "
                  "failed with error: %s", i, get_sci_error (error));
      }
    }
  }

  /* Unmap remote segments from addressable space */
  for (i = 0; i < private_data.worldsize; i++) {
    // Not my node id?
    if (i != private_data.worldrank) {
      SCIUnmapSegment (private_data.node[i].remote_map, NO_FLAGS, &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCIUnmapSegment() for remote segment failed with error: "
                  "%s", get_sci_error (error));
      }
    }
  }

  /* Disconnect from all remote segments in the SCI network */
  for (i = 0; i < private_data.worldsize; i++) {
    // Not my node id?
    if (i != private_data.worldrank) {
      SCIDisconnectSegment (private_data.node[i].remote_seg_res,
                            NO_FLAGS, &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCIDisconnectSegment() to node %d "
                  "failed with error: %s", i, get_sci_error (error));
      }
    }
  }

  /* Wait until all nodes have disconnected from all remote segments */
  MPI_Barrier (MPI_COMM_WORLD);

  /* Remove DMA queues to all nodes */
  for (i = 0; i < private_data.worldsize; i++) {

    // Not my node id?
    if (i != private_data.worldrank) {

      SCIRemoveDMAQueue(private_data.node[i].dma_queue,
                        NO_FLAGS,
                        &error);
      if (error != SCI_ERR_OK) {
        ng_error("SCIRemoveDMAQueue() to node %d failed with error: %s",
                 i, get_sci_error (error));
      }
    }
  }

  /* Unmap local DMA segment from addressable space */
  SCIUnmapSegment (private_data.local_map_dma, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIUnmapSegment() for local DMA segment failed with error: %s",
              get_sci_error (error));
  }

  /* Remove local DMA segment */
  SCIRemoveSegment (private_data.local_seg_res_dma, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIRemoveSegment() for local DMA segment failed with error: %s",
      get_sci_error (error));
  }

  /* Make local segment unavailable to the SCI network */
  SCISetSegmentUnavailable (private_data.local_seg_res, ADAPTER_NO, NO_FLAGS,
                            &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCISetSegmentUnavailable() failed with error: %s",
              get_sci_error (error));
  }

  /* Unmap local segment from addressable space */
  SCIUnmapSegment (private_data.local_map, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIUnmapSegment() for local segment failed with error: %s",
              get_sci_error (error));
  }

  /* Remove local segment */
  SCIRemoveSegment (private_data.local_seg_res, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIRemoveSegment() for local segment failed with error: %s",
      get_sci_error (error));
  }

  /* Shutdown the SCI environment */
  // Close virtual SCI device for local segment
  SCIClose (private_data.v_dev, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIClose() for local device failed with error: %s",
              get_sci_error (error));
  }
  // Close virtual SCI device for local DMA segment
  SCIClose (private_data.v_dev_dma, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error ("SCIClose() for local DMA device failed with error: %s",
              get_sci_error (error));
  }
  // Close virtual SCI devices for remote segments from all SCI nodes
  for (i = 0; i < private_data.worldsize; i++) {
    if (i != private_data.worldrank) {
      SCIClose (private_data.node[i].v_dev, NO_FLAGS, &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCIClose() for remote device failed with error: "
                  "%s", get_sci_error (error));
      }
    }
  }
  SCITerminate ();

  /* Free memory of array with information for communicating with every SCI node */
  free(private_data.node);
}

static inline void my_usage(void) {
  ng_option_usage("-M", "Bytes of memory available for creating "
                        "(local) SCI segments",
                        "bytes");       
  ng_option_usage("-P", "PIO (Programmed I/O) mode for PIO data transmissions:", "");
  ng_option_usage(""  , "     SISCI's SCIMemCpy() function",           "SCIMemCpy");
  ng_option_usage(""  , "     SISCI's SCIMemWrite() function",         "SCIMemWrite"); 
  ng_option_usage(""  , " SISCI's SCITransferBlock() function",        "SCITransferBlock"); 
  ng_option_usage(""  , "     C Standard Library's memcpy() function", "memcpy");
} 

/*
 * Returns pointer to an array of characters containing an
 * SCI specific error message determined by the parameter error.
 *
 * @param error, SCI error code
 *
 * @return, pointer to a static array of characters with human 
 *          readable error message determined by error
 */ 
static inline char *get_sci_error(int error) {
  static char *e    = "unknown error";
  static char *e0   = "SCI_ERR_NOT_IMPLEMENTED";
  static char *e1   = "SCI_ERR_ILLEGAL_FLAG";
  static char *e2   = "SCI_ERR_FLAG_NOT_IMPLEMENTED";
  static char *e3   = "SCI_ERR_ILLEGAL_PARAMETER";
  static char *e4   = "SCI_ERR_NOSPC";
  static char *e5   = "SCI_ERR_API_NOSPC";
  static char *e6   = "SCI_ERR_HW_NOSPC";
  static char *e7   = "SCI_ERR_SYSTEM";
  static char *e8   = "SCI_ERR_ILLEGAL_ADAPTERNO";
  static char *e9   = "SCI_ERR_NO_SUCH_ADAPTERNO";
  static char *e10  = "SCI_ERR_NO_SUCH_NODEID";
  static char *e11  = "SCI_ERR_ILLEGAL_NODEID";
  static char *e12  = "SCI_ERR_OUT_OF_RANGE";
  static char *e13  = "SCI_ERR_SIZE_ALIGNMENT";
  static char *e14  = "SCI_ERR_OFFSET_ALIGNMENT";
  static char *e15  = "SCI_ERR_TRANSFER_FAILED";
  static char *e16  = "SCI_ERR_NO_SUCH_SEGMENT";
  static char *e17  = "SCI_ERR_CONNECTION_REFUSED";
  static char *e18  = "SCI_ERR_TIMEOUT";
  static char *e19  = "SCI_ERR_NO_LINK_ACCESS";
  static char *e20  = "SCI_ERR_NO_REMOTE_LINK_ACCESS";
  static char *e21  = "SCI_ERR_INCONSISTENT_VERSIONS";
  static char *e22  = "SCI_ERR_NOT_INITIALIZED";
  static char *e23  = "SCI_ERR_SEGMENTID_USED";
  static char *e24  = "SCI_ERR_BUSY";
  static char *e25  = "SCI_ERR_SEGMENT_NOT_PREPARED";
  static char *e26  = "SCI_ERR_ILLEGAL_OPERATION";
  static char *e27  = "SCI_ERR_MAX_ENTRIES";
  static char *e28  = "SCI_ERR_SEGMENT_NOT_CONNECTED";

  switch (error) {
    case SCI_ERR_NOT_IMPLEMENTED:
      return e0;
      break;
    case SCI_ERR_ILLEGAL_FLAG:
      return e1;
      break;
    case SCI_ERR_FLAG_NOT_IMPLEMENTED:
      return e2;
      break;
    case SCI_ERR_ILLEGAL_PARAMETER:
      return e3;
      break;
    case SCI_ERR_NOSPC:
      return e4;
      break;
    case SCI_ERR_API_NOSPC:
      return e5;
      break;
    case SCI_ERR_HW_NOSPC:
      return e6;
      break;
    case SCI_ERR_SYSTEM:
      return e7;
      break;
    case SCI_ERR_ILLEGAL_ADAPTERNO:
      return e8;
      break;
    case SCI_ERR_NO_SUCH_ADAPTERNO:
      return e9;
      break;
    case SCI_ERR_NO_SUCH_NODEID:
      return e10;
      break;
    case SCI_ERR_ILLEGAL_NODEID:
      return e11;
      break;
    case SCI_ERR_OUT_OF_RANGE:
      return e12;
      break;
    case SCI_ERR_SIZE_ALIGNMENT:
      return e13;
      break;
    case SCI_ERR_OFFSET_ALIGNMENT:
      return e14;
      break;
    case SCI_ERR_TRANSFER_FAILED:
      return e15;
      break;
    case SCI_ERR_NO_SUCH_SEGMENT:
      return e16;
      break;
    case SCI_ERR_CONNECTION_REFUSED:
      return e17;
      break;
    case SCI_ERR_TIMEOUT:
      return e18;
      break;
    case SCI_ERR_NO_LINK_ACCESS:
      return e19;
      break;
    case SCI_ERR_NO_REMOTE_LINK_ACCESS:
      return e20;
      break;
    case SCI_ERR_INCONSISTENT_VERSIONS:
      return e21;
      break;
    case SCI_ERR_NOT_INITIALIZED:
      return e22;
      break;
    case SCI_ERR_SEGMENTID_USED:
      return e23;
      break;
    case SCI_ERR_BUSY:
      return e24;
      break;
    case SCI_ERR_SEGMENT_NOT_PREPARED:
      return e25;
      break;
    case SCI_ERR_ILLEGAL_OPERATION:
      return e26;
      break;
    case SCI_ERR_MAX_ENTRIES:
      return e27;
      break;
    case SCI_ERR_SEGMENT_NOT_CONNECTED:
      return e28;
      break;
    default:
      return e;
  }
}

/*
 * Called after a memcpy() data transfer to check
 * if the data sent arrived correctly, or in case of error
 * to resend the data until success.
 *
 * @param dst, destination node id 
 * @param dst_buf, address of the remote buffer where to store the data
 * @param src_buf, address of the local buffer 
 * @param size, bytes of data to send
 */ 
static inline int assure_reliable_memcpy_transfer(int dst, void  *dst_buf, void *src_buf, int size) {
  sci_sequence_status_t status;
  sci_error_t error;

  /* Check for transfer errors */
  status = SCICheckSequence(private_data.node[dst].remote_seg_seq, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error("SCICheckSequence() failed with error: %s",
             get_sci_error (error));
    return -1;
  }

  while (status != SCI_SEQ_OK) {

    ng_info (NG_VNORM | NG_VPALL, "SCI INFO: Data transmission error with memcpy()");

    switch (status) {
      case SCI_SEQ_RETRIABLE:
        /*
         * The transfer failed due to a non-fatal error
         * (e.g. system busy because of heavy traffic) but
         * can be immediately retried
         */
        break;
      case SCI_SEQ_PENDING:
        /*
         * The transfer failed but the driver hasn't been able
         * yet to determine the severity of the error (if fatal or non-fatal);
         * SCIStartSequence() must be called until it succeeds
         */
      case SCI_SEQ_NOT_RETRIABLE:
        /*
         * The transfer failed due to a fatal error (e.g. cable unplugged)
         * and can be retried only after a successful call to
         * SCIStartSequence()
         */
        do {
          status = SCIStartSequence(private_data.node[dst].remote_seg_seq,
                                    NO_FLAGS, &error);
          if (error != SCI_ERR_OK) {
            ng_error("SCIStartSequence() failed with error: %s",
                     get_sci_error (error));
            return -1;
          }
        } while (status != SCI_SEQ_OK);
        break;
      default:
        ng_error ("Status of sequence unknown");
        return -1;
    }

    /* Transfer data again */
    memcpy(dst_buf, src_buf, size);

    /* Check for errors again */
    status = SCICheckSequence(private_data.node[dst].remote_seg_seq,
                              NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
      ng_error ("SCICheckSequence() failed with error: %s",
                get_sci_error (error));
      return -1;
    }
  }
  return 0;
}

/*
 * Called after an SCIMemWrite() data transfer to check
 * if the data sent arrived correctly, or in case of error
 * to resend the data until success.
 *
 * @param dst, destination node id 
 * @param dst_buf, address of the remote buffer where to store the data
 * @param src_buf, address of the local buffer 
 * @param size, bytes of data to send
 */ 
static inline int assure_reliable_SCIMemWrite_transfer(int dst, void  *dst_buf, void *src_buf, int size) {
  sci_sequence_status_t status;
  sci_error_t error;

  /* Check for transfer errors */
  status = SCICheckSequence(private_data.node[dst].remote_seg_seq, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error("SCICheckSequence() failed with error: %s",
             get_sci_error (error));
    return -1;
  }

  while (status != SCI_SEQ_OK) {

    ng_info (NG_VNORM | NG_VPALL, "SCI INFO: Data transmission error with SCIMemWrite()");

    switch (status) {
      case SCI_SEQ_RETRIABLE:
        /*
         * The transfer failed due to a non-fatal error
         * (e.g. system busy because of heavy traffic) but
         * can be immediately retried
         */
        break;
      case SCI_SEQ_PENDING:
        /*
         * The transfer failed but the driver hasn't been able
         * yet to determine the severity of the error (if fatal or non-fatal);
         * SCIStartSequence() must be called until it succeeds
         */
      case SCI_SEQ_NOT_RETRIABLE:
        /*
         * The transfer failed due to a fatal error (e.g. cable unplugged)
         * and can be retried only after a successful call to
         * SCIStartSequence()
         */
        do {
          status = SCIStartSequence(private_data.node[dst].remote_seg_seq,
                                    NO_FLAGS, &error);
          if (error != SCI_ERR_OK) {
            ng_error("SCIStartSequence() failed with error: %s",
                     get_sci_error (error));
            return -1;
          }
        } while (status != SCI_SEQ_OK);
        break;
      default:
        ng_error ("Status of sequence unknown");
        return -1;
    }

    /* Transfer data again */
    SCIMemWrite(src_buf,
                dst_buf,
                size,
                NO_FLAGS,
                &error);
    if (error != SCI_ERR_OK) {
      ng_error ("SCIMemWrite() failed with error: %s",
                get_sci_error (error));
      return -1;
    }

    /* Check for errors again */
    status = SCICheckSequence(private_data.node[dst].remote_seg_seq,
                              NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
      ng_error ("SCICheckSequence() failed with error: %s",
                get_sci_error (error));
      return -1;
    }
  }
  return 0;
}

/*
 * Called after an SCITransferBlock() data transfer to check
 * if the data sent arrived correctly, or in case of error
 * to resend the data until success.
 *
 * @param dst, destination node id 
 * @param dst_buf, NULL (i.e. not used)
 * @param src_buf, NULL (i.e. not used) 
 * @param size, bytes of data to send
 */ 
static inline int assure_reliable_SCITransferBlock_transfer(int dst, void  *dst_buf, void *src_buf, int size) {
  sci_sequence_status_t status;
  sci_error_t error;

  /* Check for transfer errors */
  status = SCICheckSequence(private_data.node[dst].remote_seg_seq, NO_FLAGS, &error);
  if (error != SCI_ERR_OK) {
    ng_error("SCICheckSequence() failed with error: %s",
             get_sci_error (error));
    return -1;
  }

  while (status != SCI_SEQ_OK) {

    ng_info (NG_VNORM | NG_VPALL, "SCI INFO: Data transmission error with SCITransferBlock()");

    switch (status) {
      case SCI_SEQ_RETRIABLE:
        /*
         * The transfer failed due to a non-fatal error
         * (e.g. system busy because of heavy traffic) but
         * can be immediately retried
         */
        break;
      case SCI_SEQ_PENDING:
        /*
         * The transfer failed but the driver hasn't been able
         * yet to determine the severity of the error (if fatal or non-fatal);
         * SCIStartSequence() must be called until it succeeds
         */
      case SCI_SEQ_NOT_RETRIABLE:
        /*
         * The transfer failed due to a fatal error (e.g. cable unplugged)
         * and can be retried only after a successful call to
         * SCIStartSequence()
         */
        do {
          status = SCIStartSequence(private_data.node[dst].remote_seg_seq,
                                    NO_FLAGS, &error);
          if (error != SCI_ERR_OK) {
            ng_error("SCIStartSequence() failed with error: %s",
                     get_sci_error (error));
            return -1;
          }
        } while (status != SCI_SEQ_OK);
        break;
      default:
        ng_error ("Status of sequence unknown");
        return -1;
    }

    /* Transfer data again */
    SCITransferBlock(private_data.local_map_dma,
                     private_data.node[dst].local_dmab_addr -
                       private_data.local_seg_addr_dma,
                     private_data.node[dst].remote_map,
                     NO_OFFSET,
                     size,
                     NO_FLAGS,
                     &error);
    if (error != SCI_ERR_OK) {
      ng_error ("SCITransferBlock() failed with error: %s",
                get_sci_error (error));
      return -1;
    }

    /* Check for errors again */
    status = SCICheckSequence(private_data.node[dst].remote_seg_seq,
                              NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
      ng_error ("SCICheckSequence() failed with error: %s",
                get_sci_error (error));
      return -1;
    }
  }
  return 0;
}

/*
 * Enqueues the element pointed to by element into the queue
 * pointed to by queue.
 *
 * @param queue, pointer to a queue
 * @param element, pointer to a queue element
 */ 
static inline void enqueue(Queue *queue, Queue_element *element) {
  /* Queue empty? */
  if (!queue->head) {
    queue->head = element;
  }
  else {
    /* Update old tail element's "next" pointer */
    queue->tail->next = element;
  }
  queue->tail = element;
}

/*
 * Dequeues the first element of the queue pointed to by
 * the input parameter queue.
 *
 * @param queue, pointer to a queue
 */ 
static inline void dequeue(Queue *queue) {
  /* Queue not empty? */
  if (queue->head) {
    queue->head = queue->head->next;

    /* Queue empty now? */
    if (!queue->head) {
      queue->tail = NULL;
    }
  }
}

/*
 * Returns pointer to the head element of the next non-empty queue.
 * It is assumed that there is a non-empty queue, otherwise the
 * input parameter element could not point to a valid queue element.
 * 
 * @param element, pointer (!= NULL) to a queue element 
 */
static inline Queue_element *get_next_non_empty_queues_head(Queue_element *element) {
  Queue *queue = element->queue;
  
  /* Get the next queue which is not empty */
  queue = get_next_non_empty_queue(queue);

  /* Return its head element */
  return queue->head;
}

/*
 * Returns pointer to the next queue which is not empty.
 * If there is no non-empty queue it returns NULL.
 *
 * @param queue, pointer to a queue which might by empty
 *
 * @return, pointer to a non-empty queue (might be the same
 *          as the queue pointed to by the input parameter)
 *          if possible, otherwise NULL.
 */ 
static inline Queue *get_next_non_empty_queue(Queue *queue) {
  Queue *cur_queue = queue;
  Queue *ret_val;
  
  do {    
    cur_queue = cur_queue->next;
  }  
  /* Finish when all queues considered or non-empty queue found */
  while (cur_queue != queue && !cur_queue->head);

  /* Queue not empty? */
  if (cur_queue->head)
    ret_val = cur_queue;
  else
    ret_val = NULL;
  
  return ret_val;
}

/*
 * Tries to forward a send request pointed to by element. 
 * Any data transfers are done via DMA.
 * ONLY apply to the first request in a queue 
 * because otherwise a request could be finished
 * or send data out of order.
 * 
 * @param element, pointer to a send request
 * @return 0, the request is done and dequeued from its queue
 *         1, the request is still in progress
 *         2, no progress
 */
static inline int forward_sendto_via_DMA(Queue_element *element) {
  Sendq_data_element *data_ptr = (Sendq_data_element *) element->data;
  sci_dma_queue_state_t dma_queue_state;
  sci_error_t error;
  int ret_val;
  int size_to_send;
  unsigned int alignment_displ, padding;

  /* Assume no progress when trying to forward the request */
  ret_val = 2;

  /* 
   * DMA queue posted? 
   */
  if (data_ptr->DMA_buffer_sent) {
   
    /* Check DMA queue state */
    dma_queue_state =
      SCIDMAQueueState(private_data.node[data_ptr->dst].dma_queue);
    
    switch (dma_queue_state) {
      /* 
       * Transfer has completed successfully
       */ 
      case SCI_DMAQUEUE_DONE:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size   -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /* 
       * Post failed transfer again
       */
      case SCI_DMAQUEUE_ERROR:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Specify DMA transfer */
        SCIEnqueueDMATransfer(private_data.node[data_ptr->dst].dma_queue,
                              private_data.local_seg_res_dma,
                              private_data.node[data_ptr->dst].remote_seg_res,
                              private_data.node[data_ptr->dst].local_dmab_addr -
                                private_data.local_seg_addr_dma,
                              private_data.node[data_ptr->dst].remote_seg_offset,
                              data_ptr->DMA_transfer_size,
                              NO_FLAGS,
                              &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIEnqueueDMATransfer() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }

        /* Start data transfer */
        SCIPostDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                        NO_CALLBACK,
                        NO_ARG,
                        NO_FLAGS,
                        &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIPostDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        break;
      /*
       * DMA queue is still being processed
       */
      case SCI_DMAQUEUE_POSTED:
        break;
      default:
        ng_error("Unexpected DMA queue state to node %d", data_ptr->dst);
    }

    /* Request still in progress */
    ret_val = 1;
  }
  /*
   * DMA buffer needs to be sent
   */
  else if (data_ptr->DMA_buffer_to_send) {
    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Specify DMA transfer */
      SCIEnqueueDMATransfer(private_data.node[data_ptr->dst].dma_queue,
                            private_data.local_seg_res_dma,
                            private_data.node[data_ptr->dst].remote_seg_res,
                            private_data.node[data_ptr->dst].local_dmab_addr -
                              private_data.local_seg_addr_dma,
                            private_data.node[data_ptr->dst].remote_seg_offset,
                            data_ptr->DMA_transfer_size,
                            NO_FLAGS,
                            &error);
      if (error != SCI_ERR_OK) {
        ng_error("SCIEnqueueDMATransfer() failed with error: %s",
                 get_sci_error(error));
        return -1;
      }

      /* Start data transfer */
      SCIPostDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                      NO_CALLBACK,
                      NO_ARG,
                      NO_FLAGS,
                      &error);
      if (error != SCI_ERR_OK) {
        ng_error("SCIPostDMAQueue() failed with error: %s",
                 get_sci_error(error));
        return -1;
      }

      /* Reset "DMA_buffer_to_send" for this request */
      data_ptr->DMA_buffer_to_send = 0;

      /* Set "DMA_buffer_sent" for this request */
      data_ptr->DMA_buffer_sent = 1;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Anything left to send?
   */
  else if (data_ptr->size) {
    /* How much to copy to DMA buffer? */
    size_to_send = min(data_ptr->size, private_data.protocol_mtu);

    /* 
     * Store the size of useful data to be sent in the 
     * next sub-transfer in the request structure
     */
    data_ptr->payload_transfer_size = size_to_send;

    /* Check "size_to_send" for SCI size alignment requirements */
    padding = 0;
    alignment_displ = size_to_send % private_data.info_dma_size_alignment;
    if (alignment_displ) {
      padding = private_data.info_dma_size_alignment - alignment_displ;
    }

    /* 
     * Store actual DMA transfer size of next 
     * sub-transfer in request structure 
     */
    data_ptr->DMA_transfer_size = size_to_send + padding;

#if LOCAL_MEMCPY 
    /* Copy data to be sent to DMA buffer */ 
    memcpy(private_data.node[data_ptr->dst].local_dmab_addr, data_ptr->buffer, size_to_send);
#endif

    /* Set "DMA_buffer_to_send" in request structure */
    data_ptr->DMA_buffer_to_send = 1;
    
    /* Request still in progress */
    ret_val = 1;
  }
  /*
   * Nothing left to send for the request
   */
  else {
    /*
     * Mark it done, remove its pointer
     * to its queue and dequeue it, but
     * don't free its memory
     */
    data_ptr->done = 1;
    dequeue(element->queue);
    element->queue = NULL;

    /* Request complete */
    ret_val = 0;
  }

  return ret_val;
}

/*
 * Tries to forward a send request pointed to by element. 
 * Any data transfers are done via SCIMemCpy().
 * ONLY apply to the first request in a queue 
 * because otherwise a request could be finished
 * or send data out of order.
 * 
 * @param element, pointer to a send request
 * @return 0, the request is done and dequeued from its queue
 *         1, the request is still in progress
 *         2, no progress
 */
static inline int forward_sendto_via_SCIMemCpy(Queue_element *element) {
  Sendq_data_element *data_ptr = (Sendq_data_element *) element->data;
  sci_dma_queue_state_t dma_queue_state;
  sci_error_t error;
  int ret_val;
  int size_to_send;
  unsigned int alignment_displ, padding;
  void *src_buffer;

  /* Assume no progress when trying to forward the request */
  ret_val = 2;

  /* 
   * DMA queue posted? 
   */
  if (data_ptr->DMA_buffer_sent) {
   
    /* Check DMA queue state */
    dma_queue_state =
      SCIDMAQueueState(private_data.node[data_ptr->dst].dma_queue);
    
    switch (dma_queue_state) {
      /* 
       * Transfer has completed successfully
       */ 
      case SCI_DMAQUEUE_DONE:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size   -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /* 
       * Do failed transfer again
       */
      case SCI_DMAQUEUE_ERROR:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send data */
        SCIMemCpy(private_data.node[data_ptr->dst].remote_seg_seq,
                  private_data.node[data_ptr->dst].local_dmab_addr,
                  private_data.node[data_ptr->dst].remote_map,
                  NO_OFFSET,
                  data_ptr->DMA_transfer_size,
                  SCI_FLAG_ERROR_CHECK,
                  &error);

#if SCI_RELIABLE
        /* Make sure that the DATA sent arrives */
        while (error == SCI_ERR_TRANSFER_FAILED) {
          SCIMemCpy(private_data.node[data_ptr->dst].remote_seg_seq,
                    private_data.node[data_ptr->dst].local_dmab_addr,
                    private_data.node[data_ptr->dst].remote_map,
                    NO_OFFSET,
                    data_ptr->DMA_transfer_size,
                    SCI_FLAG_ERROR_CHECK,
                    &error);
        }
#endif

        if (error != SCI_ERR_OK) {
          ng_error("SCIMemCpy() failed with error: %s",
                   get_sci_error (error));
          return -1;
        }

        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /*
       * DMA queue is still being processed
       */
      case SCI_DMAQUEUE_POSTED:
        break;
      default:
        ng_error("Unexpected DMA queue state to node %d", data_ptr->dst);
    }

    /* Request still in progress */
    ret_val = 1;
  }
  /*
   * DMA buffer needs to be sent
   */
  else if (data_ptr->DMA_buffer_to_send) {
    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Send data */
      SCIMemCpy(private_data.node[data_ptr->dst].remote_seg_seq,
                private_data.node[data_ptr->dst].local_dmab_addr,
                private_data.node[data_ptr->dst].remote_map,
                NO_OFFSET,
                data_ptr->DMA_transfer_size,
                SCI_FLAG_ERROR_CHECK,
                &error);

#if SCI_RELIABLE
      /* Make sure that the DATA sent arrives */
      while (error == SCI_ERR_TRANSFER_FAILED) {
        SCIMemCpy(private_data.node[data_ptr->dst].remote_seg_seq,
                  private_data.node[data_ptr->dst].local_dmab_addr,
                  private_data.node[data_ptr->dst].remote_map,
                  NO_OFFSET,
                  data_ptr->DMA_transfer_size,
                  SCI_FLAG_ERROR_CHECK,
                  &error);
      }
#endif

      if (error != SCI_ERR_OK) {
        ng_error("SCIMemCpy() failed with error: %s",
                 get_sci_error (error));
        return -1;
      }

      /* Send ID, notifying the receiver that new data has arrived */
      *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
        private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
      /* Make sure that the ID sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_idb_addr,
                                       &private_data.node[data_ptr->dst].next_id_to_send,
                                       1);
#endif

      /* Update next ID to send */
      if (private_data.node[data_ptr->dst].next_id_to_send == 0)
        private_data.node[data_ptr->dst].next_id_to_send = 1;
      else
        private_data.node[data_ptr->dst].next_id_to_send = 0;

      /* Update request structure */
      data_ptr->DMA_buffer_to_send = 0;
      data_ptr->size -= data_ptr->payload_transfer_size;
      data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                         data_ptr->payload_transfer_size;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Anything left to send?
   */
  else if (data_ptr->size) {
    /* Size of useful data to be sent in the next sub-transfer */
    size_to_send = min(data_ptr->size, private_data.protocol_mtu);

    /* Check "size_to_send" for SCI size alignment requirements */
    padding = 0;
    alignment_displ = size_to_send % private_data.info_dma_size_alignment;
    if (alignment_displ) {
      padding = private_data.info_dma_size_alignment - alignment_displ;
    }

    /* Check for SCI size and offset alignment requirements */
    if (alignment_displ ||  
        (unsigned long) data_ptr->buffer % (unsigned long) private_data.info_dma_offset_alignment) {
      
      /* 
       * Store the size of useful data to be sent in the 
       * next sub-transfer in the request structure
       */
      data_ptr->payload_transfer_size = size_to_send;

      /* 
       * Store actual DMA transfer size of next 
       * sub-transfer in request structure 
       */
      data_ptr->DMA_transfer_size = size_to_send + padding;

#if LOCAL_MEMCPY 
      /* Copy data to be sent to DMA buffer */ 
      memcpy(private_data.node[data_ptr->dst].local_dmab_addr, data_ptr->buffer, size_to_send);
#endif

      /* Set "DMA_buffer_to_send" in request structure */
      data_ptr->DMA_buffer_to_send = 1;

      /* Buffer from which to copy is DMA buffer*/
      src_buffer = private_data.node[data_ptr->dst].local_dmab_addr;
    }
    else {
      /* Buffer from which to copy is user buffer */
      src_buffer = data_ptr->buffer;
    }

    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Send data */
      SCIMemCpy(private_data.node[data_ptr->dst].remote_seg_seq,
                src_buffer,
                private_data.node[data_ptr->dst].remote_map,
                NO_OFFSET,
                size_to_send + padding,
                SCI_FLAG_ERROR_CHECK,
                &error);

#if SCI_RELIABLE
      /* Make sure that the DATA sent arrives */
      while (error == SCI_ERR_TRANSFER_FAILED) {
        SCIMemCpy(private_data.node[data_ptr->dst].remote_seg_seq,
                  src_buffer,
                  private_data.node[data_ptr->dst].remote_map,
                  NO_OFFSET,
                  size_to_send + padding,
                  SCI_FLAG_ERROR_CHECK,
                  &error);
      }
#endif

      if (error != SCI_ERR_OK) {
        ng_error("SCIMemCpy() failed with error: %s",
                 get_sci_error (error));
        return -1;
      }

      /* Send ID, notifying the receiver that new data has arrived */
      *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
        private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
      /* Make sure that the ID sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_idb_addr,
                                       &private_data.node[data_ptr->dst].next_id_to_send,
                                       1);
#endif

      /* Update next ID to send */
      if (private_data.node[data_ptr->dst].next_id_to_send == 0)
        private_data.node[data_ptr->dst].next_id_to_send = 1;
      else
        private_data.node[data_ptr->dst].next_id_to_send = 0;

      /* Update request structure */
      data_ptr->DMA_buffer_to_send = 0;
      data_ptr->size -= size_to_send;
      data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                         size_to_send;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Nothing left to send for the request
   */
  else {
    /*
     * Mark it done, remove its pointer
     * to its queue and dequeue it, but
     * don't free its memory
     */
    data_ptr->done = 1;
    dequeue(element->queue);
    element->queue = NULL;

    /* Request complete */
    ret_val = 0;
  }

  return ret_val;
}

/*
 * Tries to forward a send request pointed to by element. 
 * Any data transfers are done via SCIMemWrite().
 * ONLY apply to the first request in a queue 
 * because otherwise a request could be finished
 * or send data out of order.
 * 
 * @param element, pointer to a send request
 * @return 0, the request is done and dequeued from its queue
 *         1, the request is still in progress
 *         2, no progress
 */
static inline int forward_sendto_via_SCIMemWrite(Queue_element *element) {
  Sendq_data_element *data_ptr = (Sendq_data_element *) element->data;
  sci_dma_queue_state_t dma_queue_state;
  sci_error_t error;
  int ret_val;
  int size_to_send;
  unsigned int alignment_displ, padding;
  void *src_buffer;

  /* Assume no progress when trying to forward the request */
  ret_val = 2;

  /* 
   * DMA queue posted? 
   */
  if (data_ptr->DMA_buffer_sent) {
   
    /* Check DMA queue state */
    dma_queue_state =
      SCIDMAQueueState(private_data.node[data_ptr->dst].dma_queue);
    
    switch (dma_queue_state) {
      /* 
       * Transfer has completed successfully
       */ 
      case SCI_DMAQUEUE_DONE:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size   -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /* 
       * Do failed transfer again
       */
      case SCI_DMAQUEUE_ERROR:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send data */
        SCIMemWrite(private_data.node[data_ptr->dst].local_dmab_addr,
                    private_data.node[data_ptr->dst].remote_datab_addr,
                    data_ptr->DMA_transfer_size,
                    NO_FLAGS,
                    &error);
        if (error != SCI_ERR_OK) {
          ng_error ("SCIMemWrite() failed with error: %s",
                    get_sci_error (error));
          return -1;
        }

#if SCI_RELIABLE
        /* Make sure that the DATA sent arrives */
        assure_reliable_SCIMemWrite_transfer(data_ptr->dst,
                                             private_data.node[data_ptr->dst].remote_datab_addr,
                                             private_data.node[data_ptr->dst].local_dmab_addr,
                                             data_ptr->DMA_transfer_size);
#endif

        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /*
       * DMA queue is still being processed
       */
      case SCI_DMAQUEUE_POSTED:
        break;
      default:
        ng_error("Unexpected DMA queue state to node %d", data_ptr->dst);
    }

    /* Request still in progress */
    ret_val = 1;
  }
  /*
   * DMA buffer needs to be sent
   */
  else if (data_ptr->DMA_buffer_to_send) {
    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Send data */
      SCIMemWrite(private_data.node[data_ptr->dst].local_dmab_addr,
                  private_data.node[data_ptr->dst].remote_datab_addr,
                  data_ptr->DMA_transfer_size,
                  NO_FLAGS,
                  &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCIMemWrite() failed with error: %s",
                  get_sci_error (error));
        return -1;
      }

#if SCI_RELIABLE
      /* Make sure that the DATA sent arrives */
      assure_reliable_SCIMemWrite_transfer(data_ptr->dst,
                                           private_data.node[data_ptr->dst].remote_datab_addr,
                                           private_data.node[data_ptr->dst].local_dmab_addr,
                                           data_ptr->DMA_transfer_size);
#endif

      /* Send ID, notifying the receiver that new data has arrived */
      *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
        private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
      /* Make sure that the ID sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_idb_addr,
                                       &private_data.node[data_ptr->dst].next_id_to_send,
                                       1);
#endif

      /* Update next ID to send */
      if (private_data.node[data_ptr->dst].next_id_to_send == 0)
        private_data.node[data_ptr->dst].next_id_to_send = 1;
      else
        private_data.node[data_ptr->dst].next_id_to_send = 0;

      /* Update request structure */
      data_ptr->DMA_buffer_to_send = 0;
      data_ptr->size -= data_ptr->payload_transfer_size;
      data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                         data_ptr->payload_transfer_size;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Anything left to send?
   */
  else if (data_ptr->size) {
    /* Size of useful data to be sent in the next sub-transfer */
    size_to_send = min(data_ptr->size, private_data.protocol_mtu);

    /* Check "size_to_send" for SCI size alignment requirements */
    padding = 0;
    alignment_displ = size_to_send % private_data.info_dma_size_alignment;
    if (alignment_displ) {
      padding = private_data.info_dma_size_alignment - alignment_displ;
    }

    /* Check for SCI size and offset alignment requirements */
    if (alignment_displ ||  
        (unsigned long) data_ptr->buffer % (unsigned long) private_data.info_dma_offset_alignment) {
      
      /* 
       * Store the size of useful data to be sent in the 
       * next sub-transfer in the request structure
       */
      data_ptr->payload_transfer_size = size_to_send;

      /* 
       * Store actual DMA transfer size of next 
       * sub-transfer in request structure 
       */
      data_ptr->DMA_transfer_size = size_to_send + padding;

#if LOCAL_MEMCPY 
      /* Copy data to be sent to DMA buffer */ 
      memcpy(private_data.node[data_ptr->dst].local_dmab_addr, data_ptr->buffer, size_to_send);
#endif

      /* Set "DMA_buffer_to_send" in request structure */
      data_ptr->DMA_buffer_to_send = 1;

      /* Buffer from which to copy is DMA buffer*/
      src_buffer = private_data.node[data_ptr->dst].local_dmab_addr;
    }
    else {
      /* Buffer from which to copy is user buffer */
      src_buffer = data_ptr->buffer;
    }

    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Send data */
      SCIMemWrite(src_buffer,
                  private_data.node[data_ptr->dst].remote_datab_addr,
                  size_to_send + padding,
                  NO_FLAGS,
                  &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCIMemWrite() failed with error: %s",
                  get_sci_error (error));
        return -1;
      }

#if SCI_RELIABLE
      /* Make sure that the DATA sent arrives */
      assure_reliable_SCIMemWrite_transfer(data_ptr->dst,
                                           private_data.node[data_ptr->dst].remote_datab_addr,
                                           src_buffer,
                                           size_to_send + padding);
#endif

      /* Send ID, notifying the receiver that new data has arrived */
      *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
        private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
      /* Make sure that the ID sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_idb_addr,
                                       &private_data.node[data_ptr->dst].next_id_to_send,
                                       1);
#endif

      /* Update next ID to send */
      if (private_data.node[data_ptr->dst].next_id_to_send == 0)
        private_data.node[data_ptr->dst].next_id_to_send = 1;
      else
        private_data.node[data_ptr->dst].next_id_to_send = 0;

      /* Update request structure */
      data_ptr->DMA_buffer_to_send = 0;
      data_ptr->size -= size_to_send;
      data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                         size_to_send;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Nothing left to send for the request
   */
  else {
    /*
     * Mark it done, remove its pointer
     * to its queue and dequeue it, but
     * don't free its memory
     */
    data_ptr->done = 1;
    dequeue(element->queue);
    element->queue = NULL;

    /* Request complete */
    ret_val = 0;
  }

  return ret_val;
}

/*
 * Tries to forward a send request pointed to by element. 
 * Any data transfers are done via SCITransferBlock().
 * ONLY apply to the first request in a queue 
 * because otherwise a request could be finished
 * or send data out of order.
 * 
 * @param element, pointer to a send request
 * @return 0, the request is done and dequeued from its queue
 *         1, the request is still in progress
 *         2, no progress
 */
static inline int forward_sendto_via_SCITransferBlock(Queue_element *element) {
  Sendq_data_element *data_ptr = (Sendq_data_element *) element->data;
  sci_dma_queue_state_t dma_queue_state;
  sci_error_t error;
  int ret_val;
  int size_to_send;
  unsigned int alignment_displ, padding;

  /* Assume no progress when trying to forward the request */
  ret_val = 2;

  /* 
   * DMA queue posted? 
   */
  if (data_ptr->DMA_buffer_sent) {
   
    /* Check DMA queue state */
    dma_queue_state =
      SCIDMAQueueState(private_data.node[data_ptr->dst].dma_queue);
    
    switch (dma_queue_state) {
      /* 
       * Transfer has completed successfully
       */ 
      case SCI_DMAQUEUE_DONE:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size   -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /* 
       * Do failed transfer again
       */
      case SCI_DMAQUEUE_ERROR:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send data */
        SCITransferBlock(private_data.local_map_dma,
                         private_data.node[data_ptr->dst].local_dmab_addr -
                           private_data.local_seg_addr_dma,
                         private_data.node[data_ptr->dst].remote_map,
                         NO_OFFSET,
                         data_ptr->DMA_transfer_size,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error ("SCITransferBlock() failed with error: %s",
                    get_sci_error (error));
          return -1;
        }

#if SCI_RELIABLE
        /* Make sure that the DATA sent arrives */
        assure_reliable_SCITransferBlock_transfer(data_ptr->dst, 
                                                  NULL, 
                                                  NULL, 
                                                  data_ptr->DMA_transfer_size);
#endif

        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /*
       * DMA queue is still being processed
       */
      case SCI_DMAQUEUE_POSTED:
        break;
      default:
        ng_error("Unexpected DMA queue state to node %d", data_ptr->dst);
    }

    /* Request still in progress */
    ret_val = 1;
  }
  /*
   * DMA buffer needs to be sent
   */
  else if (data_ptr->DMA_buffer_to_send) {
    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Send data */
      SCITransferBlock(private_data.local_map_dma,
                       private_data.node[data_ptr->dst].local_dmab_addr -
                         private_data.local_seg_addr_dma,
                       private_data.node[data_ptr->dst].remote_map,
                       NO_OFFSET,
                       data_ptr->DMA_transfer_size,
                       NO_FLAGS,
                       &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCITransferBlock() failed with error: %s",
                  get_sci_error (error));
        return -1;
      }

#if SCI_RELIABLE
      /* Make sure that the DATA sent arrives */
      assure_reliable_SCITransferBlock_transfer(data_ptr->dst, 
                                                NULL, 
                                                NULL, 
                                                data_ptr->DMA_transfer_size);
#endif

      /* Send ID, notifying the receiver that new data has arrived */
      *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
        private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
      /* Make sure that the ID sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_idb_addr,
                                       &private_data.node[data_ptr->dst].next_id_to_send,
                                       1);
#endif

      /* Update next ID to send */
      if (private_data.node[data_ptr->dst].next_id_to_send == 0)
        private_data.node[data_ptr->dst].next_id_to_send = 1;
      else
        private_data.node[data_ptr->dst].next_id_to_send = 0;

      /* Update request structure */
      data_ptr->DMA_buffer_to_send = 0;
      data_ptr->size -= data_ptr->payload_transfer_size;
      data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                         data_ptr->payload_transfer_size;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Anything left to send?
   */
  else if (data_ptr->size) {
    /* Size of useful data to be sent in the next sub-transfer */
    size_to_send = min(data_ptr->size, private_data.protocol_mtu);

    /* Check "size_to_send" for SCI size alignment requirements */
    padding = 0;
    alignment_displ = size_to_send % private_data.info_dma_size_alignment;
    if (alignment_displ) {
      padding = private_data.info_dma_size_alignment - alignment_displ;
    }
      
    /* 
     * Store the size of useful data to be sent in the 
     * next sub-transfer in the request structure
     */
    data_ptr->payload_transfer_size = size_to_send;

    /* 
     * Store actual DMA transfer size of next 
     * sub-transfer in request structure 
     */
    data_ptr->DMA_transfer_size = size_to_send + padding;

#if LOCAL_MEMCPY 
    /* Copy data to be sent to DMA buffer */ 
    memcpy(private_data.node[data_ptr->dst].local_dmab_addr, data_ptr->buffer, size_to_send);
#endif

    /* Set "DMA_buffer_to_send" in request structure */
    data_ptr->DMA_buffer_to_send = 1;


    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Send data */
      SCITransferBlock(private_data.local_map_dma,
                       private_data.node[data_ptr->dst].local_dmab_addr -
                         private_data.local_seg_addr_dma,
                       private_data.node[data_ptr->dst].remote_map,
                       NO_OFFSET,
                       data_ptr->DMA_transfer_size,
                       NO_FLAGS,
                       &error);
      if (error != SCI_ERR_OK) {
        ng_error ("SCITransferBlock() failed with error: %s",
                  get_sci_error (error));
        return -1;
      }

#if SCI_RELIABLE
      /* Make sure that the DATA sent arrives */
      assure_reliable_SCITransferBlock_transfer(data_ptr->dst, 
                                                NULL, 
                                                NULL, 
                                                data_ptr->DMA_transfer_size);
#endif

      /* Send ID, notifying the receiver that new data has arrived */
      *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
        private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
      /* Make sure that the ID sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_idb_addr,
                                       &private_data.node[data_ptr->dst].next_id_to_send,
                                       1);
#endif

      /* Update next ID to send */
      if (private_data.node[data_ptr->dst].next_id_to_send == 0)
        private_data.node[data_ptr->dst].next_id_to_send = 1;
      else
        private_data.node[data_ptr->dst].next_id_to_send = 0;

      /* Update request structure */
      data_ptr->DMA_buffer_to_send = 0;
      data_ptr->size -= size_to_send;
      data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                         size_to_send;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Nothing left to send for the request
   */
  else {
    /*
     * Mark it done, remove its pointer
     * to its queue and dequeue it, but
     * don't free its memory
     */
    data_ptr->done = 1;
    dequeue(element->queue);
    element->queue = NULL;

    /* Request complete */
    ret_val = 0;
  }

  return ret_val;
}

/*
 * Tries to forward a send request pointed to by element. 
 * Any data transfers are done via memcpy().
 * ONLY apply to the first request in a queue 
 * because otherwise a request could be finished
 * or send data out of order.
 * 
 * @param element, pointer to a send request
 * @return 0, the request is done and dequeued from its queue
 *         1, the request is still in progress
 *         2, no progress
 */
static inline int forward_sendto_via_memcpy(Queue_element *element) {
  Sendq_data_element *data_ptr = (Sendq_data_element *) element->data;
  sci_dma_queue_state_t dma_queue_state;
  sci_error_t error;
  int ret_val;
  int size_to_send;

  /* Assume no progress when trying to forward the request */
  ret_val = 2;

  /* 
   * DMA queue posted? 
   */
  if (data_ptr->DMA_buffer_sent) {
   
    /* Check DMA queue state */
    dma_queue_state =
      SCIDMAQueueState(private_data.node[data_ptr->dst].dma_queue);
    
    switch (dma_queue_state) {
      /* 
       * Transfer has completed successfully
       */ 
      case SCI_DMAQUEUE_DONE:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size   -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /* 
       * Do failed transfer again
       */
      case SCI_DMAQUEUE_ERROR:
        /* Set DMA queue in IDLE state */
        SCIResetDMAQueue(private_data.node[data_ptr->dst].dma_queue,
                         NO_FLAGS,
                         &error);
        if (error != SCI_ERR_OK) {
          ng_error("SCIResetDMAQueue() failed with error: %s",
                   get_sci_error(error));
          return -1;
        }
        
        /* Send data */
        memcpy(private_data.node[data_ptr->dst].remote_datab_addr,
               private_data.node[data_ptr->dst].local_dmab_addr,
               data_ptr->DMA_transfer_size);

#if SCI_RELIABLE
        /* Make sure that the DATA sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_datab_addr,
                                         private_data.node[data_ptr->dst].local_dmab_addr,
                                         data_ptr->DMA_transfer_size);
#endif
        
        /* Send ID, notifying the receiver that new data has arrived */
        *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
          private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
        /* Make sure that the ID sent arrives */
        assure_reliable_memcpy_transfer(data_ptr->dst,
                                         private_data.node[data_ptr->dst].remote_idb_addr,
                                         &private_data.node[data_ptr->dst].next_id_to_send,
                                         1);
#endif

        /* Update next ID to send */
        if (private_data.node[data_ptr->dst].next_id_to_send == 0)
          private_data.node[data_ptr->dst].next_id_to_send = 1;
        else
          private_data.node[data_ptr->dst].next_id_to_send = 0;

        /* Update request structure */
        data_ptr->DMA_buffer_sent = 0;
        data_ptr->size -= data_ptr->payload_transfer_size;
        data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                           data_ptr->payload_transfer_size;
        break;
      /*
       * DMA queue is still being processed
       */
      case SCI_DMAQUEUE_POSTED:
        break;
      default:
        ng_error("Unexpected DMA queue state to node %d", data_ptr->dst);
    }

    /* Request still in progress */
    ret_val = 1;
  }
  /*
   * DMA buffer needs to be sent
   */
  else if (data_ptr->DMA_buffer_to_send) {
    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Send data */
      memcpy(private_data.node[data_ptr->dst].remote_datab_addr,
             private_data.node[data_ptr->dst].local_dmab_addr,
             data_ptr->DMA_transfer_size);

#if SCI_RELIABLE
      /* Make sure that the DATA sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_datab_addr,
                                       private_data.node[data_ptr->dst].local_dmab_addr,
                                       data_ptr->DMA_transfer_size);
#endif

      /* Send ID, notifying the receiver that new data has arrived */
      *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
        private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
      /* Make sure that the ID sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_idb_addr,
                                       &private_data.node[data_ptr->dst].next_id_to_send,
                                       1);
#endif

      /* Update next ID to send */
      if (private_data.node[data_ptr->dst].next_id_to_send == 0)
        private_data.node[data_ptr->dst].next_id_to_send = 1;
      else
        private_data.node[data_ptr->dst].next_id_to_send = 0;

      /* Update request structure */
      data_ptr->DMA_buffer_to_send = 0;
      data_ptr->size -= data_ptr->payload_transfer_size;
      data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                         data_ptr->payload_transfer_size;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Anything left to send?
   */
  else if (data_ptr->size) {
    /* Size of useful data to be sent in the next sub-transfer */
    size_to_send = min(data_ptr->size, private_data.protocol_mtu);

    /* Am I allowed to send? */
    if (*((volatile u_int8_t *) private_data.node[data_ptr->dst].local_ackb_addr) ==
        private_data.node[data_ptr->dst].next_ack_to_recv) {
      
      /* Update next ACK to receive */
      if (private_data.node[data_ptr->dst].next_ack_to_recv == 0)
        private_data.node[data_ptr->dst].next_ack_to_recv = 1;
      else
        private_data.node[data_ptr->dst].next_ack_to_recv = 0;

      /* Send data */
      memcpy(private_data.node[data_ptr->dst].remote_datab_addr,
             data_ptr->buffer,
             size_to_send);

#if SCI_RELIABLE
      /* Make sure that the DATA sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_datab_addr,
                                       data_ptr->buffer,
                                       size_to_send);
#endif

      /* Send ID, notifying the receiver that new data has arrived */
      *((volatile u_int8_t *) private_data.node[data_ptr->dst].remote_idb_addr) =
        private_data.node[data_ptr->dst].next_id_to_send;

#if SCI_RELIABLE
      /* Make sure that the ID sent arrives */
      assure_reliable_memcpy_transfer(data_ptr->dst,
                                       private_data.node[data_ptr->dst].remote_idb_addr,
                                       &private_data.node[data_ptr->dst].next_id_to_send,
                                       1);
#endif

      /* Update next ID to send */
      if (private_data.node[data_ptr->dst].next_id_to_send == 0)
        private_data.node[data_ptr->dst].next_id_to_send = 1;
      else
        private_data.node[data_ptr->dst].next_id_to_send = 0;

      /* Update request structure */
      data_ptr->size -= size_to_send;
      data_ptr->buffer = (u_int8_t *) data_ptr->buffer + 
                         size_to_send;

      /* Request still in progress */
      ret_val = 1;
    }
  }
  /*
   * Nothing left to send for the request
   */
  else {
    /*
     * Mark it done, remove its pointer
     * to its queue and dequeue it, but
     * don't free its memory
     */
    data_ptr->done = 1;
    dequeue(element->queue);
    element->queue = NULL;

    /* Request complete */
    ret_val = 0;
  }

  return ret_val;
}

/*
 * Tries to forward a receive request pointed to by element. 
 * ONLY apply to the first request in a queue 
 * because otherwise a request could be finished
 * or send data out of order.
 * 
 * @param element, pointer to a receive request
 * @return 0, the request is done and dequeued from its queue
 *         1, the request is still in progress
 *         2, no progress
 */
static inline int forward_recvfrom(Queue_element *element) {
  Recvq_data_element *data_ptr = (Recvq_data_element *) element->data;
  int size_to_copy;
  int ret_val;

  /* Assume no progress when trying to forward the request */
  ret_val = 2;

  /* New data available? */
  if (*((volatile u_int8_t *) private_data.node[data_ptr->src].local_idb_addr) ==
      private_data.node[data_ptr->src].next_id_to_recv) {

    /* Update next ID to recv */
    if (private_data.node[data_ptr->src].next_id_to_recv == 0)
      private_data.node[data_ptr->src].next_id_to_recv = 1;
    else
      private_data.node[data_ptr->src].next_id_to_recv = 0;

    /* How much to copy to user buffer? */
    size_to_copy = min(data_ptr->size, private_data.protocol_mtu);

#if LOCAL_MEMCPY
    /* Copy received data to user buffer */
    memcpy(data_ptr->buffer, private_data.node[data_ptr->src].local_datab_addr, size_to_copy);
    data_ptr->buffer = (u_int8_t *) data_ptr->buffer + size_to_copy;
#endif

    /* Send ACK to sender */
    *((volatile u_int8_t *) private_data.node[data_ptr->src].remote_ackb_addr) =
      *((volatile u_int8_t *) private_data.node[data_ptr->src].local_idb_addr);

#if SCI_RELIABLE
    /* Make sure that the ACK sent arrives */
    assure_reliable_memcpy_transfer(data_ptr->src,
                                     private_data.node[data_ptr->src].remote_ackb_addr,
                                     private_data.node[data_ptr->src].local_idb_addr,
                                     1);
#endif

    data_ptr->size -= size_to_copy;

    /* Nothing left to receive for the request just forwarded? */
    if (!data_ptr->size) {
      /*
       * Mark the request done, remove its pointer
       * to its queue and dequeue it, but don't free its memory
       */
      data_ptr->done = 1;
      dequeue(element->queue);
      element->queue = NULL;    

      /* Request complete */
      ret_val = 0;
    }
    else {
      /* Request still in progress */
      ret_val = 1;
    }
  }
  return ret_val;
}

/*
 * Tries to forward any request if possible.
 * Returns if a request could be forwarded or all queues
 * were checked for a request to forward unsuccessfully.
 *
 * @param queue, pointer to a send/receive queue 
 */
static inline void forward_any_via_PIO(Queue *queue) {
  Queue_element *head_element_ptr;
  Queue_element *cur_head_element_ptr;
  int ret_val;

  /* Queue empty? */
  if (!queue->head) {
    queue = get_next_non_empty_queue(queue);
  }

  /* Non-empty queue found? */
  if (queue) {
    
    /* Get queue's head element */
    head_element_ptr = queue->head;
    cur_head_element_ptr = queue->head;

    /* Try to forward the first element in non-empty queue */
    ret_val = cur_head_element_ptr->forward_via_PIO(cur_head_element_ptr);

    /* No progress? */
    if (ret_val == 2) {
      do {
        /* Get the head of the next queue which is not empty */
        cur_head_element_ptr = get_next_non_empty_queues_head(cur_head_element_ptr);

        /* Try to forward that request */
        ret_val = cur_head_element_ptr->forward_via_PIO(cur_head_element_ptr);

      } 
      /* All queues considered or progress? */
      while (head_element_ptr != cur_head_element_ptr && ret_val == 2);
    }
  }
} 

/*
 * Returns if a request is done.
 *
 * @param element, pointer to a send/receive request
 *
 * @return 0, the request is still in progress
 *         1, the request is done
 */
static inline int done(Queue_element *element) {
  return !element->queue;
}

/*
 * Frees a send/receive request's memory.
 */ 
static inline void destroy(Queue_element *element) {
  free(element->data);
  free(element);
}

void register_sci (void) {
  ng_register_module (&sci_module);
}

#else /* #ifdef NG_SISCI */

void register_sci (void) {
  /* Do nothing if SISCI is not compiled in */
}

#endif /* #ifdef NG_SISCI */
