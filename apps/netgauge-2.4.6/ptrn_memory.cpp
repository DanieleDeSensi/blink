/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@illinois.edu>
 *
 */

#include "netgauge.h"
#ifdef NG_PTRN_MEMORY
#include "hrtimer/hrtimer.h"
#include "MersenneTwister.h"
#include "ptrn_memory_cmdline.h"
#include "ng_tools.hpp"
#include <vector>
#include <time.h>
#include <algorithm>
#include <numeric>

#ifdef NG_HPM
#include <libhpc.h>
#endif

// choices: 
//  USEMYPRNG - simple RNG 
//  USEFASTPRNG - optimized (masked RNG)
//  USEMTPRNG - Mersenne Twister
//  USEHPCCPRNG - HPCC RandomAccess RNG
#define USEFASTPRNG

#if (defined USEFASTPRNG) || (defined USEHPCCPRNG)
#define RNGMASK
static unsigned next = 1;

// sets the mask to 
unsigned long set_rng_mask(unsigned max) {
  unsigned power=1;
  while(power < max) power <<= 1;
  if(power != max) {
    ng_abort("this PRNG only supports sizes that are power of 2 -- choose a different one!\n");
  }
  // set all bits up to power to 1!
  unsigned long mask = ((unsigned long)power) - 1; 
  //printf("%lu %lu %lu\n", power, max, mask);
  return mask;
}
#endif

#ifdef USEFASTPRNG
// taken from POSIX.2-2001
static inline unsigned myrand(unsigned long mask) {
  next = next * 1103515245 + 12345;
  return(next & mask);
}
#endif

#ifdef USEMTPRNG
MTRand mtrand;
static inline unsigned myrand(unsigned long max) {
  return mtrand.randInt(max);
}
#endif

#ifdef USEMYPRNG
static unsigned long next = 1;
// taken from POSIX.2-2001
static inline unsigned myrand(unsigned long max) {
  next = next * 1103515245 + 12345;
  return((unsigned)(next>>16) % max);
}
#endif

#ifdef USEHPCCPRNG
static uint64_t ran=1;
static inline unsigned myrand(unsigned long mask) {
  ran = (ran<<1) ^ (((int64_t)ran<0) ? 7 : 0);
  return (ran&mask);
}
#endif


extern "C" {

// the type for the basic memory access
// need two types -- pchase and everything else!
#define TYPE double
#define TYPE_PCHASE unsigned long

/* this only exists to prevent compiler optimizations that remove the
 * cachewiper ;) */
volatile TYPE NG_Memory_res=0;


extern struct ng_options g_options;

/* internal function prototypes */
static void memory_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_memory = {
   pattern_memory.name = "memory",
   pattern_memory.desc = "measures memory performance",
   pattern_memory.flags = 0,
   pattern_memory.do_benchmarks = memory_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_memory() {
   ng_register_pattern(&pattern_memory);
   return 0;
}

// this is a *function* with a *external volatile side-effect* so that it can't be moved around
TYPE wipe_cache(TYPE *cachewiper, int cachewiper_size) {
  TYPE ret=0;
  for(int w=0; w<cachewiper_size; w++) {
    ret += ++(cachewiper[w]);
  }
  NG_Memory_res += ret;
  return ret;
}

static void memory_do_benchmarks(struct ng_module *module) {

  /** currently tested packet size and maximum */
  long data_elems;

  /** number of times to test the current datasize */
  long test_count = g_options.testcount;

  /** counts up to test_count */
  int test_round = 0;

  //parse cmdline arguments
  struct ptrn_memory_cmd_struct args_info;
  //printf("The string I got: %s\n", g_options.ptrnopts);
  if (ptrn_memory_parser_string(g_options.ptrnopts, &args_info, "netgauge") != 0) {
    exit(EXIT_FAILURE);
  }
  
  if(!g_options.size_given) g_options.max_datasize=32*1024*1024;
  static long max_data_elems = g_options.max_datasize/sizeof(TYPE);  
  /*Allocating buffers for the source and destination*/
  /* get needed data buffer memory */
  ng_info(NG_VLEV1, "Allocating %d bytes for source data buffer", max_data_elems*sizeof(TYPE));
  TYPE *buffer1; 
  NG_MALLOC(module, TYPE*, (max_data_elems+1)*sizeof(TYPE), buffer1);

  ng_info(NG_VLEV2, "Initializing source buffer (make sure it's really allocated)");
  for (int i = 0; i <= max_data_elems; i++) buffer1[i] = (TYPE)0;

  /* get needed data buffer memory */
  ng_info(NG_VLEV1, "Allocating %d bytes for destination data buffer", max_data_elems*sizeof(TYPE));
  TYPE *buffer2; 
  NG_MALLOC(module, TYPE*, (max_data_elems+1)*sizeof(TYPE), buffer2);

  /* PCHASE needs a different type :-( -- can't cast double to char* */
  TYPE_PCHASE *buffer_pchase; 
  if(strcmp(args_info.method_arg, "pchase") == 0) NG_MALLOC(module, TYPE_PCHASE*, (max_data_elems+1)*sizeof(TYPE_PCHASE), buffer_pchase);


  ng_info(NG_VLEV2, "Initializing destination buffer (make sure it's really allocated)");
  for (int i = 0; i <= max_data_elems; i++) buffer2[i] = (TYPE)0;

  /* get needed data buffer memory */
  static const int cachewiper_size = args_info.wipesize_arg*1048580/sizeof(TYPE); 
  TYPE *cachewiper; 
  if(args_info.wipe_given) {
    ng_info(NG_VLEV1, "Allocating %d bytes for cache wipe buffer", cachewiper_size*sizeof(TYPE));
    NG_MALLOC(module, TYPE*, cachewiper_size*sizeof(TYPE), cachewiper);
  }

  ng_info(NG_VNORM, "****** ATTENTION: Ensure that 'ptrn_memory.cpp' was compiled with >= -O3! (element size: %i)******", sizeof(TYPE));
  if(args_info.wipe_given) ng_info(NG_VNORM, "Wiping cache with %.2f MiB before each measurement", (double)cachewiper_size*sizeof(TYPE)/1024/1024);
  if(args_info.memcpy_given) ng_info(NG_VNORM, "Using memcpy() instead of loop for memory copy");

  int rank = g_options.mpi_opts->worldrank;
  
  /* buffer for header ... */
  char* txtbuf = (char *)malloc(2048 * sizeof(char));
  if (txtbuf == NULL) {
    ng_error("Could not (re)allocate 2048 byte for output buffer");
    ng_exit(10);
  }
  memset(txtbuf, '\0', 2048);
  
  char fname[1024];
  strncpy(fname, g_options.output_file, 1023);
  if(args_info.write_all_given) { 
    char suffix[512];
    snprintf(suffix, 511, ".%i", g_options.mpi_opts->worldrank);
    strncat(fname, suffix, 1023);
  }
	FILE* outputfd = open_output_file(fname);
  write_host_information(outputfd);


  /* header printing */
	if (strcmp(args_info.method_arg, "pchase") != 0) {
    snprintf(txtbuf, 2047,
            "## Netgauge v%s - mode %s - %i processes\n"
            "##\n"
      "## A...message size [byte]\n"
      "##\n"
      "## B...minimum time\n"
      "## C...average time\n"
      "## D...median time\n"
      "## E...maximum time\n"
      "## F...standard deviation (stddev)\n"
      "## G...number of measurements, that were bigger than avg + 2 * stddev.\n"
      "##\n"
      "## three blocks: - [read] - [write] - [copy] \n"
      "##\n"
      "## A  -  B  C  D  E (F G) - B  C  D  E (F G) -  B  C  D  E (F G)\n",
      NG_VERSION,
      g_options.mode, g_options.mpi_opts->worldsize);

    if(!rank || args_info.write_all_given) {
      fprintf(outputfd, "%s", txtbuf);
      fprintf(outputfd, "#\n"
          "# gnuplot script (medians):\n"
          "#  set logscale x\n"
          "#  plot 'ng.out' using 1:($1/$5) title 'read'\n"
          "#  replot 'ng.out' using 1:($1/$12) title 'write'\n"
          "#  replot 'ng.out' using 1:($1/$19) title 'copy'\n"
          "# \n"
      );
    }

  } else { 
    snprintf(txtbuf, 2047,
            "## Netgauge v%s - mode %s - %i processes\n"
            "##\n"
      "## A1...number of hops\n"
      "## A2...pointer size [byte]\n"
      "##\n"
      "## B...minimum time\n"
      "## C...average time\n"
      "## D...median time\n"
      "## E...maximum time\n"
      "## F...standard deviation (stddev)\n"
      "## G...number of measurements, that were bigger than avg + 2 * stddev.\n"
      "##\n"
      "##\n"
      "## A1 A2  -  B  C  D  E (F G)\n",
      NG_VERSION,
      g_options.mode,g_options.mpi_opts->worldsize);
  }

  if(rank == 0) {
    // if very verbose - long output
    if (NG_VLEV2 & g_options.verbose) {
      printf("%s", txtbuf);
    } else
    // if verbose - short output
    if (NG_VLEV1 & g_options.verbose) {
      snprintf(txtbuf, 2047,
              "## Netgauge v%s - mode %s - %i processes\n"
              "##\n"
        "## A...message size [byte]\n"
        "##\n"
        "## B...minimum time\n"
        "## C...average time\n"
        "## D...median time\n"
        "## E...maximum time\n"
        "##\n"
        "## three blocks: - [read] - [write] - [copy] \n"
        "##\n"
        "## A  -  B  C  D  E - B  C  D  E -  B  C  D  E \n",
        NG_VERSION,
        g_options.mode,g_options.mpi_opts->worldsize);


      printf("%s", txtbuf);
      
    } else
    // if not verbose - short output
    {
      // no header ...
    }
  }

  HRT_TIMESTAMP_T t[3];  
  unsigned long long tirtt, tr;

	if (strcmp(args_info.method_arg, "stream") == 0) {
    ng_info(NG_VNORM, "performing stream memory benchmark");
  }
	if (strcmp(args_info.method_arg, "random") == 0) {
    // sanity check if random number generator is fast enough!
#ifdef RNGMASK
    register unsigned long mask = set_rng_mask(max_data_elems);
#endif
    register int k;
    HRT_GET_TIMESTAMP(t[0]);
    for(register int i=1; i<max_data_elems+1; ++i) {
#ifdef RNGMASK
      k += myrand(mask);
      //printf("%lu (%lu)\n", myrand(mask), mask); // for testing the RNG ;)
#else
      k += myrand(max_data_elems);
      //printf("%lu (%lu)\n", myrand(max_data_elems), max_data_elems); // for testing the RNG ;)
#endif
    }
    HRT_GET_TIMESTAMP(t[2]);
    HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
    NG_Memory_res += k;
#ifdef USEMTPRNG
    ng_info(NG_VNORM, "performing random access memory benchmark; MersenneTwister bandwidth: %.2f Mops/s = %.2f MiB/s", max_data_elems*1.0/HRT_GET_USEC(tirtt), max_data_elems*(double)sizeof(TYPE)/HRT_GET_USEC(tirtt));
#endif
#ifdef USEMYPRNG
    ng_info(NG_VNORM, "performing random access memory benchmark; Simple PRNG bandwidth: %.2f Mops/s = %.2f MiB/s", max_data_elems*1.0/HRT_GET_USEC(tirtt), max_data_elems*(double)sizeof(TYPE)/HRT_GET_USEC(tirtt));
#endif
#ifdef USEFASTPRNG
    ng_info(NG_VNORM, "performing random access memory benchmark; Fast PRNG bandwidth: %.2f Mops/s = %.2f MiB/s", max_data_elems*1.0/HRT_GET_USEC(tirtt), max_data_elems*(double)sizeof(TYPE)/HRT_GET_USEC(tirtt));
#endif
#ifdef USEHPCCPRNG
    ng_info(NG_VNORM, "performing random access memory benchmark; HPCC PRNG bandwidth: %.2f Mops/s = %.2f MiB/s", max_data_elems*1.0/HRT_GET_USEC(tirtt), max_data_elems*(double)sizeof(TYPE)/HRT_GET_USEC(tirtt));
#endif
    // do only one large benchmark!
    g_options.min_datasize = max_data_elems*sizeof(TYPE);
  }

	if (strcmp(args_info.method_arg, "pchase") == 0) {
    // do only one large benchmark!
    g_options.min_datasize = max_data_elems*sizeof(TYPE);

    ng_info(NG_VNORM, "performing pointer chase memory benchmark - initializing pointer array of size %.2f MiB with %i elements ...", (double)g_options.min_datasize/1048576, max_data_elems);
    if(sizeof(TYPE) != sizeof(char*)) {
      ng_abort("the size of the base type differs from the pointer size on this architecture!\n");
    }
    MTRand mtrand;
    // initialize the data structure ... 
    int curindex=0; 
    for(int i=0; i<max_data_elems-1; ++i) {
      int nextindex = mtrand.randInt(max_data_elems);
      buffer_pchase[curindex] = (TYPE_PCHASE)-1; // reserve spot (avoid self-loops)
      while(buffer_pchase[nextindex] != (TYPE_PCHASE)0) {
        nextindex++;
        if(nextindex == max_data_elems) nextindex=1; // index 0 is reserved for start marker
      }
      buffer_pchase[curindex] = (TYPE_PCHASE)((char*)&buffer_pchase[nextindex]);
      //printf("%i %lu %lu \n", i, buffer1[curindex], buffer1[curindex]-(unsigned long)&buffer1[0]); 
      curindex = nextindex;
    }
    ng_info(NG_VNORM, "... pointer array initialized");

  } 

  /* Outer test loop
   * - geometrically increments data_elems (i.e. data_elems = data_elems * 2)
   *  (- geometrically decrements test_count) not yet implemented
   */
  for (data_elems = ng_max(g_options.min_datasize/sizeof(TYPE), 1); data_elems <= max_data_elems; data_elems*=2) {
    ++test_round;

    // the benchmark results
    std::vector<double> tr, tw, tc; // read, write, copy
    
    ng_info(NG_VLEV1, "Round %d: testing %d times with %d bytes:", test_round, test_count, data_elems);
    // if we print dots ...
    if ( (rank==0) && (NG_VLEV1 & g_options.verbose) ) {
      printf("# ");
    }

  	if (strcmp(args_info.method_arg, "batchstream") == 0) {
      ////////////////////////////////////////////////////////////
      // read phase
			// single warmup round
      register TYPE k=0;
      {
        for(register int i=0; i<data_elems; ++i) {
          k+=buffer1[i];
        }
        NG_Memory_res += k; // avoid optimization
      }
      HRT_GET_TIMESTAMP(t[0]);
			// batch benchmark
    	for (int test = 0; test < test_count; test++) {
        for(register int i=0; i<data_elems; ++i) {
          k+=buffer1[i];
        }
      }
      HRT_GET_TIMESTAMP(t[2]);
      HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
      tr.push_back(HRT_GET_USEC(tirtt)/test_count);
      NG_Memory_res += k; // avoid optimization

      ////////////////////////////////////////////////////////////
      // write phase
			// single warmup round
      {
        for(register int i=0; i<data_elems; ++i) {
          buffer1[i]=k++;
        }
      }
      HRT_GET_TIMESTAMP(t[0]);
			// batch benchmark
    	for (int test = 0; test < test_count; test++) {
        for(register int i=0; i<data_elems; ++i) {
          buffer1[i]=k++;
        }
      }
      HRT_GET_TIMESTAMP(t[2]);
      HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
      tw.push_back(HRT_GET_USEC(tirtt)/test_count);
      NG_Memory_res += k; // avoid optimization

      ////////////////////////////////////////////////////////////
      // copy phase
      if(args_info.memcpy_given) {
        memcpy(buffer2, buffer1, data_elems*sizeof(TYPE));
      } else { 
        for(register int i=0; i<data_elems; ++i) {
          buffer1[i]=buffer2[i];
        }
      }
      HRT_GET_TIMESTAMP(t[0]);
			// batch benchmark
    	for (int test = 0; test < test_count; test++) {
				if(args_info.memcpy_given) {
					memcpy(buffer2, buffer1, data_elems*sizeof(TYPE));
				} else { 
					for(register int i=0; i<data_elems; ++i) {
						buffer1[i]=buffer2[i];
					}
				}
      }
      HRT_GET_TIMESTAMP(t[2]);
      HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
      tc.push_back(HRT_GET_USEC(tirtt)/test_count);
  	} else // end batchstream

    /* Inner test loop
     * - run the requested number of tests for the current data size
     * - but only if testtime does not exceed the max. allowed test time
     *   (only if max. test time is not zero)
     */
    for (int test = -1 /* 1 warmup test */; test < test_count; test++) {
	
	    /* first statement to prevent floating exception */
      /* TODO: add cool abstract dot interface ;) */
	    if ( (NG_VLEV1 & g_options.verbose) && ( test_count < NG_DOT_COUNT || !(test % (int)(test_count / NG_DOT_COUNT)) )) {
	      printf(".");
	      fflush(stdout);
	    }

      /* do the client stuff ... take time, send message, wait for
       * reply and take time  ... simple ping-pong scheme */
  
	    if (strcmp(args_info.method_arg, "stream") == 0) {
        ////////////////////////////////////////////////////////////
        // read phase
        if(args_info.wipe_given) NG_Memory_res += wipe_cache(cachewiper, cachewiper_size);

#ifdef NG_MPI
        MPI_Barrier(MPI_COMM_WORLD);      
#endif

#ifdef NG_HPM
        if(test > -1 ) hpmStart(10,"read");
#endif

        HRT_GET_TIMESTAMP(t[0]);
    
        {
          register TYPE k=0;
          for(register int i=0; i<data_elems; ++i) {
            k+=buffer1[i];
          }
          NG_Memory_res += k; // avoid optimization
        }

#ifdef NG_MPI
        MPI_Barrier(MPI_COMM_WORLD);
#endif

        HRT_GET_TIMESTAMP(t[2]);
        HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);

#ifdef NG_HPM
        if(test > -1 ) hpmStop(10);
#endif
    
        if(test >= 0) tr.push_back(HRT_GET_USEC(tirtt));
        
        ////////////////////////////////////////////////////////////
        // write phase
        if(args_info.wipe_given) NG_Memory_res += wipe_cache(cachewiper, cachewiper_size);

#ifdef NG_MPI
        MPI_Barrier(MPI_COMM_WORLD);      
#endif

#ifdef NG_HPM
        if(test > -1 ) hpmStart(20,"write");
#endif

        HRT_GET_TIMESTAMP(t[0]);
    
        {
          register TYPE k=0;
          for(register int i=0; i<data_elems; ++i) {
            buffer1[i]=k++;
          }
          NG_Memory_res += k; // avoid optimization
        }

        HRT_GET_TIMESTAMP(t[2]);
        HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);

#ifdef NG_HPM
        if(test > -1 ) hpmStop(20);
#endif
    
        if(test >= 0) tw.push_back(HRT_GET_USEC(tirtt));
        
        ////////////////////////////////////////////////////////////
        // copy phase
        if(args_info.wipe_given) NG_Memory_res += wipe_cache(cachewiper, cachewiper_size);

#ifdef NG_MPI
        MPI_Barrier(MPI_COMM_WORLD);      
#endif

#ifdef NG_HPM
        if(test > -1 ) hpmStart(30,"copy");
#endif

        HRT_GET_TIMESTAMP(t[0]);
        if(args_info.memcpy_given) {
          memcpy(buffer2, buffer1, data_elems*sizeof(TYPE));
        } else { 
          for(register int i=0; i<data_elems; ++i) {
            buffer1[i]=buffer2[i];
          }
        }

        HRT_GET_TIMESTAMP(t[2]);
        HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);

#ifdef NG_HPM
        if(test > -1 ) hpmStop(30);
#endif
    
        if(test >= 0) tc.push_back(HRT_GET_USEC(tirtt));
      } 

	    if (strcmp(args_info.method_arg, "random") == 0) {
#ifdef RNGMASK
        register unsigned long mask = set_rng_mask(data_elems);
#endif

        ////////////////////////////////////////////////////////////
        // random read phase
        if(args_info.wipe_given) NG_Memory_res += wipe_cache(cachewiper, cachewiper_size);

#ifdef NG_MPI
        MPI_Barrier(MPI_COMM_WORLD);      
#endif

#ifdef NG_HPM
        if(test > -1 ) hpmStart(40,"randread");
#endif

        HRT_GET_TIMESTAMP(t[0]);
    
        {
          register TYPE k=0;
          for(register int i=0; i<data_elems; ++i) {
#ifdef RNGMASK
            k+=buffer1[myrand(mask)];
#else
            k+=buffer1[myrand(data_elems)];
#endif
          }
          NG_Memory_res += k; // avoid optimization
        }

        HRT_GET_TIMESTAMP(t[2]);
        HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
    
#ifdef NG_HPM
        if(test > -1 ) hpmStop(40);
#endif
    
        if(test >= 0) tr.push_back(HRT_GET_USEC(tirtt));
      
        ////////////////////////////////////////////////////////////
        // random write phase
        if(args_info.wipe_given) NG_Memory_res += wipe_cache(cachewiper, cachewiper_size);

#ifdef NG_MPI
        MPI_Barrier(MPI_COMM_WORLD);      
#endif

#ifdef NG_HPM
        if(test > -1 ) hpmStart(50,"randwrite");
#endif

        HRT_GET_TIMESTAMP(t[0]);
    
        {
          register TYPE k=0;
          for(register int i=0; i<data_elems; ++i) {
#ifdef RNGMASK
            buffer1[myrand(mask)]=k++;
#else
            buffer1[myrand(data_elems)]=k++;
#endif
          }
          NG_Memory_res += k; // avoid optimization
        }

        HRT_GET_TIMESTAMP(t[2]);
        HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
    
#ifdef NG_HPM
        if(test > -1 ) hpmStop(50);
#endif

        if(test >= 0) tw.push_back(HRT_GET_USEC(tirtt));

        ////////////////////////////////////////////////////////////
        // random copy phase
        if(args_info.wipe_given) NG_Memory_res += wipe_cache(cachewiper, cachewiper_size);

#ifdef NG_MPI
        MPI_Barrier(MPI_COMM_WORLD);      
#endif

#ifdef NG_HPM
        if(test > -1 ) hpmStart(60,"randcopy");
#endif

        HRT_GET_TIMESTAMP(t[0]);
        for(register int i=0; i<data_elems; ++i) {
#ifdef RNGMASK
          buffer2[myrand(mask)]=buffer2[myrand(mask)];
#else
          buffer2[myrand(data_elems)]=buffer2[myrand(data_elems)];
#endif
        }
        HRT_GET_TIMESTAMP(t[2]);
        HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);

#ifdef NG_HPM
        if(test > -1 ) hpmStop(60);
#endif
    
        if(test >= 0) tc.push_back(HRT_GET_USEC(tirtt));
      }

	    if (strcmp(args_info.method_arg, "pchase") == 0) {

        if(args_info.wipe_given) NG_Memory_res += wipe_cache(cachewiper, cachewiper_size);
#ifdef NG_MPI
        MPI_Barrier(MPI_COMM_WORLD);      
#endif

#ifdef NG_HPM
        if(test > -1 ) hpmStart(70,"pchase");
#endif

        HRT_GET_TIMESTAMP(t[0]);
        register char* index=(char*)buffer_pchase[0];
        for(int i=0; i<max_data_elems-1; ++i) {
          //printf("%i %lu -> %lu (%lu)\n", i, index, *(TYPE*)index, &buffer1[0]); 
          index = (char*)*(TYPE_PCHASE*)index;
          NG_Memory_res += (unsigned long)index;
        }
        HRT_GET_TIMESTAMP(t[2]);
        HRT_GET_ELAPSED_TICKS(t[0],t[2],&tirtt);
        HRT_GET_USEC(tirtt);
        //printf("chasing %i pointers of size %i B took %.2f us (%.2f ns per pointer)\n", max_data_elems, sizeof(char*), HRT_GET_USEC(tirtt), 1e3*HRT_GET_USEC(tirtt)/max_data_elems);

#ifdef NG_HPM
        if(test > -1 ) hpmStop(70);
#endif
        if(test >= 0) tr.push_back(HRT_GET_USEC(tirtt));
      }

    }	/* end inner test loop */
    
	  if (strcmp(args_info.method_arg, "pchase") != 0) {
      /* compute statistics - read time */
      double tr_avg = std::accumulate(tr.begin(), tr.end(), (double)0)/(double)tr.size(); 
      double tr_min = *min_element(tr.begin(), tr.end()); 
      double tr_max = *max_element(tr.begin(), tr.end()); 
      std::vector<double>::iterator nthr = tr.begin()+tr.size()/2;
      std::nth_element(tr.begin(), nthr, tr.end());
      double tr_med = *nthr;
      double tr_var = standard_deviation(tr.begin(), tr.end(), tr_avg);
      int tr_fail = count_range(tr.begin(), tr.end(), tr_avg-tr_var*2, tr_avg+tr_var*2);

      /* compute statistics - write time */
      double tw_avg = std::accumulate(tw.begin(), tw.end(), (double)0)/(double)tw.size(); 
      double tw_min = *min_element(tw.begin(), tw.end()); 
      double tw_max = *max_element(tw.begin(), tw.end()); 
      std::vector<double>::iterator nthw = tw.begin()+tw.size()/2;
      std::nth_element(tw.begin(), nthw, tw.end());
      double tw_med = *nthw;
      double tw_var = standard_deviation(tw.begin(), tw.end(), tw_avg);
      int tw_fail = count_range(tw.begin(), tw.end(), tw_avg-tw_var*2, tw_avg+tw_var*2);

      /* compute statistics - copy time */
      double tc_avg = std::accumulate(tc.begin(), tc.end(), (double)0)/(double)tc.size(); 
      double tc_min = *min_element(tc.begin(), tc.end()); 
      double tc_max = *max_element(tc.begin(), tc.end()); 
      std::vector<double>::iterator nthc = tc.begin()+tc.size()/2;
      std::nth_element(tc.begin(), nthc, tc.end());
      double tc_med = *nthc;
      double tc_var = standard_deviation(tc.begin(), tc.end(), tc_avg);
      int tc_fail = count_range(tc.begin(), tc.end(), tc_avg-tc_var*2, tc_avg+tc_var*2);


      // if very verbose - long output
        memset(txtbuf, '\0', 2048);
        snprintf(txtbuf, 2047,
          "%ld -  %.2lf %.2lf %.2lf %.2lf (%.2lf %i) -  %.2lf %.2lf %.2lf %.2lf (%.2lf %i) -  %.2lf %.2lf %.2lf %.2lf (%.2lf %i)\n",
          data_elems*sizeof(TYPE), 
          tr_min, tr_avg, tr_med, tr_max, 
          tr_var, /* standard deviation */
          tr_fail, /* how many are bigger than twice the standard deviation? */
          tw_min, tw_avg, tw_med, tw_max, tw_var, tw_fail, 
          tc_min, tc_avg, tc_med, tc_max, tc_var, tc_fail);
      if(!rank || args_info.write_all_given) fprintf(outputfd, "%s", txtbuf);


      if (rank==0) {
        /* add linebreak if we made dots ... */
        if ( (NG_VLEV1 & g_options.verbose) ) {
          ng_info(NG_VLEV1, "\n");
        }

        if (NG_VLEV2 & g_options.verbose) {
          printf("%s", txtbuf);
          
        } else
        // if verbose - short output
        if (NG_VLEV1 & g_options.verbose) {
          memset(txtbuf, '\0', 2048);
          snprintf(txtbuf, 2047,
            "%ld - %.2lf %.2lf %.2lf %.2lf - %.2lf %.2lf %.2lf %.2lf - %.2lf %.2lf %.2lf %.2lf\n",
            data_elems*sizeof(TYPE), 
            
            tr_min, tr_avg, tr_med, tr_max, 
            tw_min, tw_avg, tw_med, tw_max, 
            tc_min, tc_avg, tc_med, tc_max);
          printf("%s", txtbuf);
          
        } else
        // if not verbose - short output
        {
          memset(txtbuf, '\0', 2048);
          snprintf(txtbuf, 2047,
            "%ld bytes \t r w c -> %.2lf %.2lf %.2lf us \t\t ( %.2lf %.2lf %.2lf MiB/s)\n",
            data_elems*sizeof(TYPE), 
            tr_avg, tw_avg, tc_avg, 
            (double)data_elems*sizeof(TYPE)/tr_avg,
            (double)data_elems*sizeof(TYPE)/tw_avg,
            (double)data_elems*sizeof(TYPE)/tc_avg);
          printf("%s", txtbuf);
          
        }
      }
    } else { // pchase
      /* compute statistics - time */
      double tr_avg = std::accumulate(tr.begin(), tr.end(), (double)0)/(double)tr.size(); 
      double tr_min = *min_element(tr.begin(), tr.end()); 
      double tr_max = *max_element(tr.begin(), tr.end()); 
      std::vector<double>::iterator nthr = tr.begin()+tr.size()/2;
      std::nth_element(tr.begin(), nthr, tr.end());
      double tr_med = *nthr;
      double tr_var = standard_deviation(tr.begin(), tr.end(), tr_avg);
      int tr_fail = count_range(tr.begin(), tr.end(), tr_avg-tr_var*2, tr_avg+tr_var*2);

      // if very verbose - long output
        memset(txtbuf, '\0', 2048);
        snprintf(txtbuf, 2047,
          "%ld %ld -  %.2lf %.2lf %.2lf %.2lf (%.2lf %i)\n",
          data_elems, sizeof(char*),
          tr_min, tr_avg, tr_med, tr_max, 
          tr_var, /* standard deviation */
          tr_fail /* how many are bigger than twice the standard deviation? */);
      if(!rank || args_info.write_all_given) fprintf(outputfd, "%s", txtbuf);


      if (rank==0) {
        /* add linebreak if we made dots ... */
        if ( (NG_VLEV1 & g_options.verbose) ) {
          ng_info(NG_VLEV1, "\n");
        }

        if (NG_VLEV2 & g_options.verbose) {
          printf("%s", txtbuf);
          
        } else
        // if verbose - short output
        if (NG_VLEV1 & g_options.verbose) {
          memset(txtbuf, '\0', 2048);
          snprintf(txtbuf, 2047,
            "%ld %ld - %.2lf %.2lf %.2lf %.2lf \n",
            data_elems, sizeof(char*), 
            
            tr_min, tr_avg, tr_med, tr_max);
          printf("%s", txtbuf);
          
        } else
        // if not verbose - short output
        {
          memset(txtbuf, '\0', 2048);
          snprintf(txtbuf, 2047,
            "traversed %ld pointers of size %ld in %.2lf us -> %.2lf ns per hop\n",
            data_elems, sizeof(TYPE), 
            tr_avg, 
            1e3*tr_avg/data_elems);
          printf("%s", txtbuf);
          
        }
      }
    }
    ng_info(NG_VLEV1, "\n");
    fflush(stdout);


  }	/* end outer test loop */

  if(!rank || args_info.write_all_given) fclose(outputfd);


  if(args_info.wipe_given) for(int w=0; w<cachewiper_size; w++) {
    NG_Memory_res += cachewiper[w];
  }

   
 shutdown:
   if(txtbuf) free(txtbuf);
    
}

} /* extern C */

#else
extern "C" {
int register_pattern_memory(void) {return 0;};
}
#endif
