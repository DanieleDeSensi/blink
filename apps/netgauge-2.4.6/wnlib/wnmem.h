/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/
#ifndef wnmemH
#define wnmemH


#include "wnlib.h"



typedef struct wn_mem_block_struct *wn_mem_block;
typedef struct wn_memgp_struct *wn_memgp;

struct wn_memgp_struct
{
  int begin_magic;

  wn_memgp next,*plast;
  wn_memgp children,sub_next,*psub_last;

  wn_mem_block current_block;
  ptr block_ptr;
  int block_mem_left;
  int block_size;
  int mem_used;

  ptr pad_list,free_list;

  void (*pverify_group)(wn_memgp group),
       (*pfree_piece)(ptr p,wn_memgp group);
  ptr  (*palloc_piece)(int size,wn_memgp group);

  char *label;

  int end_magic;
};

struct wn_mem_block_struct
{
  wn_mem_block next;
  int size,leftover;
  int memory;   /* memory begins after here */
};


#define GROUP_STACK_SIZE    100

typedef struct wn_gpstack_struct *wn_gpstack;
struct wn_gpstack_struct      /* kludge for now */
{
  int stack_ptr;
  wn_memgp *group_stack; 
  wn_memgp current_group;
};
  

void wn_print_gp_stack(void);

wn_memgp wn_curgp(void);
wn_memgp wn_defaultgp(void);
void wn_gppush(wn_memgp group);
void wn_gppop(void);
void wn_gpmake(char parms[]);
void wn_gpfree(void);

ptr wn_zalloc(int size);
ptr wn_alloc(int size);
void wn_free(ptr p);
void wn_realloc(ptr *pp,int old_size,int new_size);
void wn_zrealloc(ptr *pp,int old_size,int new_size);

void wn_get_current_gpstack(wn_gpstack *pstack);
void wn_set_current_gpstack(wn_gpstack stack);

void wn_gplabel(char *label);
void wn_gperrfpush(void (*pfunc)());
void wn_gp_fill(void);
void wn_gp_pad(void);
void wn_stack_fill(void);
void wn_gp_trap_address(ptr address);
void wn_allmem_verify(void);
int wn_mem_used(void);
void wn_print_mem_used(void);
int wn_group_mem_used(wn_memgp group);
void wn_print_group_mem_used(wn_memgp group);
void wn_print_all_groups_mem_used(void);
bool wn_mem_in_group(ptr p,wn_memgp group);



#endif

