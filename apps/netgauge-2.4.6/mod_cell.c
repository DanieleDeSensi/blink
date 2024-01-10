/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"

#if defined NG_CELL && defined NG_MOD_CELL

#include <ppu_intrinsics.h>
#include <libspe2.h>
#include "mod_cell.h"

extern struct ng_options g_options;

/** module registration data structure */
struct ng_module cell_module = {
   .name         = "cell",
   .desc         = "Mode cell tests Cell BE data transmission via memcpy.",
   .max_datasize = CELL_BUFSIZE,
   .headerlen    = 0,         /* no extra space needed for header */
   .getopt       = cell_getopt,
   .flags        = NG_MOD_RELIABLE,
   .init         = cell_init,
   /* .server       = cell_server, */
   /* .client       = cell_client, */
   .shutdown     = cell_shutdown,
   .usage        = cell_usage,
   .sendto       = cell_sendto,
   .recvfrom     = cell_recvfrom,
};


/** extern modules which will be registerd by module_cell */
extern struct ng_module cell_dma_module;
extern struct ng_module cell_dmalist_module;
extern struct ng_module cell_mail_module;


/** module private data */
struct cell_private_data module_data;


/** netgauge spu program */
extern spe_program_handle_t mod_cell_spu;


/** module registration */
int register_cell(void) {
   ng_register_module(&cell_module);
   ng_register_module(&cell_dma_module);
   ng_register_module(&cell_dmalist_module);
   ng_register_module(&cell_mail_module);
   
   return 0;
}


/** module specific usage information */
void cell_usage(void) {
}


/** parse command line parameters for module-specific options */
int  cell_getopt(int argc, char **argv, struct ng_options *global_opts __attribute__((unused))) {
   int c;
   char *optchars = NG_OPTION_STRING ""; /* additional module options */
   extern char *optarg;
   extern int optind, opterr, optopt;

   int failure = 0;

   memset(&module_data, 0, sizeof(module_data));
   
   /* set as default behaviour that the main buffer is in main memory
      and spu has second buffer */
   module_data.num_spes = 1;
   module_data.tmode = ppu_spe;

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
      }
   }

   /* reset option index */
   optind = 1;

   /* set mod_cell_spu as default spu_progam */
   module_data.spu_program = &mod_cell_spu;

   /* check for necessary parameters, apply default values where possible */
   /* if (!failure) {
    }
   */
   /* report success or failure */
   return failure;
}


/** load special spu-program */
void* spe_run(void *arg) {
   unsigned int entry = SPE_DEFAULT_ENTRY;
   spe_context_ptr_t ctx = *(spe_context_ptr_t*)(arg);
   spe_stop_info_t info;

   /* start execution of spu program and wait for exit */
   if (spe_context_run(ctx, &entry, 0, NULL, NULL, &info) != 0) {
      ng_error("spe_context_run() failed: %s", strerror(errno));
   }

   return NULL;
}


/** cell specific benchmark initialization */
int  cell_init(struct ng_options *global_opts __attribute__((unused))) {
   int ret;
   uint32_t data;
   unsigned int i;

   /* check if ppu is in 32 bit mode */
   /* 64 bit mode is disabled at the moment because embed spu will only create objects for 64 (x)or 32 bit */
   if (sizeof(void*) != 4) {
      ng_error("Pointer %d bit and not 32 bit. Try to compile in 32 bit mode", sizeof(void*)*8);
      return 1;
   }

   /* create arrays to safe context and thread in it */
   module_data.pts = (pthread_t*)malloc(sizeof(pthread_t) * module_data.num_spes);
   module_data.ctx = (spe_context_ptr_t*)malloc(sizeof(spe_context_ptr_t) * module_data.num_spes);
   module_data.local_store = (void*)malloc(sizeof(void*) * module_data.num_spes);

   /* create gang for affinity of contexts */
   if (module_data.affinity != 0) {
      module_data.gang_ctx = spe_gang_context_create(0);
      if (module_data.gang_ctx == 0) {
         ng_error("spe_gang_context_create() failed: %s", strerror(errno));
         return 1;
      }
   }

   /* create different spe contexts and run them */
   for (i = 0; i < module_data.num_spes; i++) {
      /* Create context for spu programs */
      if (module_data.affinity != 0) {
         /* Create it in gang if affinity is specified*/
         if (i == 0) {
            /* initial node in gang */
            module_data.ctx[i] = spe_context_create_affinity(0, NULL, module_data.gang_ctx);
         } else {
            /* other nodes in gang */
            module_data.ctx[i] = spe_context_create_affinity(0, module_data.ctx[i - 1], module_data.gang_ctx);
         }
         if (module_data.ctx[i] == 0) {
            ng_error("spe_context_create_affinity() failed: %s", strerror(errno));
            return 1;
         }
      } else {
         module_data.ctx[i] = spe_context_create(0, NULL);
         if (module_data.ctx[i] == 0) {
            ng_error("spe_context_create() failed: %s", strerror(errno));
            return 1;
         }
      }

      /* Load program from memory for spu */
      if (spe_program_load(module_data.ctx[i], module_data.spu_program) != 0) {
         ng_error("spe_program_load() failed: %s", strerror(errno));
         return 1;
      }

      /* create thread which starts spu program */
      ret = pthread_create(&module_data.pts[i], NULL, &spe_run, &module_data.ctx[i]);
      if (ret != 0) {
         ng_error("pthread_create() failed: %s", strerror(ret));
         return 1;
      }

      /* map local store of spu into normal memory range */
      module_data.local_store[i] = spe_ls_area_get(module_data.ctx[i]);
      if (module_data.local_store[i] == 0) {
         ng_error("spe_ls_area_get() failed: %s", strerror(errno));
         return 1;
      }

      /* wait for pointer to data buffer in local store */
      while (spe_out_mbox_status(module_data.ctx[i]) == 0) {
         usleep(100);

         if (stop_tests || errno == EINTR) {
            ng_error("spe_out_mbox_status() interrupted");
            return 1;
         }
      }

      /* receive pointer to buffer and add it to position of local store */
      ret = spe_out_mbox_read(module_data.ctx[i], &data, 1);
      if (ret == 0) {
         ng_error("spe_out_mbox_read() failed: %s", strerror(errno));
         return 1;
      }
      module_data.local_store[i] += data; /* add position of buffer in ls to localstore */
   }

   /* workaround for some patterns which segfaults when memory is not allocated */
   g_options.mpi = 0;
   g_options.mpi_opts = (struct ng_mpi_options *)
                        calloc(1,sizeof(struct ng_mpi_options));
   if (!g_options.mpi_opts) {
      ng_error("Could not allocate %d bytes for the MPI specific options",
      sizeof(struct ng_mpi_options));
      exit(1);
   }

   /* everything went fine */
   return 0;
}


/** module specific shutdown */
void cell_shutdown(struct ng_options *global_opts __attribute__((unused))) {
   int ret;
   unsigned int i;

   cell_task task;
   task.description.type = CELL_TASK_EXIT;

   for (i = 0; i < module_data.num_spes; i++) {
      /* send exit signal to spu */
      if (spe_in_mbox_write(module_data.ctx[i], &task.mail, 1, SPE_MBOX_ALL_BLOCKING) != 1) {
         ng_error("spe_in_mbox_write() failed: %s", strerror(errno));
      }

      /* wait until thread stopped -> spu quit */
      ret = pthread_join(module_data.pts[i], NULL);
      if (ret != 0) {
         ng_error("pthread_join() failed: %s", strerror(ret));
      }

      /* free spu context */
      if (spe_context_destroy(module_data.ctx[i]) != 0) {
         ng_error("spe_context_destroy() failed: %s", strerror(errno));
      }
   }
   
   /* free gang context if we created one */
   if (module_data.affinity != 0) {
      if (spe_gang_context_destroy(module_data.gang_ctx) != 0) {
         ng_error("spe_gang_context_destroy() failed: %s", strerror(errno));
      }
   }
   
   /* free up context and thread arrays */
   free(module_data.pts);
   free(module_data.ctx);
   free(module_data.local_store);
}


/** module function for sending a single buffer of data */
int cell_sendto(int dst, void *buffer, int size) {
    /* send data to buffer */
    memcpy(module_data.local_store[0], buffer, size);

   /* send size to indicate that everything went fine */
   return size;
}


/** module function for receiving a single buffer of data */
int cell_recvfrom(int src, void *buffer, int size) {
   /* receive data to buffer */
   memcpy(buffer, module_data.local_store[0], size);

   /* send size to indicate that everything went fine */
   return size;
}


#else

/* module registration (dummy) */
int register_cell(void) {
   /* do nothing if Cell support is not compiled in */
   return 0;
}


#endif  /* #ifdef NG_CELL */
