/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

/* vim: set expandtab tabstop=2 shiftwidth=2 autoindent smartindent: */
#include "netgauge.h"
#include "hrtimer/hrtimer.h"
#include "netgauge_cmdline.h"
#include <signal.h>  /* signal interception */
#include <string.h>  /* memset & co. */
#include <stdarg.h>	/* variable arguments functions */
#include <math.h>    /* log2() */

#ifdef NG_HPM
#include <libhpc.h>
#endif

double g_hrttimer_startvalue;
unsigned long long g_timerfreq;

/**
 * user supplied options
 */
struct ng_options g_options;

/**
 * getopt long options
 */
#ifdef HAVE_GETOPT_H
static struct option long_options[]={
	{"help",                   no_argument, 0, 'h'},
	{"writemanpage",           no_argument, 0, 'w'},
	{"init-thread",            no_argument, 0, 'i'},
	{"verbosity level",        no_argument, 0, 'v'},
	{"nooutput",               no_argument, 0, 'n'},
	{"output",           required_argument, 0, 'o'},
	{"fulltestresults",  required_argument, 0, 'f'},
	{"servermode",             no_argument, 0, 'a'},
	{"testcount",        required_argument, 0, 'c'},
	{"time",             required_argument, 0, 't'},
	{"size",             required_argument, 0, 's'},
	{"gradation",        required_argument, 0, 'g'},
	{"sanity-check",     required_argument, 0, 'q'},
	{"hostnames",     required_argument, 0, '-'},
	{"com_pattern",	   required_argument, 0, 'x'},
	{"mode",             required_argument, 0, 'm'}, /* must be last entry! */
	{0, 0, 0, 0}
};

/**
 * array of descriptions
 * 
 * structure: description, parameter * 
 * must be conform to the long_options structure above!
 */
static struct option_info long_option_infos[]={	
  {"print help on global (and mode-specific) options", NULL},
  {"create a new manpage based on current code and print it to STDOUT", NULL},
  {"initialize with MPI_THREAD_MULTIPLE instead of MPI_THREAD_SINGLE", NULL},
  {"verbose output", NULL},
  {"no output, but benchmark results", NULL},
  {"write output to this file", "FILENAME"},
  {"write all/full testresults to files begining with the given filename", "FILENAME"},
  {"operate in server mode", NULL},
  {"initial test count", "NUMBER"},
  {"maximum time for one test", "SECONDS"},
  {"data size (range, yy-xx or -xx)", "BYTE"},
  {"gradation of the geometrical data size growth", "GRADATION"},
  {"perform timer sanity check", NULL},
  {"print hostnames", NULL},
  {"communication pattern, defaults to \"one_one\". See list of available patterns below.", "NAME"},
  {"specifies the mode (required). For further information of available modes see list below.", "NAME"}
};
#endif

/**
 * list of available communication modules
 */
struct ng_module_list *g_modules = NULL;

/**
 * global list of supported communication patterns
 */
struct ng_comm_pattern_list *g_comm_patterns = NULL;

/**
 * flag to stop tests after CTRL-C
 */
unsigned char g_stop_tests;

/* external prototypes of module registration functions -
 * add new modules here
 */
extern void register_tcp(void);
extern void register_ip(void);
extern void register_udp(void);
extern void register_eth(void);
extern void register_enet_edp(void);
extern void register_enet_esp(void);
extern void register_mpi(void);
extern void register_libof(void);
extern void register_ib(void);
extern void register_gm(void);
extern void register_ibv(void);
extern void register_sci(void);
extern void register_armci(void);
extern void register_cell(void);
extern void register_mx(void);

/* function prototypes */
void ng_usage(char *mode);
void ng_free_comm_pattern_list();
void ng_free_module_list();
struct ng_comm_pattern *ng_get_comm_pattern(const char* name);
struct ng_module *ng_get_module(const char *name);
int ng_init_mpi(struct ng_options *options, int *argc, char ***argv);
void ng_shutdown_mpi();

int main(int argc, char **argv) {
  /** requested module */
  struct ng_module *module;

  /** requested comm. pattern */
  struct ng_comm_pattern *pattern;

  /** did we successfully initialize the module? */
  char module_was_init = 0;

  memset(&g_options, 0, sizeof(g_options));

  /* register protocol modules - add new modules here */
  register_dummy();    /* dummy module if no MPI is present */
  register_tcp();    /* internet protocol - transmission control protocol */
  register_udp();    /* internet protocol - user datagram protocol */
  register_eth();    /* raw ethernet */
  register_enet_edp();   /* ethernet protocol EDP */
  register_enet_esp();   /* ethernet protocol ESP */
  register_mpi();    /* mpi send/receive (no effect if NG_MPI is not defined) */
  register_libof();    /* OF_Isend/Ireceive (no effect if NG_MPI is not defined) */
  register_ib();     /* infiniband comm. (NG_MPI and NG_VAPI || NG_OPENIB must be defined) */
  register_gm();     /* myrinet (gm) comm. (NG_MPI and NG_GM must be defined) */
  register_ibv();    /* openib verbs api */
  register_armci();  /* ARMCI */
  register_sci();    /* SCI (NG_SISCI must be defined) */     
  register_cell();    /* cell dma transfer (no effect if NG_CELL is not defined) */
  register_mx();    /* Myrinet Express or the ABI compatible OpenMX low level commlib */

  /* register communication pattern modules */
  register_pattern_overlap();
  register_pattern_one_one();
  register_pattern_one_one_all();
  register_pattern_one_one_mpi_bidirect();
  register_pattern_one_one_perturb();
  register_pattern_one_one_sync();
  register_pattern_one_one_req_queue();
  register_pattern_one_one_dtype();
  register_pattern_memory();
  register_pattern_disk();
  register_pattern_one_one_randtag();
  register_pattern_one_one_randbuf();
  register_pattern_Nto1();
  register_pattern_1toN();
  register_pattern_distrtt();
  register_pattern_nbov();
  register_pattern_loggp();
  register_pattern_noise();
  register_pattern_synctest();
  register_pattern_collvsnoise();
  register_pattern_mprobe();
  register_pattern_ebb();
  register_pattern_func_args();
  register_pattern_cpu();
  
  /* ptrnopts and modeopts are just temporary, unfortunately the
  ng_options struct gets initialized by ng_get_options (and I do not want to
  change working code, if possible) - but this function also modifies argc and argv
  so we want to run before, but have to commit our results afterwards... */
  ng_get_optionstrings(&argc, &argv, &g_options.ngopts , &g_options.ptrnopts, &g_options.modeopts, &g_options.allopts);

  /* read netgauge general options into the options struct */
  ng_get_options(&argc, &argv, &g_options);
  
#ifdef NG_MPI
  /* initialize the MPI */
  if (ng_init_mpi(&g_options, &argc, &argv)) return 1;
  ng_info(NG_VNORM, "Netgauge v%s MPI enabled (P=%i, threadlevel=%i) (%s)", NG_VERSION, g_options.mpi_opts->worldsize, g_options.mpi_opts->threadlevel, g_options.allopts);
#else
  ng_info(NG_VNORM, "Netgauge v%s (%s)", NG_VERSION, g_options.allopts);
  g_options.mpi_opts = (struct ng_mpi_options *)
  calloc(1,sizeof(struct ng_mpi_options));
  g_options.mpi_opts->worldrank=0;
  g_options.mpi_opts->worldsize=1;
  if (!g_options.mpi_opts) {
    ng_error("Could not allocate %d bytes for the MPI specific options", sizeof(struct ng_mpi_options));
    exit(1);
  }
#endif
#ifdef NG_HPM
   hpmInit(g_options.mpi_opts->worldrank, "main");
#endif

#ifndef NG_MPI
  if(strstr(g_options.mode, "mpi") == g_options.mode) g_options.mode  = "dummy"; // plug in a dummy if we don't have MPI
#endif
  /* get requested module and comm pattern */
  module = ng_get_module(g_options.mode);
  if (!module) {
    ng_error("Mode %s not supported", g_options.mode);
    goto shutdown;
  }

  pattern = ng_get_comm_pattern(g_options.pattern);
  if (!pattern) {
    ng_error("Communication pattern \"%s\" not supported", g_options.pattern);
    goto shutdown;
  }

  /* test pattern requirements */
  if(pattern->flags &&  NG_PTRN_NB) {
    /* pattern needs non-blocking comm ... test if function pointers
     * are available */
    if((module->isendto == NULL) || (module->irecvfrom == NULL) || (module->test == NULL)) {
      ng_error("The selected pattern needs non-blocking communication but the selected module does not offer this (optional) functionality!");
	    goto shutdown;
    }
  }

  /* don't initialize timers is there is a --help in pattern or mode options ! */
  if ( (strstr(g_options.ptrnopts,  "--help")  == NULL) && 
       (strstr(g_options.modeopts,  "--help")  == NULL) ) {
    /* initialize HR TIMER - only rank 0 prints */
    if(!g_options.mpi_opts->worldrank) {
      HRT_INIT(1 /* print */, g_timerfreq);
      if(g_options.do_sanity_check) sanity_check(1 /* print */); 
    } else {
      HRT_INIT(0 /* print */, g_timerfreq);
      if(g_options.do_sanity_check) sanity_check(0 /* print */); 
    }
  } else {
    // let only rank 0 get through and print help :)
#ifdef NG_MPI
    if(g_options.mpi && g_options.mpi_opts->worldrank)
      MPI_Finalize();
#endif
  }

  /* print hostnames if asked for */
  if(g_options.print_hostnames) {
    const int len=1024;
    char name[len];
    gethostname(name, len);
#ifdef NG_MPI
    if(!g_options.mpi_opts->worldrank) {
      int j;
      printf("# rank %i -> %s\n", g_options.mpi_opts->worldrank, name);
      for(j=1; j<g_options.mpi_opts->worldsize; j++) {
        MPI_Recv(name, len, MPI_CHAR, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("# rank %i -> %s\n", j, name);
      }
    } else {
      MPI_Send(name, len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  }
#endif
  }


  /* parse module options -- is this code neccessary? */
  if (module->getopt) {
    int i, found_module=0;
    /* TODO: dirty hack to use old module options !!!! - should be
     * removed!!! */
    for (i = 0; i < (argc); i++) {
      if ( (strcmp("-m", (argv)[i]) == 0) || 
           (strcmp("--mode", (argv)[i]) == 0)) {
        /* the mode option is the last (global) option and therefor the
         * 'argv' array and the 'argc' variable need an update */
        (argc)-=i+0;
        (argv)+=i+0;
        found_module=1;
      }
    }
    /* if no module was selected -> no options */
    if(!found_module) {
      argv+=argc;
      argc=0;
    }

    if ( module->getopt(argc, argv, &g_options) ) {
      ng_info(NG_VLEV1, "Parsing specific parameters for mode %s", module->name);
      ng_error("Error parsing module options, exiting.");
	    goto shutdown;
    }
  }
 
  /* parse pattern options -- is this code neccessary? */
  if (pattern->getopt) {
    if (pattern->getopt(argc, argv)) {
      ng_info(NG_VLEV1, "Parsing specific parameters for pattern %s", pattern->name); 
      ng_error("Error parsing module options, exiting.");
      goto shutdown;
    }
  }
   
   /* intercept keyboard interrupt,
    * but allow it to interrupt blocking system calls */
   g_stop_tests = 0;
   //signal(SIGINT, sig_int);
   //siginterrupt(SIGINT, 1);
   
   if (module->init) {
     ng_info(NG_VLEV1, "Initializing transmission mode %s", module->name);
      
     if (module->init(&g_options) != 0) {
	     ng_error("Could not initialize transmission mode %s, aborting.", module->name);
	     goto shutdown;
     } else {
	     module_was_init = 1;
     }
   }

   /* spit out warning if module is unreliable */
   if(!(module->flags & NG_MOD_RELIABLE)) 
     ng_info(NG_VNORM, "The selected communication module %s is unreliable, the run may hang if data loss occurs!", module->name);

   /* check maximum data size of transmission mode */
   if (module->max_datasize > 0 && module->max_datasize < g_options.max_datasize) {
      g_options.max_datasize = module->max_datasize;
      ng_info(NG_VLEV1, "Reducing maximum data size to transmission mode maximum of %d bytes.", g_options.max_datasize);
   }

   /* start the benchmarking using the selected module */
   pattern->do_benchmarks(module);

  shutdown:
   /* clean up */
   if (module_was_init) {
     ng_info(NG_VLEV1, "Shutting down transmission module.");
     if (module->shutdown) module->shutdown(&g_options);
   }

   //ng_free_comm_pattern_list();
   //ng_free_module_list();
#ifdef NG_HPM
   hpmTerminate(g_options.mpi_opts->worldrank);
#endif
#ifdef NG_MPI
   ng_shutdown_mpi();
#endif

   return 0;
}

#ifdef NG_MPI
/**
 * initializes the MPI and fills the global options
 * structure with some infos about it
 */
int ng_init_mpi(struct ng_options *options, int *argc, char ***argv) {
  int mpi_init = 0;
  /* we don't need no MPI */
  if (options->mpi == 0) {
    return 0;
  }
  
  /* check if the MPI subsystem nedds to be 
   * initialized here or already has been
   */
  MPI_Initialized(&mpi_init);
  if (!mpi_init) {
     /* initialize MPI subsystem */
     //if (MPI_Init(argc, argv) != MPI_SUCCESS) {
     int threadlevel=options->mpi_opts->threadlevel;
     int res = MPI_Init_thread(argc, argv, options->mpi_opts->threadlevel, &threadlevel);
     if (res != MPI_SUCCESS) {
  	   ng_error("The MPI subsystem could not be initialized");
	     goto shutdown;
     }
     if (options->mpi_opts->threadlevel > threadlevel) {
       ng_error("Requested threadlevel is not available (req: %i given: %i)!", options->mpi_opts->threadlevel, threadlevel);
	     goto shutdown;
     }
  }
  
  /* get process count and ranks for MPI_COMM_WORLD */
  if (MPI_Comm_rank(MPI_COMM_WORLD, &options->mpi_opts->worldrank) != MPI_SUCCESS) {
     ng_error("Could not determine rank whithin MPI_COMM_WORLD");
     goto shutdown;
  }
  
  if (MPI_Comm_size(MPI_COMM_WORLD, &(options->mpi_opts->worldsize)) != MPI_SUCCESS) {
     ng_error("Could not determine the size of MPI_COMM_WORLD");
     goto shutdown;
  }
  
  /* check if at least two processes are present */
  /* htor: this is not true anymore with the noise pattern 
  if (options->mpi_opts->worldsize < 2) {
     ng_error("At least two processes are needed");
     goto shutdown;
  }*/

  return 0;
   
  shutdown:
   return 1;
}

void ng_shutdown_mpi() {
   int was_init = 0;

   /* nothing to do */
   if (g_options.mpi == 0) return;
   
   MPI_Initialized(&was_init);
   if (was_init) {
      ng_info(NG_VLEV1, "Shutting down MPI subsystem");
      MPI_Finalize();
   }
}

#endif /* NG_MPI */

void ng_abort(const char* error) {
  printf("%s", error);
#ifdef NG_MPI
  MPI_Abort(MPI_COMM_WORLD, 10);
#endif
  exit(1);
}

/**
 * protocol module search function
 */
struct ng_module *ng_get_module(const char *name) {
  struct ng_module_list *ml = g_modules;
   
  while (ml) {
    if (strcmp(ml->module->name, name) == 0) return ml->module;
    ml = ml->next;
  }
  return NULL;
}

/**
 * comm. pattern search function
 */
struct ng_comm_pattern *ng_get_comm_pattern(const char* name) {
  struct ng_comm_pattern_list *l = g_comm_patterns;

  while (l) {
    if (strcmp(l->pattern->name, name) == 0) return l->pattern;
    l = l->next;
  }
  return NULL;
}
   

/**
 * SIGINT handler function, simply sets a global
 * flag which should be used to abort operation
 */
void sig_int(int signum) {
  g_stop_tests = 1;
}

/**
 * communication pattern registration function
 *
 * (adds to front of global list)
 */
int ng_register_pattern(struct ng_comm_pattern *pattern) {
   struct ng_comm_pattern_list *entry = malloc(sizeof(struct ng_comm_pattern_list));
   if (!entry) return 1;

   entry->pattern = pattern;
   entry->next = g_comm_patterns;
   g_comm_patterns = entry;
   
   return 0;
}

void ng_free_comm_pattern_list() {
  struct ng_comm_pattern_list *next;
  
  while (g_comm_patterns) {
    next = g_comm_patterns->next;
    free(g_comm_patterns);
    g_comm_patterns = next;
  }
}

/** 
 * protocol module registration function
 *
 * appends the given module to the end of the global
 * communication module list
 */
int ng_register_module(struct ng_module *module) {
   struct ng_module_list **ml;
   struct ng_module_list *ml_entry = malloc(sizeof(*ml_entry));
   if (!ml_entry) return 1;
   
   ml_entry->module = module;
   ml_entry->next = NULL;
   ng_info(NG_VLEV1, "registering module: %s", module->name);   
   
   for (ml = &g_modules; *ml; ml = &(*ml)->next);
   *ml = ml_entry;

	 if(! ml_entry->module->getopt )
		 ng_error("Module (%s) has no getopt() function.", module->name);
   if(! ml_entry->module->init)
		 ng_error("Module (%s) has no init() function.", module->name);
   if(! ml_entry->module->shutdown )
		 ng_error("Module (%s) has no shutdown() function.", module->name);
   if(! ml_entry->module->usage )
		 ng_error("Module (%s) has no usage() function.", module->name);
   if(! ml_entry->module->writemanpage)
		 ng_info(NG_VLEV1, "Module (%s) has no writemanpage() function.", module->name);

   return 0;
}

/** 
 * free protocol module list 
 */
void ng_free_module_list(void) {
  struct ng_module_list *ml, *ml_next;
  ml = g_modules;
   
  while (ml) {
    ml_next = ml->next;
    free(ml);
    ml = ml_next;
  }
}

/**
 * parse command line parameters for global options
 */
int ng_get_options(int *argc, char ***argv, struct ng_options *options) {
  char *optchars = NG_OPTION_STRING;
  int c, option_index = 0, i;
  extern char *optarg;
  extern int optind, opterr, optopt;
  char print_help = 0;
  
  /* variables needed for correct getopt behavior */
  int opt_count = 0;
 
  /* parameters precheck */
  if ((*argc) < 2) {
    ng_usage(NULL);
    ng_exit(1);
  }
   
  /* we have MPI on by default */
	options->mpi = 1;
	options->mpi_opts = (struct ng_mpi_options *)calloc(1,sizeof(struct ng_mpi_options));
	if (!options->mpi_opts) {
	  ng_abort("Could not allocate memory for the MPI specific options");
	}
		
	//parse cmdline arguments
	struct netgauge_cmd_struct args_info;
	if (netgauge_parser_string(g_options.ngopts, &args_info, "netgauge") != 0) {
		ng_abort("failure while parsing netgauge arguments");
	}

  /* TODO: set old options - this should be removed in a general code
   * cleanup later */
  /* verbosity level */
  for(i = 0; i<args_info.verbosity_arg; i++) {
    options->verbose += 1 << i;
  }
  /* server / client */
  if(args_info.server_flag) options->server = 1;

#ifdef NG_MPI
  if(options->mpi) {
    if(args_info.init_thread_given)
      options->mpi_opts->threadlevel = MPI_THREAD_MULTIPLE;
    else
      options->mpi_opts->threadlevel = MPI_THREAD_SINGLE;
  }
#endif
  /* mode */
  options->mode = args_info.mode_arg;
  /* pattern */
	options->pattern = args_info.comm_pattern_arg;
  /* outfile name */
  options->output_file = args_info.output_arg;
  /* detailed outfile name */
  options->full_output_file = args_info.full_output_arg;
  /* testcount */
  options->testcount = args_info.tests_arg;
  /* testime */
  options->max_testtime = args_info.time_arg;
  /* datasize */
  ng_readminmax(args_info.size_arg, &options->min_datasize, &options->max_datasize);
  options->size_given = args_info.size_given;
  /* gradation */
  options->grad_datasize = args_info.grad_arg;
  /* sanity check? */
  options->do_sanity_check = args_info.sanity_check_flag;
  /* print hostname */
  options->print_hostnames = args_info.hostnames_flag;
  /* write manpage */
  if(args_info.manpage_flag) {
    ng_manpage();
    exit(10);
  }
   
  /* print help if requested or needed */
  if(args_info.help_flag) {
    ng_usage(options->mode);
    ng_exit(0);
  }

  ng_info(NG_VLEV2, "Parsing parameters");
   
  /* apply default values for options not set */
  if (!options->server && !options->output_file) {
     options->output_file = NG_DEFAULT_OUTPUT_FILE;
     ng_info(NG_VLEV2, "No output file specified - using default %s", options->output_file);
  }
  if (options->testcount < 1) {
     options->testcount = NG_DEFAULT_TEST_COUNT;
     ng_info(NG_VLEV2, "No or bad test count specified - using default %d", options->testcount);
  }
  if (options->max_datasize <= 0 || options->max_datasize > NG_MAX_DATA_SIZE) {
     options->max_datasize = NG_MAX_DATA_SIZE;
     ng_info(NG_VLEV2, "No or bad data size specified - using default %d", options->max_datasize);
  }
  if (options->grad_datasize < 1) {
     options->grad_datasize = NG_DEFAULT_DATASIZE_GRADATION;
     ng_info(NG_VLEV2, "No or bad data size gradation specified - using default %d", options->grad_datasize);
  }
  
  if (!options->mode) {
    ng_error("No mode given - use option -m or --mode");
#ifdef NG_MPI
    ng_shutdown_mpi();
#endif
    exit(1);
  }
   
  if (!(options->pattern)) {
    ng_info(NG_VLEV2, "No communication pattern given - using default \"%s\"", NG_DEFAULT_COMM_PATTERN);
    options->pattern = NG_DEFAULT_COMM_PATTERN;
  }
  
  if(options->min_datasize == 0) options->min_datasize=NG_START_PACKET_SIZE;
  if(options->max_datasize == 0) options->max_datasize=NG_MAX_DATA_SIZE;

  /* reset option index for module option parsing */
  optind = 1;

  return 0;
}


/**
 * print usage information
 */
void ng_usage(char *mode) {
#ifdef HAVE_GETOPT_H
	struct ng_module_list **ml;
  struct ng_comm_pattern_list *pl;
  
  int i;

  if(g_options.mpi == 1) {
     if(g_options.mpi_opts->worldrank != 0) return;
  }

  /* print global usage info */
  printf("USAGE: netgauge [global options] -m mode [mode options]\n");
  printf("\nGlobal options:\n");
   
  i=0;
  while (long_options[i].name != NULL) {
   	ng_longoption_usage(
   		long_options[i].val, 
   		long_options[i].name, 
   		long_option_infos[i].desc,
   		long_option_infos[i].param
   	);
   	
   	/* if last entry was reached --> diplay mode usage */
   	if (long_options[++i].name == "mode") {
   		printf("\nMode:\n");
   	}
  } 	
	
	
	/* print module-specific usage information */
	if (mode) {
		struct ng_module *module = ng_get_module(mode);
		
		if (module && module->usage) {
			printf("\nMode %s options:\n", module->name);
			module->usage();
		}
	}
	
	/* if g_modules != NULL, list all available modules */ 
  if (g_modules) {
    printf("\nAvailable modes:\n");
    for (ml = &g_modules; *ml; ml = &(*ml)->next) {
    	//ng_option_usage((*ml)->module->name, (*ml)->module->desc, NULL);
    	ng_longoption_usage(0, (*ml)->module->name, (*ml)->module->desc, NULL);	 		
    }
	}
  
  /* if g_comm_patterns != NULL, list all available patterns */
	if (g_comm_patterns) {
		printf("\nAvailable communication patterns:\n");
		for (pl = g_comm_patterns; pl; pl=pl->next) {
			//ng_option_usage(pl->pattern->name, pl->pattern->desc, NULL);
			ng_longoption_usage(0, pl->pattern->name, pl->pattern->desc, NULL);
		}
	}
#endif
}


/**
 * print usage information for a single option 
 * 
 * DEPRECATED! (please use funktion below)
 */
void ng_option_usage(const char *opt, const char *desc, const char *param) {
   if (param)
      printf("   %-2s %-12s %s\n", opt, param, desc);
   else
      printf("   %-15s %s\n", opt, desc);
}


/**
 * print enhanced usage information for a single option
 */
void ng_longoption_usage
(const char opt, const char *longopt, const char *desc, const char *param) {
	int i;
	int padding = DESC_START_POS - strlen(longopt) - 8;
	int desc_pos;
	int desc_linelength=0;
	int desc_fulllength = strlen(desc);
	char desc_line[CLI_WIDTH - DESC_START_POS + 1];
	
	printf("  ");
	
	/* print short option, long option and parameter (if specified) */  
	if (param) {
		padding-=strlen(param)+1;
		printf("-%c, --%s=%s", opt, longopt, param);
	}
	else {
		if (opt) {
			printf("-%c, --%s", opt, longopt);
		}
		else {
			/* if opt == NULL and param == NULL then 
				 it's just a info, not a cmd-param */
			padding+=6;
			printf("%s", longopt);
		}
	}
	
	/* print description line by line */
	for (desc_pos = 0; desc_pos < desc_fulllength; desc_pos += desc_linelength) {
		if (desc_pos > 0) {
			printf("%*c", DESC_START_POS, ' '); 
			desc+=desc_linelength; 				/* goto text of "next line" */
		}
		else {
			printf("%*c", padding, ' '); 			/* print space 'padding'-times */
		}
		
		/* get length till last space of current line (depends on line width) */
		desc_linelength = 0;
		for (i = 0; i < (CLI_WIDTH - DESC_START_POS); i++) {
			/* is description string at the end? */
			if (desc[i] == 0) {
				desc_linelength = i;
				break;
			}
			else if (desc[i] == ' ') {
				desc_linelength = i+1;
			}
		}
		
		/* copy particular region of string to array and display it*/
		memcpy(desc_line, desc, desc_linelength);
		desc_line[desc_linelength] = 0;		/* set string end */
		printf("%s\n", desc_line);
	}
}


/**
 * Write manpage to STDOUT
 * 
 * please modify variables if needed
 * 
 * usage example: ./netgauge -w | nroff -man | less * 
 */
void ng_manpage(void) {
#ifdef HAVE_GETOPT_H
	int i;
	struct ng_module_list **ml;
	struct ng_comm_pattern_list *pl;
		
	const char *date = "April 1st, 2007";
	const char *description_short = "netgauge benchmark suite";
	const char *description_long = "netgauge benchmark suite. and so on...";
	const char *authors = "Torsten Hoefler, Mirko Reinhardt, Robert Kullmann, Matthias Treydte, "
                         "Sebastian Kratzert, Stefan Wild, Rene Oertel";
	const char *see_also = "ifconfig(1)";
	
	
	printf(".TH %s 1  \"%s\" \"Version %s\" \"USER COMMANDS\"\n", PACKAGE_NAME, date, PACKAGE_VERSION);
	
	printf(".SH NAME\n");
	printf("%s \\- %s\n", PACKAGE_NAME, description_short);
	
	printf(".SH SYNOPSIS\n");
	printf(".B %s\n", PACKAGE_NAME);
	printf("[global options] \\-m mode [mode options]\n");
	
	printf(".SH DESCRIPTION\n");
	printf("%s\n", description_long);

	printf(".P\n");
	printf(".SH GLOBAL OPTIONS\n");
	
	/**
	 * Structure of a command
	 *  
	 * printf(".TP\n");
	 * printf("\\-d\n");
	 * printf("use the given device instead of /dev/cdrom\n");
	 */
   
  i=0;
  while (long_options[i].name != NULL) {
  	printf(".TP\n");
  	
  	if (long_option_infos[i].param) {
  		printf("\\-%c, \\-\\-%s %s\n",	long_options[i].val, long_options[i].name, long_option_infos[i].param);
  	}
  	else {
  		printf("\\-%c, \\-\\-%s\n",	long_options[i].val, long_options[i].name);
  	}
  	 
   	printf("%s\n", long_option_infos[i].desc);
   	
   	/* if last entry was reached --> diplay mode usage */
   	if (long_options[++i].name == "mode") {
   		printf(".P\n.B Mode\n");
   	}
  }
		
	/**
	 * Available Modes
	 */	
	/* if g_modules != NULL, list all available modules */ 
  if (g_modules) {
  	printf(".P\n");
		printf(".SH AVAILABLE MODES\n");	
		for (ml = &g_modules; *ml; ml = &(*ml)->next) {
			printf(".TP\n");
			printf("%s\n", (*ml)->module->name);
    	printf("%s\n", (*ml)->module->desc);	 		
  	}
  }
	
	/**
	 * Available communication patterns
	 */	
	/* if g_comm_patterns != NULL, list all available patterns */
	if (g_comm_patterns) {
		printf(".P\n");
		printf(".SH AVAILABLE COMMUNICATION PATTERNS\n");
		for (pl = g_comm_patterns; pl; pl=pl->next) {
			printf(".TP\n");
			printf("%s\n", pl->pattern->name);
    	printf("%s\n", pl->pattern->desc);
		}
	}
	
	/**
	 * Module options
	 */
	printf(".P\n");
	printf(".SH MODULE OPTIONS\n");
  if (g_modules) {
    for (ml = &g_modules; *ml; ml = &(*ml)->next) {
    	printf(".P\n");
  		printf(".TP\n.B Mode %s\n", (*ml)->module->name);
    	(*ml)->module->writemanpage(); 		 		
    }
	}
	
	printf(".SH EXAMPLES\n");
	printf(".TP\n");
	printf("Show options of module 'ip':\n");
	printf(".B %s\n", PACKAGE_NAME);
	printf("\\-h \\-m ip\n");
	printf(".PP\n");
	
	printf(".SH EXIT STATUS\n");
	printf("%s returns a zero exist status if it succeeds. ", PACKAGE_NAME);
	printf("Non zero is returned in case of failure.\n");
	
	printf(".SH AUTHOR\n");
	printf("%s\n", authors);
	
	printf(".SH SEE ALSO\n");
	printf("%s\n", see_also);
	
	return;
#endif
}

/**
 * Write manpage of module to STDOUT
 */
void ng_manpage_module
(const char opt, const char *longopt, const char *desc, const char *param) {    	
	printf(".TP\n");
  	
	if (param) {
  	printf("\\-%c, \\-\\-%s %s\n", opt, longopt, param);
  }
  else {
  	printf("\\-%c, \\-\\-%s\n", opt, longopt);
  }
  		
	printf("%s\n", desc);
  
  return;
}


/**
 * Formatted Error message output
 *
 * Basically a printf with a prepended message prefix and appended linebreak
 */
void ng_error(const char *format, ...) {
   va_list val;
   char msg[1024];
   unsigned int msgptr;
   
   memset(msg, '\0', sizeof(msg));
   
   snprintf(msg, sizeof(msg)-1, "ERROR: ");
   msgptr = strlen(msg);
#ifdef NG_MPI
   if (g_options.mpi > 0) {
      snprintf(msg + msgptr, sizeof(msg) - strlen(msg) - 1, "(%d): ", g_options.mpi_opts->worldrank);
      msgptr = strlen(msg);
   }
#endif
   snprintf(msg + msgptr, sizeof(msg) - strlen(msg) - 1, "%s\n", format);
   
   va_start(val, format);
   vfprintf(stderr, msg, val);
   fflush(stderr);
   va_end(val);
}


/**
 * Error message output
 *
 * Basically a perror/sprintf combination to provide the possibility to use
 * a format-string and parameter substitutions in error messages
 */
void ng_perror(const char *format, ...) {
   va_list val;
   char msg[1024];
   unsigned int msgptr;
   
   memset(msg, '\0', sizeof(msg));
   
   snprintf(msg, sizeof(msg)-1, "ERROR: ");
   msgptr = strlen(msg);
#ifdef NG_MPI
   if (g_options.mpi > 0) {
      snprintf(msg + msgptr, sizeof(msg) - strlen(msg) - 1, "(%d): ", g_options.mpi_opts->worldrank);
      msgptr = strlen(msg);
   }
#endif
   va_start(val, format);
   vsnprintf(msg + msgptr, sizeof(msg) - strlen(msg) - 1, format, val);
   va_end(val);
   perror(msg);
   fflush(stderr);
}

/**
 * Formatted Info message output
 *
 * Basically printf with a prepended message prefix and appended linebreak
 * 'vlevel' gives the level (of importance) of the incoming message
 */
void ng_info(u_int8_t vlevel, const char *format, ...) {
  if ( (!g_options.silent) && (vlevel & g_options.verbose) ) {
#ifdef NG_MPI
    /* if we use MPI to start the processes we only allow information messages
     * from the root of the "even" communicator (see do_benchmark() function
     * for the definition who is a member of said comm.) or we allow all processes
     * to print the info. msg. if the NG_VPALL bit is set in "vlevel"
     * (see netgauge.h for the macro definitions)
     */
    if ((g_options.mpi_opts->worldrank <= 0) || ((vlevel & NG_VPALL) > 0)) {
#endif
	  if (strlen((void *)format) > 1) {
	    va_list val;
	    char msg[1024], spaces[] = "      ";
	    unsigned int msgptr;
	    
	    memset(msg, '\0', sizeof(msg));
	    //int pos = 2 * (int)(log2((double)(vlevel & g_options.verbose)));
	    //printf("pos: %d\n", pos);
	    int pos = vlevel;
	    spaces[pos] = '\0';
	    
	    snprintf(msg, sizeof(msg)-1, "# Info:  %s", spaces);
	    msgptr = strlen(msg);
#ifdef NG_MPI
	    if (g_options.mpi > 0) {
	       snprintf(msg + msgptr, sizeof(msg) - strlen(msg) - 1, "(%d): ", g_options.mpi_opts->worldrank);
	       msgptr = strlen(msg);
	    }
#endif
	    snprintf(msg + msgptr, sizeof(msg) - strlen(msg) - 1, "%s\n", format);
	    
	    va_start(val, format);
	    vprintf(msg, val);
	    fflush(stdout);
	    va_end(val);
	    
	 } else {
	    /* special mode: if "format" only holds one
	     * character print this character without 
	     * anything else (used for displaying
	     * progress and things like '\n')
	     */
	    putchar(format[0]);
	    fflush(stdout);
	 }
#ifdef NG_MPI
      }  /* end if ( (g_options.mpi > 0) ... */
#endif
   }  /* end if ( (!g_options.silent) && (vlevel & g_options.verbose) ) */
   
}


/**
 * calculate datasize (and test count) for next test round
 */
void get_next_testparams(long *data_size, 
			 long *test_count,
			 struct ng_options *options, 
          struct ng_module *module) {
   
   int max_datasize = ng_min(options->max_datasize, module->max_datasize);
   
   /* after the maximum size has been
    * reached a negative value will be set
    */
   if (*data_size < max_datasize) {
      int x, data_inc;
      
      frexp(*data_size * pow(0.5, options->grad_datasize-1), &x);
      data_inc = pow(2.0, --x);
      /* ensure a data size increment at least of 1 and
       * at most of the difference to the maximum data size
       */
      *data_size += ng_min(ng_max(data_inc, 1), max_datasize - *data_size);
   }
   else *data_size = -1;
}

/**
 * Sends out all data with repetitive calls to the
 * module's send_to(...) function.
 */
/* TODO: deprecated ... replace with NG_SEND and NG_RECV macros ... */
int ng_send_all(int dst, void *buffer, int size, const struct ng_module *module) {
   char *bufptr = buffer;
   int sent = 0;
   unsigned int sent_total = 0;
   
   /* send data */
   while (sent_total < size) {
      sent = module->sendto(dst, bufptr, size - sent_total);

      if (sent < 0) {
	 /* s.t. went wrong... */
	 return -1;
      }

      sent_total += sent;
      bufptr += sent;
   }

   return 0;
}

int ng_recv_all(int src, void *buffer, int size, const struct ng_module *module) {
   char *bufptr = buffer;
   int received = 0;
   unsigned int recv_total = 0;
   
   /* recv data */
   while (recv_total < size) {
      received = module->recvfrom(src, bufptr, size - recv_total);
      
      if (received < 0) {
	 /* s.t. went wrong... */
	 return -1;
      }
      
      recv_total += received;
      bufptr += received;
   }
   
   return 0;
}

void ng_exit(int retcode) {

#ifdef NG_MPI
  ng_shutdown_mpi();
#endif
  exit(retcode);
}

/* accepts string of the following formats:
 * 1. "1000-2000" : sets *min to 1000 and *max to 2000
 * 2. "1000-"     : sets *min to 1000 and leaves *max untouched
 * 3. "-2000"     : sets *max to 2000 and leaves *min untouched
 * 4. "2000"      : sets *max to 2000 and leaves *min untouched 
 */
int ng_readminmax(char *buf, unsigned long *min, unsigned long *max) {
  char *pos, temp[1024], *posdash;

  memset(temp, '\0', 1024);
  *min = 0;
  *max = 0;

  posdash=NULL;
  pos = buf;
  while(*pos != '\0') {
    if(*pos == '-') {
      /* min is before "-" */
      if(pos != buf) {
        /* if there is a min and not "-10" only a max */
        strncpy(temp, buf, pos-buf);
        *min = atoi(temp);
        memset(temp, '\0', 1024);
      }
      posdash=pos+1;
    }
    pos++;
  }
  if(posdash != NULL) {
    /* we found a dash */
    if(pos != posdash) {
      /* if there is a maximum and not "10-" only a min */
      strncpy(temp, posdash, pos-posdash);
      *max = atoi(temp);
      memset(temp, '\0', 1024);
    }
  } else {
    *min = atoi(buf);
    *max = atoi(buf);
  }

  if(*min > *max) {
    printf("min (%ld) > max (%ld), setting min=%ld\n", *min, *max, *max);
    *min = *max;
  }

  return 0;
}

void ng_get_optionstrings(int *argc, char ***argv, char **ngopts, char **ptrnopts, char **modeopts, char **allopts) {
  
  int i;
  char **ptr;
#if 0
  From the gengetopt options:

  {"com_pattern",	   required_argument, 0, 'x'},
	{"mode",             required_argument, 0, 'm'}, // must be last entry!
#endif

  *ngopts = malloc(1 * sizeof(char)); (*ngopts)[0] = '\0';
  *ptrnopts = malloc(1 * sizeof(char)); (*ptrnopts)[0] = '\0';
  *modeopts = malloc(1 * sizeof(char)); (*modeopts)[0] = '\0';
  *allopts = malloc(1 * sizeof(char)); (*allopts)[0] = '\0';
  
  /* option string pointer goes from ng over ptrn to mode */
  ptr = ngopts;

  // iterate over arguments to find out the length
  // of the option strings
  //printf("I was called with argc=%i\n", (*argc));
  for (i = 0; i < (*argc); i++) {
    //printf("argument %i: \"%s\"\n", i, (*argv)[i]);

    *allopts = realloc(*allopts, (strlen(*allopts)+strlen((*argv)[i])+2)*sizeof(char)  /* space */);
    strcat(*allopts, (*argv)[i]);
    strcat(*allopts, " ");

    if ( (strcmp("-x", (*argv)[i]) == 0) || 
         (strcmp("--comm_pattern", (*argv)[i]) == 0)) {
      ptr = ptrnopts;

      *ngopts = realloc(*ngopts, (strlen(*ngopts)+strlen((*argv)[i])+2)*sizeof(char)  /* space */);
      strcat(*ngopts, (*argv)[i]);
      strcat(*ngopts, " ");
      if(i+1 >= (*argc)) printf("no pattern given!\n");
      else {
        *ngopts = realloc(*ngopts, (strlen(*ngopts)+strlen((*argv)[i+1])+2)*sizeof(char) /* space */);
        strcat(*ngopts, (*argv)[i+1]); 
        strcat(*ngopts, " ");
      }
    }
    if ( (strcmp("-m", (*argv)[i]) == 0) || 
         (strcmp("--mode", (*argv)[i]) == 0)) {
      ptr = modeopts;

      *ngopts = realloc(*ngopts, (strlen(*ngopts)+strlen((*argv)[i])+2)*sizeof(char) /* space */);
      strcat(*ngopts, (*argv)[i]);
      strcat(*ngopts, " ");
      if(i+1 >= (*argc)) printf("no mode given!\n");
      else {
        *ngopts = realloc(*ngopts, (strlen(*ngopts)+strlen((*argv)[i+1])+2)*sizeof(char) /* space */);
        strcat(*ngopts, (*argv)[i+1]); 
        strcat(*ngopts, " ");
      }
    }
    int len = strlen(*ptr) + 1; /* terminating null byte*/
    //printf("%p %s %i\n", *ptr, (*argv)[i], len+strlen((*argv)[i]));
    *ptr = realloc(*ptr, (len+strlen((*argv)[i])+2)*sizeof(char) /* space */);
    strcat(*ptr, (*argv)[i]);
    strcat(*ptr, " ");
  }
  /* - TODO: hah, this is a nasty catch 22 :-/ 
  ng_info(NG_VLEV1, "netgauge-opts: %s", *ngopts); 
  ng_info(NG_VLEV1, "pattern-opts: %s", *ptrnopts); 
  ng_info(NG_VLEV1, "module-opts: %s", *modeopts); */
  /*printf("netgauge-opts: %s\n", *ngopts);
  printf("pattern-opts: %s\n", *ptrnopts);
  printf("module-opts: %s\n", *modeopts);*/
}

FILE *open_output_file(char *filename) {

	FILE *fd;

	fd = fopen(filename, "w");
	if (fd == NULL) {
		ng_error("Could not open output file\n");
		ng_exit(EXIT_FAILURE);
	}
	else {
		return fd;
	}
}


/* HTOR: this should go in the general section of NG (for every
 * benchmark) - is this portable? */
void write_host_information(FILE *fd) {
	
	struct utsname uninfo;
	
	if (uname (&uninfo) < 0) {
		ng_info(NG_VLEV1, "Error during execution of uname()");
	}
	else {
		fprintf(fd, "# Sysname : %s\n", uninfo.sysname);
		fprintf(fd, "# Nodename: %s\n", uninfo.nodename);
		fprintf(fd, "# Release : %s\n", uninfo.release);
		fprintf(fd, "# Version : %s\n", uninfo.version);
		fprintf(fd, "# Machine : %s\n", uninfo.sysname);
	}
}

