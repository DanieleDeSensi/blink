#include "netgauge.h"
#ifdef NG_MPI

#include <mpi.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <set>
#include <pthread.h>


#define TWODMAP
//#define WRONGMATCHING
//#define MPROBE

#ifdef TWODMAP

namespace twodmap {
  typedef struct {} any; /* denotes any type */ 

  template <typename T>
  class twodmap_t {
    public:

      twodmap_t() {
        zero_d = false;
        zero_d_act = 0;
        one_d_act[0] = 0;
        one_d_act[1] = 0;
        if(sem_init(&sem, 0, 0) != 0) { perror("sem_init()"); }
        pthread_mutex_init(&classlock, NULL);
      }

      ~twodmap_t() {
        if(sem_destroy(&sem) != 0) { perror("sem_destroy()"); }
      }

      bool trylock(T x,T y) {
        bool ret=true;
        pthread_mutex_lock(&classlock);
#ifdef NOANY
        if(zero_d ||
          (one_d_x.end() != one_d_x.find(x)) ||
          (one_d_y.end() != one_d_y.find(y)) ||
          (two_d.end() != two_d.find(std::make_pair(x,y)))) ret = false;
        else {
          two_d.insert(std::make_pair(x,y));
          two_d_x_act[x]++;
          two_d_y_act[y]++;
          zero_d_act++;
          //printf("xy %i %i %i\n", two_d_x_act[x], two_d_y_act[y], zero_d_act);
          //printf("(x,y)\n");
        }
#else
        if((two_d.end() != two_d.find(std::make_pair(x,y)))) ret = false;
        else two_d.insert(std::make_pair(x,y));
#endif
        pthread_mutex_unlock(&classlock);
        return ret;
      }
      
      /* y is * */
      bool trylock(T x, any y) {
        bool ret=true;
        pthread_mutex_lock(&classlock);
        if(zero_d ||
          (one_d_act[1]) || 
          (two_d_x_act[x]) ||
          (one_d_x.end() != one_d_x.find(x))) ret = false;
        else {
          one_d_x.insert(x);
          zero_d_act++;
          one_d_act[0]++;
          //printf("x* %i * %i\n", two_d_x_act[x], zero_d_act);
          //printf("(x,*)\n");
        }
        pthread_mutex_unlock(&classlock);
        return ret;
      }
      
      /* x is * */
      bool trylock(any x, T y) {
        bool ret=true;
        pthread_mutex_lock(&classlock);
        if(zero_d ||
          (one_d_act[0]) || 
          (two_d_y_act[y]) ||
          (one_d_y.end() != one_d_y.find(y))) ret = false;
        else {
          one_d_y.insert(y);
          zero_d_act++;
          one_d_act[1]++;
          //printf("*y * %i %i\n", two_d_y_act[y], zero_d_act);
          //printf("(*,y)\n");
        }
        pthread_mutex_unlock(&classlock);
        return ret;
      }

      /* x and y are * */
      bool trylock(any x, any y) {
        bool ret=true;
        pthread_mutex_lock(&classlock);
        if(zero_d || zero_d_act) ret = false;
        else { 
          zero_d = true;
        //printf("(*,*)\n"); 
        }
        pthread_mutex_unlock(&classlock);
        return ret;
      }
      
      template <typename F, typename G>
      void lock(G x, F y) {
        while(true) {
          if(trylock(x,y)) {
            sem_trywait(&sem);
            return;
          }
          if(sem_wait(&sem) != 0) { perror("sem_wait()"); }
        }
      }

      void unlock(T x,T y) {
        pthread_mutex_lock(&classlock);
        two_d.erase(std::make_pair(x,y));
#ifndef NOANY
        two_d_x_act[x]--;
        two_d_y_act[y]--;
        zero_d_act--;
#endif
        pthread_mutex_unlock(&classlock);
        if(sem_post(&sem) != 0) { perror("sem_post()"); }
      }
      
      /* y is * */
      void unlock(T x, any y) {
        pthread_mutex_lock(&classlock);
        one_d_x.erase(x);
        zero_d_act--;
        one_d_act[0]--;
        pthread_mutex_unlock(&classlock);
        if(sem_post(&sem) != 0) { perror("sem_post()"); }
      }
      
      /* x is * */
      void unlock(any x, T y) {
        pthread_mutex_lock(&classlock);
        one_d_y.erase(y);
        zero_d_act--;
        one_d_act[1]--;
        pthread_mutex_unlock(&classlock);
        if(sem_post(&sem) != 0) { perror("sem_post()"); }
      }

      /* x and y are * */
      void unlock(any x, any y) {
        pthread_mutex_lock(&classlock);
        zero_d = false;
        pthread_mutex_unlock(&classlock);
        if(sem_post(&sem) != 0) { perror("sem_post()"); }
      }

    private:
      /* this consists of several lists:
       * - a 2-d list with all (x,y) pairs that are set
       * - a 1-d list with all x that are set
       * - a 1-d list with all y that are set
       * - a 0-d list (scalar) which indicates if all x,y are set
       */
      std::set<std::pair<T,T> > two_d;
      std::set<T> one_d_x, one_d_y;
      std::map<T,int> two_d_x_act, two_d_y_act;
      volatile bool zero_d;
      volatile int zero_d_act, one_d_act[2];
      pthread_mutex_t classlock;

      sem_t sem;
  };
}

typedef struct {
  twodmap::twodmap_t<int> map;
} DYN_Comminfo_t;

/* the keyval (global) */
static int gkeyval=MPI_KEYVAL_INVALID;

static int DYN_Key_copy(MPI_Comm oldcomm, int keyval, void *extra_state, void *attribute_val_in, void *attribute_val_out, int *flag) {
  /* delete the attribute in the new comm  - it will be created at the
   * first usage */
  *flag = 0;

  return MPI_SUCCESS;
}

static int DYN_Key_delete(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state) {

  if(keyval != gkeyval) {
    printf("Got wrong keyval!(%i)\n", keyval);
  }

  return MPI_SUCCESS;
}

int DYN_Init_comm(int flags, MPI_Comm comm) {

  /* fetch attribute (our structure from communicator - attach it if it
   * isn't there already */
  if(MPI_KEYVAL_INVALID == gkeyval) {
    int res = MPI_Keyval_create(DYN_Key_copy, DYN_Key_delete, &(gkeyval), NULL);
    if((MPI_SUCCESS != res)) { printf("Error in MPI_Keyval_create() (%i)\n", res); return res; }
  }
  
  int rank; MPI_Comm_rank(comm, &rank);
  //printf("[%i] creating new comminfo\n", rank);
  DYN_Comminfo_t* comminfo = new DYN_Comminfo_t();

  /* put the new attribute to the comm */
  int res = MPI_Attr_put(comm, gkeyval, comminfo);
  if((MPI_SUCCESS != res)) { printf("Error in MPI_Attr_put() (%i)\n", res); return MPI_ERR_INTERN; }

  return MPI_SUCCESS;
}

int DYN_Send(void *buf, int count, MPI_Datatype datatype, int dest, 
                  int tag, MPI_Comm comm) {
  return MPI_Send(buf, count, datatype, dest, tag, comm);
}


int DYN_Recv(MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
             void **buf, int *count, MPI_Status *status ) {

  int res;
  DYN_Comminfo_t* comminfo;
  int flag;
  res = MPI_Attr_get(comm, gkeyval, &comminfo, &flag);
  if((MPI_SUCCESS != res)) { printf("Error in MPI_Attr_get() (%i)\n", res); return res; }

  if (!flag) {
    printf("Communicator not initialized!\n");
    MPI_Abort(MPI_COMM_WORLD, 10);
  }

  /* begin the different cases */
  if(src != MPI_ANY_SOURCE && tag != MPI_ANY_TAG) {
    comminfo->map.lock(src,tag);

    MPI_Status stat;
    MPI_Probe(src, tag, comm, &stat);
    MPI_Get_count(&stat, datatype, count);
    MPI_Aint extent;
    MPI_Type_extent(datatype, &extent);
    *buf = malloc(extent**count);
    MPI_Request req;
    MPI_Irecv(*buf, *count, datatype, src, tag, comm, &req);
    comminfo->map.unlock(src,tag);

    MPI_Wait(&req, status);

  } else 
  if(src != MPI_ANY_SOURCE && tag == MPI_ANY_TAG) {
    comminfo->map.lock(src,twodmap::any());
    
    MPI_Status stat;
    MPI_Probe(src, tag, comm, &stat);
    MPI_Get_count(&stat, datatype, count);
    MPI_Aint extent;
    MPI_Type_extent(datatype, &extent);
    *buf = malloc(extent**count);
    MPI_Request req;
    MPI_Irecv(*buf, *count, datatype, src, tag, comm, &req);
    comminfo->map.unlock(src,twodmap::any());

    MPI_Wait(&req, status);
  
  } else 
  if(src == MPI_ANY_SOURCE && tag != MPI_ANY_TAG) {
    comminfo->map.lock(twodmap::any(),tag);
    
    MPI_Status stat;
    MPI_Probe(src, tag, comm, &stat);
    MPI_Get_count(&stat, datatype, count);
    MPI_Aint extent;
    MPI_Type_extent(datatype, &extent);
    *buf = malloc(extent**count);
    MPI_Request req;
    MPI_Irecv(*buf, *count, datatype, src, tag, comm, &req);
    comminfo->map.unlock(twodmap::any(),tag);

    MPI_Wait(&req, status);

  } else 
  if(src == MPI_ANY_SOURCE && tag == MPI_ANY_TAG) {
    comminfo->map.lock(twodmap::any(),twodmap::any());

    MPI_Status stat;
    MPI_Probe(src, tag, comm, &stat);
    MPI_Get_count(&stat, datatype, count);
    MPI_Aint extent;
    MPI_Type_extent(datatype, &extent);
    *buf = malloc(extent**count);
    MPI_Request req;
    MPI_Irecv(*buf, *count, datatype, src, tag, comm, &req);
    comminfo->map.unlock(twodmap::any(),twodmap::any());

    MPI_Wait(&req, status);
  }

  return MPI_SUCCESS;
}

int DYN_Irecv(MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
             void **buf, int *count, int *flag, MPI_Request *req ) {

  int res;
  DYN_Comminfo_t* comminfo;
  int f;
  res = MPI_Attr_get(comm, gkeyval, &comminfo, &f);
  if((MPI_SUCCESS != res)) { printf("Error in MPI_Attr_get() (%i)\n", res); return res; }

  if (!f) {
    printf("Communicator not initialized!\n");
    MPI_Abort(MPI_COMM_WORLD, 10);
  }

  /* begin the different cases */
  if(src != MPI_ANY_SOURCE && tag != MPI_ANY_TAG) {
    comminfo->map.lock(src,tag);

    MPI_Status stat;
    MPI_Iprobe(src, tag, comm, flag, &stat);
    if(*flag) {
      MPI_Get_count(&stat, datatype, count);
      MPI_Aint extent;
      MPI_Type_extent(datatype, &extent);
      *buf = malloc(extent**count);
      MPI_Irecv(*buf, *count, datatype, src, tag, comm, req);
    }
    comminfo->map.unlock(src,tag);

  } else 
  if(src != MPI_ANY_SOURCE && tag == MPI_ANY_TAG) {
    comminfo->map.lock(src,twodmap::any());
    
    MPI_Status stat;
    MPI_Iprobe(src, tag, comm, flag, &stat);
    if(*flag) {
      MPI_Get_count(&stat, datatype, count);
      MPI_Aint extent;
      MPI_Type_extent(datatype, &extent);
      *buf = malloc(extent**count);
      MPI_Irecv(*buf, *count, datatype, src, tag, comm, req);
    }
    comminfo->map.unlock(src,twodmap::any());
  
  } else 
  if(src == MPI_ANY_SOURCE && tag != MPI_ANY_TAG) {
    comminfo->map.lock(twodmap::any(),tag);
    
    MPI_Status stat;
    MPI_Iprobe(src, tag, comm, flag, &stat);
    if(*flag) {
      MPI_Get_count(&stat, datatype, count);
      MPI_Aint extent;
      MPI_Type_extent(datatype, &extent);
      *buf = malloc(extent**count);
      MPI_Irecv(*buf, *count, datatype, src, tag, comm, req);
    }
    comminfo->map.unlock(twodmap::any(),tag);

  } else 
  if(src == MPI_ANY_SOURCE && tag == MPI_ANY_TAG) {
    comminfo->map.lock(twodmap::any(),twodmap::any());

    MPI_Status stat;
    MPI_Iprobe(src, tag, comm, flag, &stat);
    if(*flag) {
      MPI_Get_count(&stat, datatype, count);
      MPI_Aint extent;
      MPI_Type_extent(datatype, &extent);
      *buf = malloc(extent**count);
      MPI_Irecv(*buf, *count, datatype, src, tag, comm, req);
    }
    comminfo->map.unlock(twodmap::any(),twodmap::any());
  }

  return MPI_SUCCESS;
}

#endif

#ifdef WRONGMATCHING

int DYN_Init_comm(int flags, MPI_Comm comm) {return 0;}

int DYN_Send(void *buf, int count, MPI_Datatype datatype, int dest, 
                  int tag, MPI_Comm comm) {
  return MPI_Send(buf, count, datatype, dest, tag, comm);
}

int DYN_Recv(MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
             void **buf, int *count, MPI_Status *status ) {

  MPI_Status stat;
  MPI_Probe(src, tag, comm, &stat);
  MPI_Get_count(&stat, datatype, count);
  MPI_Aint extent;
  MPI_Type_extent(datatype, &extent);
  *buf = malloc(extent**count);
  MPI_Request req;
  MPI_Irecv(*buf, *count, datatype, src, tag, comm, &req);
  MPI_Wait(&req, status);

  return 0;
}

int DYN_Irecv(MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
             void **buf, int *count, int *flag, MPI_Request *req ) {
  MPI_Status stat;
  MPI_Iprobe(src, tag, comm, flag, &stat);
  if(*flag) {
    MPI_Get_count(&stat, datatype, count);
    MPI_Aint extent;
    MPI_Type_extent(datatype, &extent);
    *buf = malloc(extent**count);
    MPI_Irecv(*buf, *count, datatype, src, tag, comm, req);
  }

  return 0;
}
#endif

#ifdef MPROBE

int DYN_Init_comm(int flags, MPI_Comm comm) {return 0;}

int DYN_Send(void *buf, int count, MPI_Datatype datatype, int dest, 
                  int tag, MPI_Comm comm) {
  return MPI_Send(buf, count, datatype, dest, tag, comm);
}

int DYN_Recv(MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
             void **buf, int *count, MPI_Status *status ) {

  MPI_Status stat;
  MPI_Message msg;

  MPI_Mprobe(src, tag, comm, &msg, &stat);
  MPI_Get_count(&stat, datatype, count);
  MPI_Aint extent;
  MPI_Type_extent(datatype, &extent);
  *buf = malloc(extent**count);
  MPI_Request req;
  MPI_Imrecv(*buf, *count, datatype, &msg, &req);
  MPI_Wait(&req, status);

  return 0;
}

int DYN_Irecv(MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
             void **buf, int *count, int *flag, MPI_Request *req ) {
  MPI_Status stat;
  MPI_Message msg;

  MPI_Improbe(src, tag, comm, flag, &msg, &stat);
  if(*flag) {
    MPI_Get_count(&stat, datatype, count);
    MPI_Aint extent;
    MPI_Type_extent(datatype, &extent);
    *buf = malloc(extent**count);
    MPI_Imrecv(*buf, *count, datatype, &msg, req);
  }

  return 0;
}
#endif
#endif
