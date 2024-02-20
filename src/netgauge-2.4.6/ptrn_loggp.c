/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_LOGGP
#include "hrtimer/hrtimer.h"
#include <string.h>	/* memset & co. */
#include <time.h>
#include <assert.h>

extern struct ng_module_list *g_modules;
extern struct ng_options g_options;

/* the benchmark works as follows: 
 * - we want to measure PRTT(n,d,s)
 * - we have two benchmark loops, one over the datasizes and one over the
 *   testcount for each specific datasize (s)
 * - n is the count of packets sent from client to server, the server
 *   sends a single packet back if all are received, n is given as a
 *   parameter (lica: could this be automated?, why 16?)
 * - d is the delay between each message send on the client, this is
 *   also a parameter and should be bigger than the gap
 * 
 * We use two objects to store measurement values:
 * 1. ng_loggp_tests_val is used to store the results of all tests
 *    for a specific combination of n, d and s
 *    - the methods getmed(), getavg(), getmin() and getmax() return
 *      median, average, minimum and maximum test values
 *      
 * 2. ng_loggp_prtt_val is used to store all sizes (s) for a specific
 *    combination of n and d. We need three of those: one for PRTT(1,0,s),
 *    one for PRTT(n,0,s) and one for PRTT(n,d,s)
 *    - they are used for curve-fitting to calculate the LogGP
 *    parameters
 */

/* internal function prototypes */
struct ng_loggp_tests_val;
void loggp_do_benchmarks(struct ng_module *module);
int loggp_prepare_benchmarks(struct ng_module *module);
static int be_a_server(void *buffer, int size, struct ng_module *module, int n, double d, char o_r);
static int be_a_client(void *buffer, int size,
		       struct ng_module *module,
		       struct ng_loggp_tests_val *values);

/* this is object1 that holds all the tests for a specific PRTT(n,d,s)
 * in microseconds */
struct ng_loggp_tests_val {
  double* data; /* data array */
  int n, s; /* n,s values s in bytes*/
  double d; /* d (delay time) in usec */
  int testc, itestc; /* maximal and actual testcount */
  char o_r; /* should we measure o_r or not? - if yes, measure o_r at server and communicate it to client */
  void (*constructor)(struct ng_loggp_tests_val *this, int testc, int n, double d, int s);
  void (*destructor)(struct ng_loggp_tests_val *this);
  void (*addval)(struct ng_loggp_tests_val *this, double val);
  double (*getmed)(struct ng_loggp_tests_val *this);
};

/* object function definitions :) */

/* constructor for ng_loggp_tests_val class */
static void ng_loggp_tests_val_constr(struct ng_loggp_tests_val *this, int testc, int n, double d, int s) {
  this->n=n;
  this->d=d;
  this->s=s;
  this->testc=testc;
  this->o_r=0;
  this->itestc=0; /* number of tests in array */
  this->data = (double*)malloc(testc*sizeof(double));
  {
    int itestc;
    for(itestc=0;itestc<testc;itestc++) {
      this->data[itestc]=0.0;
    } 
  }

}

/* destructor for ng_loggp_tests_val */
static void ng_loggp_tests_val_destr(struct ng_loggp_tests_val *this) {
  if(this->data != NULL) free(this->data);
  this->data=NULL;
}

/* add a measurement value */
static void ng_loggp_tests_val_addval(struct ng_loggp_tests_val *this, double val) {
  
  if(this->itestc < this->testc) {
    this->data[(this->itestc)++]=val;
  } else {
    ng_error("too many tests (this should not happen!)\n");
  }
}

/* get median of all mesurements  */
static double ng_loggp_tests_val_getmed(struct ng_loggp_tests_val *this) {

  /* bubble-sort data */
  int x,y;
  double holder;

  for(x = 0; x < this->itestc; x++)
    for(y = 0; y < this->itestc-1; y++)
      if(this->data[y] > this->data[y+1]) {
        holder = this->data[y+1];
        this->data[y+1] = this->data[y];
        this->data[y] = holder;
      }

  /* return median */
  y = (this->itestc+1)/2;
  return this->data[y];
}

/* this object holds a full PRTT(n,d,s) for a fixed n and d */
typedef struct {
  int size;
  double value;
} t_sizevalue;
struct ng_loggp_prtt_val {
  t_sizevalue* data; /* data array */
  int n; /* n values */
  double d; /* d value in microseconds */
  int elems; /* # of elements in the data array */
  double a, b, lsquares; /* curve parameters, y=ax+b */
  void (*constructor)(struct ng_loggp_prtt_val *this, int n, double d);
  void (*destructor)(struct ng_loggp_prtt_val *this);
  void (*addval)(struct ng_loggp_prtt_val *this, int s, double val);
  void (*getfit)(struct ng_loggp_prtt_val *this, int lower, int upper);
  void (*remove)(struct ng_loggp_prtt_val *this, int item);
};

/* constructor for ng_loggp_prtt_val class */
static void ng_loggp_prtt_val_constr(struct ng_loggp_prtt_val *this, int n, double d) {
  this->n=n;
  this->d=d;
  this->elems=0;
  this->data=NULL;
}

/* destructor for ng_loggp_prtt_val class */
static void ng_loggp_prtt_val_destr(struct ng_loggp_prtt_val *this) {
  if(this->data != NULL) free(this->data);
  this->data=NULL;
}

/* calculate the parameters for y = ax + b in the interval [lower,upper] elements 
 * if lower == upper == 0 -> fit all values */
static void ng_loggp_prtt_getfit(struct ng_loggp_prtt_val *this, int lower, int upper) {
  long double
         x_mean=0,
         y_mean=0,
         h=0,j=0;
  int iterator;
  int count = upper-lower;

  /* solve the linear least squares problem directly, see
   * http://de.wikipedia.org/wiki/Kleinste-Quadrate-Methode (sorry, it's
   * missing in the english variant) for details.
   */
  for(iterator=lower; iterator<upper; iterator++) {
    //printf("fit: %i - %f (%Lf, %Lf, %Lf, %Lf)\n", this->data[iterator].size, this->data[iterator].value, prod_sum, sq_sum, x_mean, y_mean);
    x_mean   += this->data[iterator].size;
    y_mean   += this->data[iterator].value;
  }
  x_mean /= count;
  y_mean /= count;
  for(iterator=lower; iterator<upper; iterator++) {
    h += (this->data[iterator].size - x_mean)*(this->data[iterator].value - y_mean);
    j += (this->data[iterator].size - x_mean)*(this->data[iterator].size - x_mean);
  }
  this->a = h/j;
  this->b = y_mean - this->a * x_mean;
  //printf("params: %lf, %lf (%i) (%Lf, %Lf, %Lf, %Lf)\n", this->a, this->b, count, x_mean, y_mean, h, j);

  /* calculate the least squares difference for a fixed msg-size
   * (x-axis) */
  this->lsquares = 0;
  for(iterator=lower; iterator<upper; iterator++) {
    double sq;

    sq = this->a*this->data[iterator].size + this->b - this->data[iterator].value;
    this->lsquares += sq*sq;
  }
  this->lsquares /= (count-2);
  this->lsquares = sqrt(this->lsquares);
}

/* addval for ng_loggp_prtt_val class */
static void ng_loggp_prtt_addval(struct ng_loggp_prtt_val *this, int s, double val) {
  (this->elems)++;
  this->data = realloc(this->data, this->elems*sizeof(t_sizevalue));
  assert(this->data != NULL);
  this->data[this->elems-1].size = s;
  this->data[this->elems-1].value = val;
}

/* remove for ng_loggp_prtt_val class */
static void ng_loggp_prtt_remove(struct ng_loggp_prtt_val *this, int item) {
  t_sizevalue* tmp;  
  int i, ind;
  
  (this->elems)--;
  tmp = malloc(this->elems*sizeof(t_sizevalue));
  assert(tmp != NULL);
  
  ind = 0;
  for(i = 0; i < this->elems+1; i++) {
    if(i == item) continue;
    tmp[ind] = this->data[i];
    ind++;
  }
  free(this->data);
  this->data=tmp;  
}



/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_loggp = {
   .name = "loggp",
   .flags = 0,
   .desc = "measures LogGP Parameters (experimental)",
   .do_benchmarks = loggp_do_benchmarks
};

static struct {
      MPI_Comm client_comm;
      int client_size, client_rank;
} pattern_data;

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_loggp() {
   ng_register_pattern(&pattern_loggp);
   return 0;
}


static void printparams(struct ng_loggp_prtt_val *gresults, 
                        struct ng_loggp_prtt_val *results_1_0, 
                        struct ng_loggp_prtt_val *results_n_d, 
                        struct ng_loggp_prtt_val *results_n_0,
                        struct ng_loggp_prtt_val *results_o_r,
                        unsigned long data_size, FILE *out, int n, int lower, int upper) {
  double g, G, o_s, o_r, L;
  int ielem;
  char buf[1024];
  
  g = gresults->b;
  G = gresults->a;

  ielem = results_n_d->elems-1;
  o_s = (results_n_d->data[ielem].value - results_1_0->data[ielem].value) / (results_n_d->n-1) - results_1_0->data[ielem].value /* =d */;
  o_r = results_o_r->data[ielem].value;

  /* get L */
  //o0 = (results_n_d->data[0].value - results_1_0->data[0].value) / (results_n_d->n-1) - results_1_0->data[0].value /* =d */;
  //l = results_1_0.data[ielem].value/2-2*o-bigg*results_1_0.data[ielem].size;
  //l = results_1_0.b/2-o;
  L = results_1_0->data[0].value/2;
    
  printf(" L=%lf ", L);
  printf(" s=%i ", results_1_0->data[ielem].size);
  printf(" o_s=%lf ", o_s);
  printf(" o_r=%lf ", o_r);
  printf(" g=%lf ", g);
  printf(" G=%lf (%lf GiB/s)", G, 1/G*8/1024);
  printf(" lsqu(g,G)=%lf ", gresults->lsquares);
  
  if(results_n_d->d < g+G*data_size) printf("!!! d (%lf) is smaller than g+size*G (%lf) !!!\n", results_n_d->d, g+G*data_size);

  printf("\n");
  /* print all results of this round to a file if the user wants it ... */
  /* format: size n PRTT(1,0,s) PRTT(n,0,s) PRTT(n,PRTT(1,0,s),s) L o g G */
  if(out != NULL) {
    snprintf(buf, 1023, "%lu %i %lf %lf %lf %lf %lf %lf %lf %lf %lf\n", 
        data_size,
        n,
        results_1_0->data[results_1_0->elems-1].value,
        results_n_0->data[results_n_0->elems-1].value,
        results_n_d->data[results_n_d->elems-1].value,
        L,
        o_s,
        o_r,
        g,
        G,
        gresults->lsquares);
    fputs(buf, out);
    fflush(out);
  }
}

/* this is the inner test loop from do_benchmarks ... we have to put
 * this in an extra function because we need to do the whole
 * benchmarkset more than once */
static int prtt_do_benchmarks(unsigned long data_size, struct ng_module *module, struct ng_loggp_tests_val *values, struct ng_loggp_prtt_val *results, char *buffer, char o_r /* benchmark o_r? */) {
  /** number of times to test the current datasize */
  unsigned long test_count = g_options.testcount;

  /** how long does the test run? */
  time_t test_time, cur_test_time;

  /** number of tests run */
  int test, ovr_tests, ovr_bytes;

  /* initialize tests object  */
  values->n=results->n;
  values->d=results->d;
  if (!g_options.server) {
    values->constructor(values, /* testcount = */ test_count, /* n =*/ values->n, 
                                /* d = */ values->d, /* s = */ 0);
  }
  values->o_r=o_r;
   
  /* Inner test loop
  * - run the requested number of tests for the current data size
  * - but only if testtime does not exceed the max. allowed test time
  *   (only if max. test time is not zero)
  */
  test_time = 0;
  for (test = 0; !g_stop_tests && test < test_count; test++) {
    /* first statement to prevent floating exception */
    if ( test_count < NG_DOT_COUNT || 
       !(test % (int)(test_count / NG_DOT_COUNT)) ) {
      ng_info(NG_VLEV1, ".");
      fflush(stdout);
    }
  
    /* call the appropriate client or server function */
    if (g_options.server) {
      cur_test_time = time(NULL);
      /* execute server mode function */
      if (be_a_server(buffer, data_size, module, values->n, (o_r ? values->d : 0.0), o_r )) {
        ng_error("server error (test: %i)!\n", test); 
        return 1;
      }
      test_time += time(NULL) - cur_test_time;
    } else {
      /* wait some time for the server to get ready */
      usleep(10);
      cur_test_time = time(NULL);
      /* execute client mode function */
      if (be_a_client(buffer, data_size, module, values)) {
        ng_error("client error (test: %i)!\n", test); 
        return 1;
      }
      test_time += time(NULL) - cur_test_time;
    }
  
    /* calculate overall statistics */
    ovr_tests++;
    ovr_bytes += data_size;
  
    /* measure test time and quit test if 
    * test time exceeds max. test time
    * but not if the max. test time is zero
    */
    if ( (g_options.max_testtime > 0) && 
       (test_time > g_options.max_testtime) ) {
      ng_info(NG_VLEV2, "Round exceeds %d seconds (duration %d seconds)",
      g_options.max_testtime, test_time);
      ng_info(NG_VLEV2, "Test stopped at %d tests", test);
      break;
    }

  }	/* end inner test loop */

  ng_info(NG_VLEV1, "\n");

  /* only a client does the stats stuff */
  if (!g_options.server) {
    double res;
    res = values->getmed(values);
    results->addval(results, data_size, res);
    //results->getfit(results, 0, results->elems);
    //printf("PRTT(n=%i,d=%.2lf,s=%lu)=%.4lf - a=%lf b=%lf (lsquares: %lf)\n", values->n, values->d, data_size, res, results->a, results->b, results->lsquares);
    values->destructor(values);
  }

   return 0;
}

/* the REAL benchmark loop - loops over all sizes for all three
 * benchmarks (PRTT(1,0,s), PRTT(n,0,s), PRTT(n,d,s) where d=PRTT(1,0,s)
 * and n is defined as const */
void loggp_do_benchmarks(struct ng_module *module) {
  /** size of the buffer used for transmission tests */
  int buffer_size;

  /** the buffer used for transmission tests */
  char *buffer;

  /** Output File */
  FILE *out = NULL;
  char buf[2048];

  /* initialize the statistics */
  const char* outfile_name = NULL;

  int i, res;

  /** to store the temporary results and define test parameters */
  struct ng_loggp_tests_val values = {
    .constructor = ng_loggp_tests_val_constr,
    .destructor = ng_loggp_tests_val_destr,
    .addval = ng_loggp_tests_val_addval,
    .getmed = ng_loggp_tests_val_getmed,
  }; 

  /* stores the final results (median of tests) for n=1 and d=0 */
  struct ng_loggp_prtt_val results_1_0 = {
    .constructor = ng_loggp_prtt_val_constr,
    .destructor = ng_loggp_prtt_val_destr,
    .addval = ng_loggp_prtt_addval,
    .getfit = ng_loggp_prtt_getfit,
    .remove = ng_loggp_prtt_remove,
  };

  /* stores the final results (median of tests) for arbitrary n and d=0 */
  struct ng_loggp_prtt_val results_n_0 = {
    .constructor = ng_loggp_prtt_val_constr,
    .destructor = ng_loggp_prtt_val_destr,
    .addval = ng_loggp_prtt_addval,
    .getfit = ng_loggp_prtt_getfit,
    .remove = ng_loggp_prtt_remove,
  };

  /* stores the final results (median of tests) for arbitrary a and d */
  struct ng_loggp_prtt_val results_n_d = {
    .constructor = ng_loggp_prtt_val_constr,
    .destructor = ng_loggp_prtt_val_destr,
    .addval = ng_loggp_prtt_addval,
    .getfit = ng_loggp_prtt_getfit,
    .remove = ng_loggp_prtt_remove,
  };

  /* stores the o_r results - it's a bit an abuse of this data structure
   * but it works conveniently */
  struct ng_loggp_prtt_val results_o_r = {
    .constructor = ng_loggp_prtt_val_constr,
    .destructor = ng_loggp_prtt_val_destr,
    .addval = ng_loggp_prtt_addval,
    .getfit = ng_loggp_prtt_getfit,
    .remove = ng_loggp_prtt_remove,
  };

  /* this is just a temp. object to store
   * (PRTT(size,n,0)-PRTT(size,0,0))/(n-1) to fit g and G to this
   * values */
  struct ng_loggp_prtt_val gresults = {
    .constructor = ng_loggp_prtt_val_constr,
    .destructor = ng_loggp_prtt_val_destr,
    .addval = ng_loggp_prtt_addval,
    .getfit = ng_loggp_prtt_getfit,
    .remove = ng_loggp_prtt_remove,
  };

  /* the famous n of PRTT(n,d,s) */
  const int n = 10;
   
  /** currently tested packet size */
  unsigned long data_size;

  /** number of times to test the current datasize */
  unsigned long test_count = g_options.testcount;

  /* element of last protocol change */
  int lastchange=0 /* last protocol change */;

  if (loggp_prepare_benchmarks(module)) return;
  
  /* get needed data buffer memory */
  buffer_size = g_options.max_datasize + module->headerlen;
  ng_info(NG_VLEV2, "Allocating %d bytes data buffer", buffer_size);
  NG_MALLOC(module, void*, buffer_size, buffer);
  if (!buffer) {
    ng_perror("Failed to allocate %d bytes for the network data buffer", buffer_size);
    goto shutdown;
  }
  
  ng_info(NG_VLEV2, "Initializing data buffer");
  for (i = 0; i < buffer_size; i++) buffer[i] = i & 0xff;
  

  if ((pattern_data.client_rank == 0) && (!g_options.server)) 
    outfile_name = g_options.output_file;
  
  ng_info(NG_VLEV1 | NG_VPALL, "Operating in %s mode", 
    g_options.server? "server" : "client");
  
  if (!g_options.server) {
    /* open results file */
    if (g_options.output_file != NULL) {
      out = fopen(outfile_name, "w");
      if (NULL == out) {
        perror("Unable to open output file");
      } else {
        snprintf(buf, 2047, "# size n PRTT(1,0,s) PRTT(n,0,s) PRTT(n,PRTT(1,0,s),s) L o_s o_r g G lsqu(g,G)\n# to get o: plot \"./loggp.out\" using 1:(($5-$3)/($2-1)-$3)\n# to get g,G: plot \"./loggp.out\" using 1:($4-$3)/($2-1)\n");
        fputs(buf, out);
        fflush(out);
      }

    } else {
      out = NULL;
    }
  }

  /* warmup - because at least IPoIB acts weid for the first packets */
  ng_info(NG_VNORM, "Warming module %s up ... this may take a while", module->name);
  /* results_1_0.constructor(&results_1_0, 1, 0);
  for(data_size = 0; data_size < 50; data_size++)
    res = prtt_do_benchmarks(data_size, module, &values, &results_n_0, buffer);
  results_1_0.destructor(&results_1_0);
    */

  /* construct real benchmark values */
  results_1_0.constructor(&results_1_0, 1, 0);
  results_n_0.constructor(&results_n_0, n, 0);
  results_n_d.constructor(&results_n_d, n, results_n_d.d);
  results_o_r.constructor(&results_o_r, n, 0);
  gresults.constructor(&gresults, 1, 0);
  /* Outer test loop
   * - geometrically increments data_size (i.e. data_size = data_size * 2)
   *  (- geometrically decrements test_count) not yet implemented
   */
  for (data_size = NG_START_PACKET_SIZE;
	     !g_stop_tests && data_size > 0 && data_size <= g_options.max_datasize;
	     //get_next_testparams(&data_size, &test_count, &g_options)) {
	     data_size+=2*512) {
      
    if (!g_options.server) {
      printf("Testing %lu bytes %lu times:\n", data_size, test_count);
    }

    res = prtt_do_benchmarks(data_size, module, &values, &results_1_0, buffer, 0);
    if(res) goto shutdown;
    res = prtt_do_benchmarks(data_size, module, &values, &results_n_0, buffer, 0);
    if(res) goto shutdown;
      
    /* g needs to be fitted to: (PRTT(size,n,0)-PRTT(size,0,0))/(n-1) */
    if (!g_options.server) {
  
      /* add last measurement value to gresults */
      gresults.addval(  &gresults, 
                        results_n_0.data[results_1_0.elems-1].size, 
                        (results_n_0.data[results_1_0.elems-1].value-results_1_0.data[results_1_0.elems-1].value)/(results_n_0.n-1));
      //gresults.getfit(&gresults, lastchange, gresults.elems);
      /* take the PRTT(1,0,s) as delay - this is bigger than g+G*size :) */
      results_o_r.d = results_n_d.d = results_1_0.data[results_1_0.elems-1].value;
    }
    /* results_o_r.d must be valid on client and server! */
    MPI_Bcast(&results_o_r.d, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    /* only set this once */
    res = prtt_do_benchmarks(data_size, module, &values, &results_n_d, buffer, 0);
    if(res) goto shutdown;
    
    /* only set this once */
    res = prtt_do_benchmarks(data_size, module, &values, &results_o_r, buffer, 1);
    if(res) goto shutdown;
    
    /* evaluate the measurement results */
    if (!g_options.server) {
      int ielem;

      /* if lsquares-deviation of fit too high:
       * remove all extreme outliers from gresults - 
       * an outlier is a value that is more than 2*lsquares(g,G) away from the fitted function 
       */
      if(gresults.lsquares > 100) {
        for(ielem = 0; ielem<gresults.elems; ielem++) {
          double lsquares = gresults.lsquares;
          
          /* if this point is more than 2*lsquares above the line, it's
           * probably an outlier */
          if(gresults.data[ielem].value > gresults.a*gresults.data[ielem].size + gresults.b + 2 * lsquares)  {
            double value = gresults.data[ielem].value;
            unsigned int size = gresults.data[ielem].size;

            /* remove value from elements */
            gresults.remove(&gresults, ielem);
            /* TODO: should we also remove it from other prtt_results ? */
            gresults.getfit(&gresults, lastchange, gresults.elems);
            printf("**** removed value %lf for size %u from gresults, lsquares was: %lf, new lsquares: %lf\n", value, size, lsquares, gresults.lsquares);
          }
        }
      }
    
      /* This loop tries to detect protocol changes. A protocol change is
       * assumed if the next x measurement values do differ more that
       * lsquares*2 from the prediction
       */
      {
        const int x = 5; /* number of points to look ahead (+1) */
        //for(ielem = lastchange+3 /* we need 2 elements for a fit */; ielem<gresults.elems-x-1; ielem++) {
        if(lastchange + x /* look-ahead x elems */ + 2 /* we need 2 elems for fit */ <= gresults.elems) {
          int ix /* runner */;
          int flag = 1; /* are all bigger than f(x) + 2*lsquares ? -> 1 = yes */
          double lsquares;
          const double pfact = 2.0; /* wurschtel-factor */
        
          ielem = gresults.elems-x;
          /* get fit for lastchange up to current item */
          //printf("getfit: %i, %i\n", lastchange, ielem);
          gresults.getfit(&gresults, lastchange, ielem);
          lsquares = gresults.lsquares;

          /* look x elements ahead */
          for (ix = ielem+1; ix < ielem+x; ix++) {
            gresults.getfit(&gresults, lastchange, ix);
            /* only if all lsquares have at least doubled 
             * ... alles scheisse, wenn lsquares mal wirklich klein ist
             * haben wir viele Protokollwechsel :-( */
            if((gresults.lsquares < pfact*lsquares) || isnan(lsquares) || (lsquares < 0.15) /* lower bound to prevent flapping */) flag = 0;
          }

#if 0
          /* look x elements ahead */
          for (ix = ielem+1; ix < ielem+x; ix++) {
            
            /* if this point is less than pfact * lsquares above the line, it
             * is not an outlier and no protocol change */
            printf("%i: %lf (%lf*%lf=%lf)\n", gresults.data[ielem].size, fabs(gresults.data[ix].value - gresults.a*gresults.data[ix].size + gresults.b), pfact, gresults.b, pfact * gresults.b);
            if( (isnan(gresults.lsquares)) || /* prevent nan false positives */
                (fabs(gresults.data[ix].value - gresults.a*gresults.data[ix].size + gresults.b) < pfact * gresults.lsquares))  {
              flag = 0;
            }
          }
#endif
          

          /* if all x points are > f(x) + 2*lsquares, we have a protocol
           * change, if only b < x points are larger, they are
           * outliers and are removed in the next loop ... */
          if(flag) {
            /* we have a protocol change and the current element
             * (ielem) is the last element in the old protocol */
            printf("we detected a protocol change at %i bytes:\n", gresults.data[ielem].size);
            gresults.getfit(&gresults, lastchange, ielem);
            printparams(&gresults, &results_1_0, &results_n_d, &results_n_0, &results_o_r, data_size, out, n, lastchange, ielem);
            lastchange=ielem+1;
            /* ok, we have now a new protocol beginning at ielem + 1,
             * and we need to fit the new line to the next x elements */
            //ielem += x; /* ATTENTION: we change the loop-runner here */
            gresults.getfit(&gresults, lastchange, gresults.elems);
          }
        } /* for(ielem = 0; ielem<gresults.elems ... */
        gresults.getfit(&gresults, lastchange, gresults.elems);
        printparams(&gresults, &results_1_0, &results_n_d, &results_n_0, &results_o_r, data_size, out, n, lastchange, results_n_0.elems);
      }
    }
  }  /* if (!g_options.server) */


  if (!g_options.server) {
    /* close results file */
    if ((g_options.output_file != NULL) && (NULL != out))
      fclose(out);
  }

  shutdown:
   free(buffer);
   results_1_0.destructor(&results_1_0);
   results_n_0.destructor(&results_n_0);
   results_n_d.destructor(&results_n_d);
   gresults.destructor(&gresults);
} /* for (data_size = NG_START_PACKET_SIZE; ... */

/**
 * - make a new communicator with an even process count
 * - randomly create server/client pairs
 * @return 0 if everything's fine
 *         other value else
 */
int loggp_prepare_benchmarks(struct ng_module *module) {
  /* only if we've got MPI */
#if NG_MPI
  
  /* and want MPI */
  if (!g_options.mpi) return 0;
   /** all the clients */
   MPI_Comm client_comm;
   int client_size, client_rank;

   /** for splitting the communicators */
   int color;   

   /** field containing a 0 for non-servers */
   char *is_server = NULL;

   /** field telling who is who's partner */
   int *mpi_partners = NULL;

   /** everyone but the last uneven rank */
   MPI_Comm even_comm;
   int even_size, even_rank;


   int i;

   /* kick off the last uneven rank (if any) so we
    * have an even process count
    */
   color = 1;
   if ((g_options.mpi_opts->worldsize % 2 == 1) &&
       (g_options.mpi_opts->worldrank == 
	g_options.mpi_opts->worldsize - 1))
      
      color = MPI_UNDEFINED;

   if (MPI_Comm_split(MPI_COMM_WORLD, color, g_options.mpi_opts->worldrank,
		      &even_comm) != MPI_SUCCESS) {
      ng_error("Could not generate new MPI communicator for an even process count");
      goto shutdown;
   }
   
   /* let the last uneven rank quit itself */
   if ((g_options.mpi_opts->worldsize % 2 == 1) &&
       (g_options.mpi_opts->worldrank == 
	g_options.mpi_opts->worldsize - 1) )
      goto shutdown;

   /* get world-size and ranks for the new even-communicator */
   if (MPI_Comm_rank(even_comm, &even_rank) != MPI_SUCCESS) {
      ng_error("Could not determine rank whithin even_comm");
      goto shutdown;
   }
   
   if (MPI_Comm_size(even_comm, &even_size) != MPI_SUCCESS) {
      ng_error("Could not determine the size of even_comm");
      goto shutdown;
   }
   
   /* initializing client/server pairs at random */
   if (even_rank == 0) {
      //struct drand48_data rand_buf;
      long int rand_num;
      int client_rank, server_rank, pos1;
      
      is_server = (char *)calloc(even_size, sizeof(char));
      if (is_server == NULL) {
	 ng_error("Could not allocate %d bytes for client/server array",
		  even_size * sizeof(char));
	 goto shutdown;
      }
      
      mpi_partners = (int *)malloc(even_size * sizeof(int));
      if (mpi_partners == NULL) {
	 ng_error("Could not allocate %d bytes for mpi partners array",
		  even_size * sizeof(int));
	 goto shutdown;
      }
      
      mpi_partners = (int *)memset(mpi_partners, -1, even_size * sizeof(int));
      
      //srand48_r((long int)time(NULL), &rand_buf);
	  srand( (unsigned int) time(NULL) );
      
      /* roll the dice :) */
      for (i=0; i<even_size/2; i++) {
	 do {
	    //lrand48_r(&rand_buf, &rand_num);
		rand_num = rand();
	    pos1 = rand_num % even_size;
	 } while (is_server[pos1] == 1);
	 
	 is_server[pos1] = 1;
      } 
      
      /* determine partners */
      for (i=0; i<even_size/2; i++) {
	 /* find (next) server without a partner */
	 pos1 = 0;
	 while ( (pos1 < even_size) && 
		 (mpi_partners[pos1] != -1 || 
		  is_server[pos1] != 1) ) pos1++;
	 server_rank = pos1;
	 
	 /* find (next) client without a partner */
	 pos1 = 0;
	 while ( (pos1 < even_size) && 
		 (mpi_partners[pos1] != -1 || 
		  is_server[pos1] == 1) ) pos1++;
	 client_rank = pos1;
	 
	 mpi_partners[server_rank] = client_rank;
	 mpi_partners[client_rank] = server_rank;
      } /* end for */
   } /* end if (even_rank == 0) */
   
   /* tell everyone if he/she is a server */
   if (MPI_Scatter(is_server, 1, MPI_CHAR, &g_options.server, 1, 
		   MPI_CHAR, 0, even_comm) != MPI_SUCCESS) {
      ng_error("Could not distribute server ranks");
      goto shutdown;
   }

   /* tell everyone his partner */
   if (MPI_Scatter(mpi_partners, 1, MPI_INT, &g_options.mpi_opts->partner,
		   1, MPI_INT, 0, even_comm) != MPI_SUCCESS) {
      ng_error("Could not distribute client-server partner ranks");
      goto shutdown;
   }
   
   if (even_rank == 0) {
      free(is_server);
      free(mpi_partners);
   }
   
   ng_info(NG_VLEV1 | NG_VPALL, "Node %d, I am a %s, My partner is %d",
	   even_rank,
	   g_options.server? "server": "client",
	   g_options.mpi_opts->partner);

   
   /* Split the new even_comm. to generate one for the clients only
    * Note that, even though the variables are called "client_..." this operation
    * creates two communicators:
    * one for all processes with options->server == 0 (the Clients) and
    * one for all processes with options->server == 1 (the Servers)
    * When addressing only one group make sure you additionally check for
    * options->server!
    */
   if (MPI_Comm_split(even_comm, g_options.server, 
		      even_rank, &client_comm) != MPI_SUCCESS) {
      ng_error("Could not generate new MPI communicator for all clients");
      goto shutdown;
   }
   
   /* get world-size and ranks for the new client-communicator */
   if (MPI_Comm_rank(client_comm, &client_rank) != MPI_SUCCESS) {
      ng_error("Could not determine rank whithin client_comm");
      goto shutdown;
   }
   
   if (MPI_Comm_size(client_comm, &client_size) != MPI_SUCCESS) {
      ng_error("Could not determine the size of client_comm");
      goto shutdown;
   }

   
   /* copy some vital data for later use */
   pattern_data.client_comm = client_comm;
   pattern_data.client_size = client_size;
   pattern_data.client_rank = client_rank;

   return 0;
   
   /* going to shutdown is the error case... */
  shutdown:
   return 1;
#else
   return 0;
#endif
}

static void my_wait(double d) {
  HRT_TIMESTAMP_T ts;
  unsigned long long targettime, time;

  HRT_GET_TIMESTAMP(ts);
  HRT_GET_TIME(ts,targettime);
  // sleep d usec ...
  targettime += g_timerfreq/1e6*d;
  
  do {
    HRT_GET_TIMESTAMP(ts);
    HRT_GET_TIME(ts,time);
  } while (time < targettime); 
}

/**
 * The server function. Simply receive data from partner and send it back.
 */
static int be_a_server(void *buffer, int size, struct ng_module *module, int n, double d, char o_r /* measure o_r? */) {
  int in; 
  const int partner = (g_options.mpi)?
    g_options.mpi_opts->partner:
    (g_options.server)?
    0 : 1;
  HRT_TIMESTAMP_T t[2];
  
  if (ng_recv_all(partner, buffer, size, module)) {
    ng_error("Error in first ng_recv_all()\n");
    return 1;
  }
  /* get start time */
  if(o_r) HRT_GET_TIMESTAMP(t[0]);
  /* Phase 1: receive data */
  for(in = 0; in<n-1;in++)  {
    if(o_r) my_wait(d);
    if (ng_recv_all(partner, buffer, size, module)) {
      ng_error("Error in ng_recv_all() in=%i\n", in);
      return 1;
    }
  }

  /* get after-receiving time */
  if(o_r) HRT_GET_TIMESTAMP(t[1]);

  /* Phase 2: send data back */
  if (ng_send_all(partner, buffer, size, module))
    return 1;
  
  /* calculate results */
  if(o_r) {
    unsigned long long ticks;
    double val;
    HRT_GET_ELAPSED_TICKS(t[0], t[1], &ticks); 
    val = (HRT_GET_USEC(ticks)-d*(n-1))/(n-1);
    MPI_Send(&val, 1, MPI_DOUBLE, partner, 11, MPI_COMM_WORLD);
  }

   
  return 0;
}

/** 
 * The client function. 
 */
static int be_a_client(void *buffer, int size,
		       struct ng_module *module,
		       struct ng_loggp_tests_val *values) {
  
  int in;
  HRT_TIMESTAMP_T t[2];
  unsigned long long ticks;
  
   /* Phase 1: send data */
   const int partner = (g_options.mpi)?
     g_options.mpi_opts->partner:
     (g_options.server)?
     0 : 1;


   /* get start time */
   HRT_GET_TIMESTAMP(t[0]);

   if (ng_send_all(partner, buffer, size, module)) {
     ng_error("error in first ng_send_all()\n");
     return 1;
   }
   for(in=0;in<values->n-1;in++) {
        my_wait(values->d);
        if (ng_send_all(partner, buffer, size, module)) {
        ng_error("error in ng_send_all(), in=%i\n", in);
        return 1;
      }
   }

   /* Phase 2: receive returned data */
   if (ng_recv_all(partner, buffer, size, module)) {
     ng_error("error in ng_recv_all()\n");
     return 1;
   }
   
   /* get after-receiving time */
   HRT_GET_TIMESTAMP(t[1]);
   
   /* calculate results */
   HRT_GET_ELAPSED_TICKS(t[0], t[1], &ticks); 

   if(values->o_r) { /* benchmark o_r */
     double val;
     MPI_Recv(&val, 1, MPI_DOUBLE, partner, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
     values->addval(values, val);
   } else {
     values->addval(values, HRT_GET_USEC(ticks));
   }
   
   /* return success */
   return 0;
}
#else
int register_pattern_loggp(void) {return 0;}
#endif
