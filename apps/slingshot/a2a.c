#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include "mpi.h"
#ifdef PMI
#include "pmi.h"
#endif

#define M (1024*1024)

// Globals
char *program;

double dclock()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return(tv.tv_sec + (double)tv.tv_usec/1000000);
}

void usage()
{
  fprintf(stderr,"\n\tUsage: %s\n\n", program);
  fprintf(stderr,"           [-h]                    Print this help message\n");
  fprintf(stderr,"           [-m minlongs:maxlongs]  Default 1:0\n");
#ifndef PMI
  fprintf(stderr,"           [-p ppn]                Set PPN, Default 1\n");
#endif
  fprintf(stderr,"           [-r reps]               Repetitions, Default 1024\n");
}

typedef struct {
  int    count;
  double mean;
  double sum;
  double sumsq;
  double min;
  double max;
} stats_t;

int main (int argc, char *argv[])
{
  char hostname[32], *ptr, *sptr;
  double start, end, initTime, startTime;
  int proc, nproc, nid, *nids, i, count=0, longs;
  int nlongs=64*M,minlongs=1,maxlongs=0,reps=1024,rank,opt,status,ppn=1;
  long *source, *target;
  int step=1, verbose = 0, cycles=1;

  program = argv[0];

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &proc);

  initTime = dclock();
  if (proc == 0) { printf("%-15.2f A2A Init Complete\n", initTime); fflush(stdout); }

#ifdef PMI
  if ((status = PMI_Get_clique_size(&ppn)) != PMI_SUCCESS) {
    fprintf (stderr, "PMI_Get_clique_size() Failed: %d\n", status);
    exit(1);
  }
#endif

  while ((opt = getopt(argc, argv, "c:hm:p:r:v")) != EOF) {
    switch (opt) {
    case 'h':
      if (proc == 0)
         usage();
      exit(1);
    case 'm':
      if (ptr = strchr(optarg, ':')) {
	*ptr++ = 0;
        minlongs=atoi(optarg);
	if (sptr = strchr(ptr, ':')) {
	  *sptr++ = 0;
	  step = atoi(sptr);
	} 
        maxlongs = atoi(ptr);
      } else {
        minlongs = maxlongs = atoi(optarg);
      }
      break;
    case 'c':
      cycles = atoi(optarg);
      break;
    case 'p':
      ppn = atoi(optarg);
      break;
    case 'r':
      reps= atoi(optarg);
      break;
    case 'v':
      verbose++;
      break;
    default:
      if (proc == 0)
         usage();
      exit(1);
    }
  }

  gethostname(hostname, sizeof(hostname));
  sscanf(hostname,"nid%d", &nid);
  
  if (!(source = (long *)malloc(nlongs * sizeof(long))) ||
      !(target = (long *)malloc(nlongs * sizeof(long)))) {
    fprintf(stderr,"%d: failed to allocate memory\n", proc);
    exit(1);
  }
  
  for (i=0; i < nlongs; i++) {
    source[i] = 0;
    target[i] = 0;
  }

  longs = nlongs / nproc;
  if (maxlongs == 0 || maxlongs > longs)
    maxlongs=longs;
 
  int iter=1;

  // one warmup
  MPI_Alltoall(source, minlongs, MPI_LONG, target, minlongs, MPI_LONG, MPI_COMM_WORLD);
  startTime = dclock();
 
  if (proc == 0) { printf("%-15.2f A2A Starting %6.2f\n", startTime, startTime-initTime); fflush(stdout); }

  if (proc==0) {
      printf("%15s %6s %6s %8s %6s %6s %6s %8s %8s %8s\n", "", "", "", "Bytes","Size","Node","Reps","Secs","BW/proc","BW/node");
  }

  for (iter=0; iter < cycles; iter++) {
      start = dclock();
      for (i=0; i < reps; i++) 
        MPI_Alltoall(source, minlongs, MPI_LONG, target, minlongs, MPI_LONG, MPI_COMM_WORLD);
      end = dclock();

      double bw, bwn, maxsecs, secs = end - start;

      if (proc == 0) {
	int bytes = minlongs * sizeof(long);
	bw = (double)bytes * nproc * reps / (secs * M);
        bwn = bw*ppn;

	printf("%-15.2f %-6.2f %6d %8d %6d %6d %6d %8.5f %8.2f %8.2f\n", end, end-initTime, iter, bytes, nproc, nid, reps, secs, bw, bwn);
        fflush(stdout);
      }
  } 

  if (proc == 0) { printf("%-15.2f A2A Finished\n", dclock()); fflush(stdout); }
  MPI_Finalize();
  exit(0);
}
