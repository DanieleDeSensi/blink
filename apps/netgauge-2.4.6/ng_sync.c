/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include "netgauge.h"
#include <stdint.h>
#include "hrtimer/hrtimer.h"

#ifdef NG_MPI

#define SYNC_WINDOW

#ifdef SYNC_HTOR
static unsigned long long g_diff; // difference from me to node 0
static unsigned long long g_next; // next window starts

/* simple MPI_Barrier synchronization mechanism */
void ng_sync_init_stage1(MPI_Comm comm) {

  int notsmaller = 0;
  unsigned long long min = ~0x0, mdiff;
  int rank, peer;

  MPI_Comm_rank(comm, &rank);

  if(rank == 0) {
    peer = 1;
    while (1) {
      HRT_TIMESTAMP_T ts_start, ts_end;
      unsigned long long tstart, trem, rttticks;
      int dst = 1;
      MPI_Comm comm=MPI_COMM_WORLD;

      HRT_GET_TIMESTAMP(ts_start);
      HRT_GET_TIME(ts_start, tstart);

      MPI_Send(&tstart, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
      MPI_Recv(&trem, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm, MPI_STATUS_IGNORE);

      HRT_GET_TIMESTAMP(ts_end);
      HRT_GET_ELAPSED_TICKS(ts_start, ts_end, &rttticks);
      //printf("notsmaller: %i\n", notsmaller);
   
      if(rttticks < min) {
        min = rttticks;
        notsmaller = 0;
        mdiff = tstart - rttticks/2 - trem; // difference to remote host for the minimal RTT 
        //printf("tstart: %llu, rttticks: %llu, trem: %llu (%lf)\n", tstart, rttticks, mdiff, (double)mdiff/(double)g_timerfreq*1e6);
      } else {
        notsmaller++;
        if(notsmaller > 100) {
          trem = 0;
          MPI_Send(&trem, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
          break;
        }
      }
    }
    MPI_Send(&mdiff, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
    g_diff = 0;
  } else {
    peer = 0;
    while(1) {
      unsigned long long trem;
      HRT_TIMESTAMP_T ts;

      MPI_Recv(&trem, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm, MPI_STATUS_IGNORE);
      if(trem == 0) break;

      HRT_GET_TIMESTAMP(ts);
      HRT_GET_TIME(ts, trem);

      MPI_Send(&trem, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
    }
    MPI_Recv(&g_diff, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm, MPI_STATUS_IGNORE);
  }

  HRT_TIMESTAMP_T ts;
  unsigned long long time;
  HRT_GET_TIMESTAMP(ts);
  HRT_GET_TIME(ts, time);
  //printf("time difference: %llu (%lf) - time: %llu\n", g_diff,(double)g_diff/(double)g_timerfreq*1e6, time);
}

void ng_sync_init_stage2(MPI_Comm comm, unsigned long long *win) {
  int i;
  HRT_TIMESTAMP_T ts1, ts2;
  unsigned long long ticks, time, mticks;

  HRT_GET_TIMESTAMP(ts1);
  for(i=0; i<10; i++) {
    MPI_Bcast(&g_next, 1, MPI_UNSIGNED_LONG_LONG, 0, comm);
  }
  HRT_GET_TIMESTAMP(ts2);
  HRT_GET_ELAPSED_TICKS(ts1, ts2, &ticks);

  // stupid MPI-2.1 doesn't reduce MPI_UNSIGNED_LONG_LONG :-(
  //MPI_Reduce(&ticks, &mticks, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, 0, comm);
  long long lticks=(long long)ticks, lmticks;
  MPI_Reduce(&lticks, &lmticks, 1, MPI_LONG_LONG, MPI_MAX, 0, comm);
  mticks = lmticks;

  // this goes into ng_sync ...

  HRT_GET_TIMESTAMP(ts2);
  HRT_GET_TIME(ts2, time);
  g_next = time + mticks;
  MPI_Bcast(&g_next, 1, MPI_UNSIGNED_LONG_LONG, 0, comm);
  g_next = g_next - g_diff;

  //printf("offset: %llu (%lf), time: %llu waiting %lf us until: %llu\n", g_diff, (double)g_diff/(double)g_timerfreq*1e6, time, (double)(g_next-time)/HRT_RESOLUTION*1e6, g_next);
  do {
    HRT_GET_TIMESTAMP(ts2);
    HRT_GET_TIME(ts2, time);
  } while(time < g_next);

  // testing only 
  
  {
    unsigned long long roottime,timediff;
    int rank;
    MPI_Comm_rank(comm, &rank);
    HRT_GET_TIMESTAMP(ts1);
    HRT_GET_TIME(ts1,time);
    roottime = time;
    MPI_Bcast(&roottime, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    timediff = roottime - time;
    //if(rank) printf("[%i] %i diff: %lf us\n", rank, i, 1e6*(double)timediff/(double)g_timerfreq);
  }


}

long ng_sync(unsigned long long win) {
}
#endif

#ifdef SYNC_BARRIER
/* simple MPI_Barrier synchronization mechanism */
void ng_sync_init_stage1(t_mpiargs *mpiargs) {

}

void ng_sync_init_stage2(t_mpiargs *mpiargs) {

}

double ng_sync(t_mpiargs *mpiargs) {
  MPI_Barrier(mpiargs->comm);
  return 0;
}
#endif

#ifdef SYNC_DISSEMINATION
/* internal dissemination algorithm */
void ng_sync_init_stage1(t_mpiargs *mpiargs) {

}

void ng_sync_init_stage2(t_mpiargs *mpiargs) {

}

/* done at the beginning of every new nprocs */
double ng_sync(t_mpiargs *mpiargs) {
  MPI_Comm comm = mpiargs->comm;
  int round = -1, src, dst, maxround, size, i, rank;
  char buf;

  MPI_Comm_size(comm, &size);
  MPI_Comm_rank(comm, &rank);

  /* round starts with 0 -> round - 1 */
  maxround = (int)ceil((double)log(size)) - 1;
            
  do {
    round++;
    dst = (rank + round) % size;
    MPI_Send(&buf, 1, MPI_BYTE, dst, 0, comm);
    /* + size*size that the modulo does never get negative ! */
    src = ((rank - round) + size*size) % size;
    MPI_Recv(&buf, 1, MPI_BYTE, src, 0, comm, MPI_STATUS_IGNORE);
  } while (round < maxround);     

  return 0;
}
#endif

#ifdef SYNC_WINDOW
static unsigned long long *diffs=NULL; /* global array of all diffs to all ranks - only
                  completely valid on rank 0 */
static unsigned long long gdiff=0; /* the is the final time diff to rank 0 :-) */
static unsigned long long gnext=0; /* start-time for next round - synchronized with rank 0 */

#define NUMBER_SMALLER 100 /* do RTT measurement until n successive
                             messages are *not* smaller than the current
                             smallest one */

static inline unsigned long long p2psynch(int peer, int client, int server, MPI_Comm comm) {
  unsigned long long diff;
  int notsmaller = 0; /* count number of RTTs that are *not* smaller than
                  the current smallest one */
  unsigned long long smallest = (unsigned long long)(~0); /* the current smallest time */
  long tries=0;
  int res;
  HRT_TIMESTAMP_T ts_start, ts_end;
  unsigned long long tstart, /* local start time */
             tend, /* local end time */
             trem, tmpdiff;
  do {
    /* the client sends a ping to the server and waits for a pong (and
     * takes the RTT time). It repeats this procedure until the last
     * NUMBER_SMALLER RTTs have not been smaller than the smallest
     * (tries to find the smallest RTT). When the smallest RTT is
     * found, it sends a special flag (0d) to the server that it knows
     * that the benchmark is finished. The client computes the diff
     * with this smallest RTT with the scheme described in the paper.
     * */
    if(client) {
      //tstart = MPI_Wtime();
      HRT_GET_TIMESTAMP(ts_start);
      HRT_GET_TIME(ts_start, tstart);
      res = MPI_Send(&tstart, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
      //printf("[%i] client: waiting for response from %i\n", r, peer);
      res = MPI_Recv(&trem, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm, MPI_STATUS_IGNORE);
      //printf("[%i] client: got response from %i\n", r, peer);
      //tend = MPI_Wtime();
      HRT_GET_TIMESTAMP(ts_end);
      HRT_GET_TIME(ts_end, tend);
      tmpdiff = tstart + (tend-tstart)/2 - trem;
      
      // pure statistics
      tries++;

      if(tend-tstart < smallest) {
        //printf("[%i] reset notsmaller %lli < %lli (tries: %lu)\n", r, tend-tstart, smallest, tries);
        smallest = tend-tstart;
        notsmaller = 0;
        diff = tmpdiff; /* save new smallest diff-time */
      } else {
        if(++notsmaller == NUMBER_SMALLER) {
          /* send abort flag to client */
          trem = 0;
          res = MPI_Send(&trem, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
          //printf("[%i] diff to %i: %lli (tries: %lu)\n", r, peer, diff, tries);
          break;
        }
      }
      //printf("[%i] notsmaller: %i\n", r, notsmaller);
    }

    /* The server just replies with the local time to the client
     * requests and aborts the benchmark if the abort flag (0d) is
     * received in any of the requests. */
    if(server) {
      //printf("[%i] server: waiting for ping from %i\n", r, peer);
      res = MPI_Recv(&tstart, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm, MPI_STATUS_IGNORE);
      //printf("[%i] server: got ping from %i (%lf) \n", r, peer, tstart);
      if(tstart == 0) break; /* this is the signal from the client to stop */
      //trem = MPI_Wtime(); /* fill in local time on server */
      HRT_GET_TIMESTAMP(ts_start);
      HRT_GET_TIME(ts_start, trem);
      res = MPI_Send(&trem, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
    }
    /* this loop is only left with a break */
  } while(1);

  return diff;
}

/* time-window based synchronization mechanism 
 * - */
void ng_sync_init_stage1(MPI_Comm comm) {
  int p, r, res, dist, round;
  int maxpower, group2;


  res = MPI_Comm_rank(comm, &r);
  res = MPI_Comm_size(comm, &p);
  
  /* reallocate tha diffs array with the right size */
  if(diffs != NULL) free(diffs);
  diffs = (unsigned long long*)calloc(1, p*sizeof(unsigned long long));
  
  /* check if p is power of 2 */
  { int i=1;
    while((i = i << 1) < p) {};
    /*if(i != p) { 
      printf("communicator size (%i) must be power of 2 (%i)!\n", p, i);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }*/
    if(i==p) maxpower = i;
    else maxpower = i/2;
    group2 = p-maxpower;
  }

  if(r < maxpower) {
    //printf("[%i] in group 1\n", r); 

    dist = 1; /* this gets left-shifted (<<) every round and is after 
                     $\lceil log_2(p) \rceil$ rounds >= p */
    round = 1; /* fun and printf round counter - not really needed */
    do {
      int peer; /* synchronization peer */
      int client, server;
      unsigned long long tstart, /* local start time */
             tend, /* local end time */
             trem; /* remote time */
             
      long long tmpdiff, /* temporary difference to remote clock */
                diff; /* difference to remote clock */
      HRT_TIMESTAMP_T ts_start, ts_end;
      client = 0; server = 0;
      client = ((r % (dist << 1)) == 0);
      server = ((r % (dist << 1)) == dist);
      
      if(server) {
        peer = r - dist;
        if(peer < 0) server = 0;
        //if(server) printf("(%i) %i <- %i\n", round, r, peer);
      }
      if(client) {
        peer = r + dist;
        if(peer >= p) client = 0;
        //if(client) printf("(%i) %i -> %i\n", round, peer, r);
      }
      if(!client && !server) break; /* TODO: leave loop if no peer left -
                                       works only for power of two process
                                       groups */

      /* synchronize clocks with peer */
      diff = p2psynch(peer, client, server, comm);
      
      /* the client measured the time difference to his peer-server of the
       * current round. Since rank 0 is the global synchronization point,
       * rank 0's array has to be up to date and the other clients have to
       * communicate all their knowledge to rank 0 as described in the
       * paper. */
      
      if(client) {
        /* all clients just measured the time difference to node r + diff
         * (=peer) */
        diffs[peer] = diff;

        /* we are a client - we need to receive all the knowledge
         * (differences) that the server we just synchronized with holds!
         * Our server has been "round-1" times client and measures
         * "round-1" diffs */
        if(round > 1) {
          unsigned long long *recvbuf; /* receive the server's data */
          int items, i;

          items = (1 << (round-1))-1;
          recvbuf = (unsigned long long*)malloc(items*sizeof(unsigned long long));
          
          res = MPI_Recv(recvbuf, items, MPI_UNSIGNED_LONG_LONG, peer, 0, comm, MPI_STATUS_IGNORE);
          
          //printf("[%i] round: %i, client merges %i items\n", r, round, items);
          /* merge data into my own field */
          for(i=0; i<items; i++) {
            diffs[peer+i+1] =  diffs[peer] /* diff to server */ + 
                            recvbuf[i] /* received time */; // TODO: + or - ???
          }
          free(recvbuf);
        }
      }

      if(server) {
        /* we are a server, we need to send all our knowledge (time
         * differences to our client */
      
        /* we have measured "round-1" nodes at the end of round "round"
         * and hold $2^(round-1)-1$ diffs at this time*/
        if(round > 1) {
          int i, tmpdist, tmppeer, items;
          unsigned long long *sendbuf;
          
          items = (1 << (round-1))-1;
          sendbuf = (unsigned long long*)malloc(items*sizeof(unsigned long long));

          //printf("[%i] round: %i, server sends %i items\n", r, round, items);

          /* fill buffer - every server holds the $2^(round-1)-1$ next
           * diffs */
          for(i=0; i<items; i++) {
            sendbuf[i] = diffs[r+i+1];
          }
          res = MPI_Send(sendbuf, items, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
          free(sendbuf);
        }
      }
      
      dist = dist << 1;
      round++;
    } while(dist < maxpower);

    /* scatter all the time diffs to the processes */
    MPI_Scatter(diffs, 1, MPI_UNSIGNED_LONG_LONG, &gdiff, 1, MPI_UNSIGNED_LONG_LONG, 0, comm);

    // I have a peer in group 2 ...
    if(r<group2) {
      unsigned long long diff;

      int peer = r+maxpower;
      diff = p2psynch(peer, 1, 0, comm);
      diff = gdiff + diff;
      res = MPI_Send(&diff, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm);
    }
  } else {
    unsigned long long diff;
    //printf("[%i] in group 2\n", r); 

    int peer = r-maxpower;
    diff = p2psynch(peer, 0, 1, comm);
    MPI_Recv(&gdiff, 1, MPI_UNSIGNED_LONG_LONG, peer, 0, comm, MPI_STATUS_IGNORE);
  }
  //printf("[%i] diff: %ld\n", r, gdiff);

  /*if(!r) {
    int i;
    for(i=0; i<p; i++) {
      printf("%lf ", diffs[i]*1e6);
    }
    printf("\n");
  }*/

}

/* done at the beginning of every new size */
void ng_sync_init_stage2(MPI_Comm comm, unsigned long long *win) {
  unsigned long long time, bcasttime; /* measure MPI_Bcast() time :-/ */
  int i;

  HRT_TIMESTAMP_T t1, t2;


  HRT_GET_TIMESTAMP(t1);
  for(i=0; i<10; i++) {
    MPI_Bcast(&gnext, 1, MPI_UNSIGNED_LONG_LONG, 0, comm);
  }
  HRT_GET_TIMESTAMP(t2);
  HRT_GET_ELAPSED_TICKS(t1, t2, &gnext);

  /* get maximum bcasttime */
  // stupid MPI-2.1 doesn't reduce MPI_UNSIGNED_LONG_LONG :-(
  long long lgnext=(long long)gnext, lbcasttime;
  //MPI_Reduce(&gnext, &bcasttime, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, 0, comm);
  MPI_Reduce(&lgnext, &lbcasttime, 1, MPI_LONG_LONG, MPI_MAX, 0, comm);
  bcasttime = lbcasttime;

  /* rank 0 sets base-time to a time when the bcast is expected to be
   * finished */
  HRT_GET_TIMESTAMP(t1);
  HRT_GET_TIME(t1,time);

  gnext = time + bcasttime;
  MPI_Bcast(&gnext, 1, MPI_UNSIGNED_LONG_LONG, 0, comm);

  gnext= gnext - gdiff; /* adjust rank 0's time to local time */
}

int syncctr=0;
volatile int NG_Dummy_var=0; /* avoid optimizations */

/* for every single measurement */
long ng_sync(unsigned long long win) {
  long err = 0;
  HRT_TIMESTAMP_T ts;
  unsigned long long time;

  HRT_GET_TIMESTAMP(ts);
  HRT_GET_TIME(ts,time);

  if(time > gnext) {
    //printf("[%i] tooooo late!!!! :-( (%lf)\n", mpiargs->rank, (MPI_Wtime()-gnext)*1e6);
    err = time-gnext;
  } else {
    /* wait */
    //printf("waiting ... %llu\n", gnext-time);
    while(time < gnext) {
      NG_Dummy_var++;
      HRT_GET_TIMESTAMP(ts);
      HRT_GET_TIME(ts,time);
    };
  }
  
  //int rank;
  //MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //printf("[%i] syncctr: %i (next: %lf, err: %lf)\n", rank, syncctr++, gnext, err);
  
  gnext = gnext + win;
  //printf("leaving ... err: %lf, winlen: %lf\n", err, win);
  return err;
}
#endif
#endif
