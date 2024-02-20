/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#if defined NG_PTRN_MPROBE && defined NG_MPI
#include "hrtimer/hrtimer.h"
#include "mersenne/MersenneTwister.h"
#include "ptrn_mprobe_cmdline.h"
#include <stdint.h>
#include <assert.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include "librecv_dynsize.h"

//#define HAVE_IRECV


extern "C" {
#include "ng_sync.h"

extern struct ng_options g_options;


/* internal function prototypes */
static void mprobe_do_benchmarks(struct ng_module *module);

/**
 * comm. pattern description and function pointer table
 */
static struct ng_comm_pattern pattern_mprobe = {
   pattern_mprobe.name = "mprobe",
   pattern_mprobe.desc = "benchmarks threaded dynamic receive performance",
   pattern_mprobe.flags = 0,
   pattern_mprobe.do_benchmarks = mprobe_do_benchmarks
};

/**
 * register this comm. pattern for usage in main
 * program
 */
int register_pattern_mprobe() {
   ng_register_pattern(&pattern_mprobe);
   return 0;
}

// accessible from threads, read only
static int p, t, n, option, blocking, procs, recvprocs, rank, gsize; 
//static sem_t *sems;
static pthread_barrier_t barr;
static unsigned long minsize, maxsize;

static struct ptrn_mprobe_cmd_struct args_info;


void *thr_receive(void *data) {
  
  long tid = (long)data;
  int cnt;

  int nrecvpeers = (p-recvprocs)/t;
  void **buf = (void**)malloc(sizeof(void*)*nrecvpeers);
  MPI_Request *reqs = (MPI_Request*)malloc(sizeof(MPI_Request)*nrecvpeers);

  for(int size=minsize; size<=maxsize;size*=2) {
    for(int c = 0; c<args_info.reps_arg; c++) {

      //if(args_info.verbose_given && !rank) printf("# THR c: %i/%i size: %i/%i\n", c,args_info.reps_arg, size, maxsize);

      if(!procs) {
        int ret=pthread_barrier_wait(&barr);
        if(ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) { perror("barr_wait()"); exit(1); }
        //ret=pthread_barrier_wait(&barr);
        //if(ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) { perror("barr_wait()"); exit(1); }
      }
     
      for(int i=0; i<n; i++) {
        for(int j=0; j<nrecvpeers; ) {
          int tag,src;
          src = tag = nrecvpeers*tid + j+1;
          //printf("tid: %i, recv from %i\n", tid, src);

          if(blocking) {
            switch(option) {
              case 1: DYN_Recv(MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &buf[j], &cnt, MPI_STATUS_IGNORE);
                      break;
              case 2: DYN_Recv(MPI_BYTE, src, MPI_ANY_TAG, MPI_COMM_WORLD, &buf[j], &cnt, MPI_STATUS_IGNORE);
                      break;
              case 3: DYN_Recv(MPI_BYTE, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &buf[j], &cnt, MPI_STATUS_IGNORE);
                      break;
              case 4: DYN_Recv(MPI_BYTE, src, tag, MPI_COMM_WORLD, &buf[j], &cnt, MPI_STATUS_IGNORE);
                      break;
            }
            j++;
          } else {
            int flag=0;
#ifdef HAVE_IRECV
            switch(option) {
              case 1: DYN_Irecv(MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &buf[j], &cnt, &flag, &reqs[j]);
                      break;
              case 2: DYN_Irecv(MPI_BYTE, src, MPI_ANY_TAG, MPI_COMM_WORLD, &buf[j], &cnt, &flag, &reqs[j]);
                      break;
              case 3: DYN_Irecv(MPI_BYTE, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &buf[j], &cnt, &flag, &reqs[j]);
                      break;
              case 4: DYN_Irecv(MPI_BYTE, src, tag, MPI_COMM_WORLD, &buf[j], &cnt, &flag, &reqs[j]);
                      break;
            }
#else
            printf("irecv disabled\n");
#endif
            if(flag) j++;
          }
        }
#ifdef HAVE_IRECV
        if(!blocking) MPI_Waitall(nrecvpeers,reqs,MPI_STATUSES_IGNORE);
#endif
        for(int j=0; j<nrecvpeers; j++) free(buf[j]);
      }

      // indicate that we're done
      if(!procs) {
        int ret=pthread_barrier_wait(&barr);
        if(ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) { perror("barr_wait()"); exit(1); }
      }
      //if(args_info.verbose_given && !rank) printf("# ATHR c: %i/%i size: %i/%i\n", c,args_info.reps_arg, size, maxsize);
    }
  }
  free(buf); free(reqs);
}

void *thr_ppsendrecv(void *data) {
  
  long tid = (long)data;
  int cnt;

  void *buf=(void*)malloc(maxsize);
  void *rbuf;
  int peer=rank+p/2;
  MPI_Request req;

  // wait until the main thread says: go!
  //if(!procs && sem_wait(&sems[tid]) != 0) { perror("sem_wait()"); }
  if(!procs) pthread_barrier_wait(&barr);
  
  //printf("[%i] sending to peer: %i, tid: %i\n", rank, peer, tid);
 
  for(int i=0; i<n;i++) {
    DYN_Send(buf, gsize, MPI_BYTE, peer /* dst */, tid /* tag */, MPI_COMM_WORLD);
    if(blocking) {
      switch(option) {
        case 1: DYN_Recv(MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rbuf, &cnt, MPI_STATUS_IGNORE);
                break;
        case 2: DYN_Recv(MPI_BYTE, peer, MPI_ANY_TAG, MPI_COMM_WORLD, &rbuf, &cnt, MPI_STATUS_IGNORE);
                break;
        case 3: DYN_Recv(MPI_BYTE, MPI_ANY_SOURCE, tid, MPI_COMM_WORLD, &rbuf, &cnt, MPI_STATUS_IGNORE);
                break;
        case 4: DYN_Recv(MPI_BYTE, peer, tid, MPI_COMM_WORLD, &rbuf, &cnt, MPI_STATUS_IGNORE);
                break;
      }
    } else {
      int flag=0;
#ifdef HAVE_IRECV
      while(!flag)
      switch(option) {
        case 1: DYN_Irecv(MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rbuf, &cnt, &flag, &req);
                break;
        case 2: DYN_Irecv(MPI_BYTE, peer, MPI_ANY_TAG, MPI_COMM_WORLD, &rbuf, &cnt, &flag, &req);
                break;
        case 3: DYN_Irecv(MPI_BYTE, MPI_ANY_SOURCE, tid, MPI_COMM_WORLD, &rbuf, &cnt, &flag, &req);
                break;
        case 4: DYN_Irecv(MPI_BYTE, peer, tid, MPI_COMM_WORLD, &rbuf, &cnt, &flag, &req);
                break;
      }
#else
        printf("irecv disabled\n");
#endif
    }
    if(!blocking) MPI_Wait(&req,MPI_STATUSES_IGNORE);
    free(rbuf);
  }
  if(!procs) pthread_barrier_wait(&barr);
}

void *thr_pprecvsend(void *data) {
  
  long tid = (long)data;
  int cnt;

  void *rbuf;
  int peer=rank-p/2;
  MPI_Request req;

  // wait until the main thread says: go!
  //if(!procs && sem_wait(&sems[tid]) != 0) { perror("sem_wait()"); }
  //printf("[%i] receiving from peer: %i, tid: %i\n", rank, peer, tid);
  if(!procs) pthread_barrier_wait(&barr);
 
  for(int i=0; i<n;i++) {
    if(blocking) {
      switch(option) {
        case 1: DYN_Recv(MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rbuf, &cnt, MPI_STATUS_IGNORE);
                break;
        case 2: DYN_Recv(MPI_BYTE, peer, MPI_ANY_TAG, MPI_COMM_WORLD, &rbuf, &cnt, MPI_STATUS_IGNORE);
                break;
        case 3: DYN_Recv(MPI_BYTE, MPI_ANY_SOURCE, tid, MPI_COMM_WORLD, &rbuf, &cnt, MPI_STATUS_IGNORE);
                break;
        case 4: DYN_Recv(MPI_BYTE, peer, tid, MPI_COMM_WORLD, &rbuf, &cnt, MPI_STATUS_IGNORE);
                break;
      }
    } else {
      int flag=0;
#ifdef HAVE_IRECV
      while(!flag)
      switch(option) {
        case 1: DYN_Irecv(MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &rbuf, &cnt, &flag, &req);
                break;
        case 2: DYN_Irecv(MPI_BYTE, peer, MPI_ANY_TAG, MPI_COMM_WORLD, &rbuf, &cnt, &flag, &req);
                break;
        case 3: DYN_Irecv(MPI_BYTE, MPI_ANY_SOURCE, tid, MPI_COMM_WORLD, &rbuf, &cnt, &flag, &req);
                break;
        case 4: DYN_Irecv(MPI_BYTE, peer, tid, MPI_COMM_WORLD, &rbuf, &cnt, &flag, &req);
                break;
      }
#else
        printf("irecv disabled\n");
#endif
    }
    if(!blocking) MPI_Wait(&req,MPI_STATUSES_IGNORE);
    DYN_Send(rbuf, gsize, MPI_BYTE, peer /* dst */, tid /* tag */, MPI_COMM_WORLD);
    free(rbuf);
  }
  if(!procs) pthread_barrier_wait(&barr);
}

static void mprobe_do_benchmarks(struct ng_module *module) {
	//parse cmdline arguments
	//printf("The string I got: %s\n", g_options.ptrnopts);
	if (ptrn_mprobe_parser_string(g_options.ptrnopts, &args_info, "netgauge") != 0) {
		exit(EXIT_FAILURE);
	}

  t=args_info.threads_arg;
  n=args_info.messages_arg;
  option=args_info.option_arg;
  minsize=g_options.min_datasize;
  maxsize=ng_min(g_options.max_datasize + module->headerlen, module->max_datasize);

  blocking = args_info.nonblocking_given ? 0 : 1;
  procs = args_info.procs_given;
  recvprocs = !procs ? 1 : t;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  ng_info(NG_VNORM, "pattern mprobe: p: %i, size: %i-%i, messages: %i, reps: %i, option: %i", p, minsize, maxsize, n, args_info.reps_arg, option);
  if(!args_info.latency_given)
  if(procs)  ng_info(NG_VNORM, "pattern mprobe (message rate): running with %i receiving processes (rank 0..%i)", t, t-1);
  else ng_info(NG_VNORM, "pattern mprobe (message rate): running with %i threads in a receiving processes (rank 0)", t);
  else if(procs)  ng_info(NG_VNORM, "pattern mprobe (latency): running with %i sending processes (rank 0..%i)", p/2, p/2-1);
  else ng_info(NG_VNORM, "pattern mprobe (latency): running with %i threads in each of the %i sending processes (rank 0..%i)", t, p/2, p/2-1);

  if(args_info.hostname_given) {
    const int len=1024;
    char name[len];
    gethostname(name, len);
    if(!rank) {
      printf("# receiver [%i] %s\n", rank, name);
      for(int j=1; j<p; j++) {
        MPI_Recv(name, len, MPI_CHAR, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(j < recvprocs) printf("# receiver [%i] %s\n", j, name);
        else printf("# [%i] %s (sending to rank %i)\n", j, name, j % recvprocs);
      }
    } else {
      MPI_Send(name, len, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
  } }


  DYN_Init_comm(0, MPI_COMM_WORLD);

  /* this benchmark consists of two different benchmarks - this is #1,
   * the message-rate benchmark */
  if(!args_info.latency_given) {

    if(((p-recvprocs) % t != 0) || (t<1) || (p<=2)) {
      ng_info(NG_VNORM, "please start me with (n (%i) -recvprocs (%i)) % t (%i) == 0 and p>2!\n", p, recvprocs, t);
      ng_abort("aborting\n");
    }

    if(procs) {
      ng_info(NG_VNORM, "non-threaded case not implemented\n");
      ng_abort("aborting\n");
    }

    // I will run t threads on rank 0 and every rank r \in {1..p-1} will
    // send to rank 0, I will synchronize the start and benchmark how long
    // it takes to receive all n messages 

    pthread_t* threads = (pthread_t*)malloc(t*sizeof(pthread_t));
    if(rank < recvprocs) {
      pthread_attr_t attr;
      pthread_attr_init(&attr);

      pthread_barrier_init(&barr, NULL, t+1);

      for(int thr=0; thr<t; thr++){
        int rc = pthread_create(&threads[thr], &attr, thr_receive, (void *)(long)thr);
        if(rc) { MPI_Abort(MPI_COMM_WORLD, 1); }
      }
    }

    void *sendbuf=malloc(maxsize);
    for(int size=minsize; size<=maxsize;size*=2) {
      unsigned long long win=0;
      std::vector<double> res;

      for(int c = 0; c<args_info.reps_arg; c++) {
        if(rank < recvprocs) {
          HRT_TIMESTAMP_T t1, t2;
          //printf("[%i] before barrier\n", rank);

          // synchronize all processes with fancy timer scheme
          MPI_Barrier(MPI_COMM_WORLD); // this is just to be extra-sure
          /*ng_sync_init_stage1(MPI_COMM_WORLD);
          ng_sync_init_stage2(MPI_COMM_WORLD, &win);
          long err = ng_sync(win);
          printf("[%i] after barrier\n", rank);*/

          // free threads to go!
          //printf("freeing threads\n");
          pthread_barrier_wait(&barr);

          HRT_GET_TIMESTAMP(t1);

          //pthread_barrier_wait(&barr);

          // wait until they're done
          pthread_barrier_wait(&barr);

          HRT_GET_TIMESTAMP(t2);

          uint64_t num_ticks;
          HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks);
          double us = 1e6 * num_ticks / (double) g_timerfreq;
          if(NG_VLEV1 & g_options.verbose) printf("[%i] c: %i size: %i took %lf us\n", rank, c, size, us);
          res.push_back(us);

        } else {
          //printf("[%i] before barrier\n", rank);
          // synchronize all processes with fancy timer scheme
          MPI_Barrier(MPI_COMM_WORLD); // this is just to be extra-sure
          /*ng_sync_init_stage1(MPI_COMM_WORLD);
          ng_sync_init_stage2(MPI_COMM_WORLD, &win);
          long err = ng_sync(win);
          printf("[%i] after barrier\n", rank);*/

          int peer = rank % recvprocs;

          for(int i=0; i<n;i++) DYN_Send(sendbuf, size, MPI_BYTE, peer /* dst */, rank /* tag */, MPI_COMM_WORLD);
        }
      } // c=0 -> args_info.reps_arg

      if(rank < recvprocs) {
        assert(args_info.reps_arg == res.size()); // safety first!
        double sum = std::accumulate(res.begin(), res.end(), 0.0, std::plus<double>());
        double result = sum/res.size();
        std::sort(res.begin(), res.end());
        int nmsgs=(p-recvprocs)*n/recvprocs;
        printf("[%i] size: %i average: %lf us ( %lf MOP/s), median: %lf us ( %lf MOP/s), min: %lf us ( %lf MOP/s)\n", 
            rank, size, result, nmsgs/result, res[res.size()/2], nmsgs/res[res.size()/2], res[0], nmsgs/res[0]);
      }
    } // size=minsize -> maxsize
    free(sendbuf);

    if(rank < recvprocs) {
      for(int j=0; j<t; j++) {
        pthread_join( threads[j], NULL);
      }
    }

    pthread_barrier_destroy(&barr);
    free(threads);

  } else {
  /* this benchmark consists of two different benchmarks - this is #2,
   * the latency benchmark */

    if((t<1) || (p%2!=0)) {
      ng_info(NG_VNORM, "please start me with p (%i) % 2 == 0 !\n", p);
      ng_abort("aborting\n");
    }

    for(gsize=minsize; gsize<=maxsize;gsize*=2) {
      std::vector<double> res;
      HRT_TIMESTAMP_T t1, t2;
    
      for(int c = 0; c<args_info.reps_arg; c++) {
        if(!procs) { // threaded case
          pthread_attr_t attr;
          pthread_t* threads = (pthread_t*)malloc(t*sizeof(pthread_t));
          pthread_attr_init(&attr);

          //sems = (sem_t*)malloc(t*sizeof(sem_t));
          
          /*for(int i=0; i<t; i++) {
            if(sem_init(&sems[i], 0, 0) != 0) { perror("sem_init()"); }
          }*/
          pthread_barrier_init(&barr, NULL, t+1);

          for(int thr=0; thr<t; thr++){
            int rc;
            if(rank < p/2) {
              rc = pthread_create(&threads[thr], &attr, thr_ppsendrecv, (void *)(long)thr);
            } else {
              rc = pthread_create(&threads[thr], &attr, thr_pprecvsend, (void *)(long)thr);
            }
            if(rc) { MPI_Abort(MPI_COMM_WORLD, 1); }
          }

          // TODO: synchronize all processes with fancy timer scheme
          MPI_Barrier(MPI_COMM_WORLD);

          // free threads to go!
          /*for(int i=0; i<t; i++) {
            if(sem_post(&sems[i]) != 0) { perror("sem_post()"); }
          }*/
          pthread_barrier_wait(&barr);

          HRT_GET_TIMESTAMP(t1);

          pthread_barrier_wait(&barr);

          HRT_GET_TIMESTAMP(t2);

          for(int i=0; i<t; i++) {
            pthread_join( threads[i], NULL);
          }

          /*for(int i=0; i<t; i++) {
            if(sem_destroy(&sems[i]) != 0) { perror("sem_destroy()"); }
          }*/
          pthread_barrier_destroy(&barr);
          free(threads);
          //free(sems);
        } else { // nonthreaded case
          t = 1; // set t to 1

          // TODO: synchronize all processes with fancy timer scheme
          MPI_Barrier(MPI_COMM_WORLD);

          HRT_GET_TIMESTAMP(t1);
          if(rank < p/2) {
            thr_ppsendrecv((void *)(long)rank);
          } else {
            thr_pprecvsend((void *)(long)rank);
          }
          HRT_GET_TIMESTAMP(t2);
        }

        uint64_t num_ticks;
        HRT_GET_ELAPSED_TICKS(t1, t2, &num_ticks);
        double us = 1e6 * num_ticks / (double) g_timerfreq / n / t;
        if(NG_VLEV1 & g_options.verbose) printf("[%i] size: %i took %lf us\n", rank, gsize, us);
        res.push_back(us);
      }

      if(rank < p/2) {
        assert(args_info.reps_arg == res.size()); // safety first!
        double sum = std::accumulate(res.begin(), res.end(), 0.0, std::plus<double>());
        double result = sum/res.size();
        assert(result != 0);
        std::sort(res.begin(), res.end());
        int nmsgs=(p-recvprocs)*n/recvprocs;
        printf("[%i] size: %i average: %lf us ( %lf MiB/s), median: %lf us ( %lf MiB/s)\n", rank, gsize, result, gsize/result, res[res.size()/2], gsize/res[res.size()/2]);
      }
    } // size=minsize -> maxsize
  }
}

} /* extern C */

#else
extern "C" {
int register_pattern_mprobe(void) {return 0;};
} /* extern C */
#endif
