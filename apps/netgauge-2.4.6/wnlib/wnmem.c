/**********************************************************************

FUNCTIONS PROVIDED:

  wn_gpmake(parms)
  wn_gpfree()
  wn_gppush(group)
  wn_gppop()
  
  ptr wn_alloc(size)
  ptr wn_zalloc(size)
  wn_free(p)

  wn_realloc(&p,old_size,new_size)
  wn_zrealloc(&p,old_size,new_size)

  wn_memgp wn_curgp();
  wn_memgp wn_defaultgp();
  
  wn_gperrfpush(pfunc)
  wn_gperrfpop()  * not implemented *

  wn_make_gpstack(&stack)
  wn_get_current_gpstack(stack)
  wn_set_current_gpstack(stack)
  wn_swap_current_gpstack(stack)
  
DEBUGGING:

  wn_gp_fill();
  wn_gp_pad();
  wn_stack_fill();
  wn_gp_args_check();  * not implemented *
  wn_gp_trap_address(address);
  
  wn_allmem_verify();
  wn_group_verify(group);

  int wn_mem_used()
  int wn_group_mem_used(group)

  wn_print_mem_used()
  wn_print_all_groups_mem_used()
  wn_print_group_mem_used(group)

  bool wn_mem_in_group(p,group)

  wn_print_gp_stack()

  wn_gplabel(label)

**********************************************************************/
/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/

#include "../config.h"
#include <stdio.h>
#include <sys/types.h>

#if HAVE_MALLOC_H
	#include <malloc.h>
#endif
#if HAVE_STDLIB_H
	#include <stdlib.h>
#endif

#include <string.h>

#include "wnlib.h"
#include "wnasrt.h"
#include "wnmemb.h"

#include "wnmem.h"



#define BEGIN_MAGIC      (0x58b2c29d)
#define END_MAGIC        (0x81c102bf)

local bool initialized = FALSE;

local wn_gpstack current_gpstack;

local wn_memgp group_list = NULL;
local wn_memgp default_group = NULL;

bool wn_gp_fill_flag=FALSE,
     wn_gp_pad_flag=FALSE,
     wn_gp_trap_address_flag=FALSE;
ptr wn_gp_trap_address_address;

ptr wn_system_alloc(/*size*/);

local void default_error_func(int size);
typedef void (*voidfunc)(int size);
local voidfunc perror_func = (default_error_func);

void wn_initialize_group_for_general_free(wn_memgp group);
void wn_initialize_group_for_no_free(wn_memgp group);


ptr wn_system_alloc(int size)
{
  ptr ret;

  ret = malloc((unsigned int)size);

  if(ret == NULL)  /* out of memory */
  {
    (*perror_func)(size);  /* print out of memory message and crash,
                              or free up necessary memory */

    ret = (ptr)malloc((unsigned int)size);   /* try again */

    if(ret == NULL)    
    {
      fprintf(stderr,"out of memory.\n");
      wn_assert_notreached();  /* no second chance */
    }
  }

  if(wn_gp_fill_flag)
  {
    wn_memset(ret,'a',size); 
  }

  return(ret);
}


void wn_system_free(ptr mem,int size)
{
  if(wn_gp_fill_flag)
  {
    wn_memset(mem,'f',size); 
  }

  free(mem);
}


local void print_group(wn_memgp group)
{
  char *label;

  if(group == NULL)
  {
    printf("%20s  %p", "", (ptr) group);
  }
  else
  {
    label = group->label;

    if(label == NULL)
    {
      label = "(NULL)";
    }

    printf("%20s  %p", label, (ptr)group);
  }
}


void wn_print_gp_stack(void)
{
  int i;
  wn_memgp group;

  printf("************** memory group stack *************\n");

  printf("top -> ");    /* top of stack */
  print_group(current_gpstack->current_group);
  printf("\n");

  for(i=current_gpstack->stack_ptr;i>=0;i--)
  {
    group = (current_gpstack->group_stack)[i];

    printf("       "); 
    print_group(group);
    printf("\n");
  }

  printf("***********************************************\n");
}


wn_memgp wn_curgp(void)
{
  if(!(initialized))
  {
    wn_gppush(wn_defaultgp());
  }

  if(current_gpstack->stack_ptr < 0)
  {
    return(NULL);
  }
  else
  {
    return(current_gpstack->current_group);
  }
}


void wn_gppush(wn_memgp group)
{
  ++(current_gpstack->stack_ptr);

  wn_assert(current_gpstack->stack_ptr < GROUP_STACK_SIZE);

  (current_gpstack->group_stack)[current_gpstack->stack_ptr] =
                                      current_gpstack->current_group;
  current_gpstack->current_group = group;
}


void wn_gppop(void)
{
  current_gpstack->current_group =
               (current_gpstack->group_stack)[current_gpstack->stack_ptr];

  --(current_gpstack->stack_ptr);
}


local void make_system_gpstack(wn_gpstack *pstack)    
{
  *pstack = (wn_gpstack)wn_system_alloc(sizeof(struct wn_gpstack_struct));

  (*pstack)->stack_ptr = -1;
  (*pstack)->group_stack = (wn_memgp *)
                        wn_system_alloc(GROUP_STACK_SIZE*sizeof(wn_memgp));
  (*pstack)->current_group = NULL;
}


void wn_make_gpstack(wn_gpstack *pstack)    
{
  *pstack = (wn_gpstack)wn_zalloc(sizeof(struct wn_gpstack_struct));

  (*pstack)->stack_ptr = -1;
  (*pstack)->group_stack = (wn_memgp *)
                               wn_zalloc(GROUP_STACK_SIZE*sizeof(wn_memgp));
  (*pstack)->current_group = NULL;
}


void wn_get_current_gpstack(wn_gpstack *pstack)
{
  *pstack = current_gpstack;
}


void wn_set_current_gpstack(wn_gpstack stack)
{
  current_gpstack = stack;
}


local void initialize_memory_allocator(void)
{
  make_system_gpstack(&current_gpstack);   

  initialized = TRUE;
}


local void make_memory_group 
( 
  wn_memgp *pgroup, 
  char parms[], 
  wn_memgp parent_group
)
{
  char group_type[50];
  int group_size;

  *pgroup = (wn_memgp)wn_system_alloc(sizeof(struct wn_memgp_struct));

  (*pgroup)->begin_magic = BEGIN_MAGIC;
  (*pgroup)->end_magic = END_MAGIC;

  (*pgroup)->next = group_list;
  if(group_list != NULL)
  {
    group_list->plast = &((*pgroup)->next);
  }
  (*pgroup)->plast = &group_list;
  group_list = (*pgroup);

  if(parent_group != NULL)
  {
    (*pgroup)->sub_next = parent_group->children;
    if(parent_group->children != NULL)
    {
      parent_group->children->psub_last = &((*pgroup)->sub_next);
    }
    (*pgroup)->psub_last = &(parent_group->children);
    parent_group->children = (*pgroup);
  }
  else
  {
    (*pgroup)->sub_next = NULL;
    (*pgroup)->psub_last = NULL;
  }

  (*pgroup)->current_block = NULL;

  (*pgroup)->block_size = 4000;
  (*pgroup)->block_ptr = NULL;
  (*pgroup)->block_mem_left = 0;
  (*pgroup)->mem_used = 0;

  (*pgroup)->pad_list = NULL;
  (*pgroup)->children = NULL;

  (*pgroup)->label = NULL;

  strcpy(group_type,"");
  group_size = 0;
  sscanf(parms,"%s %d",group_type,&group_size); 

  if(strcmp(group_type,"no_free") == 0)
  {
    wn_initialize_group_for_no_free(*pgroup);
  }
  else if(strcmp(group_type,"general_free") == 0)
  {
    wn_initialize_group_for_general_free(*pgroup);
  }
  else
  {
    fprintf(stderr,"make_memory_group: illegal type of memory group\n");
    wn_assert_notreached();
  }

  if(group_size > 0)
  {
    (*pgroup)->block_size = group_size;
  }
}


local void label_group(char *label,wn_memgp group)
{
  wn_assert(group->label == NULL);

  group->label = label;
}


void wn_gplabel(char *label)
{
  char *label_copy;
  int save_mem_used;

  wn_assert(current_gpstack->stack_ptr >= 0);
   
  save_mem_used = current_gpstack->current_group->mem_used;

  label_copy = (char *)wn_alloc(strlen(label)+1);
  strcpy(label_copy,label);
  label_group(label_copy,current_gpstack->current_group);

  current_gpstack->current_group->mem_used = save_mem_used;
}


wn_memgp wn_defaultgp(void)
{
  if(!(initialized))
  {
    initialize_memory_allocator();
  }

  if(default_group == NULL)
  {
    make_memory_group(&default_group,"general_free",(wn_memgp)NULL);

    label_group("default_group",default_group);
  }

  return(default_group);
}


void wn_gpmake(char parms[])
{
  wn_memgp ret;

  if(!(initialized))
  {
    initialize_memory_allocator();
  }

  make_memory_group(&ret,parms,current_gpstack->current_group);
  wn_gppush(ret);
}


local void free_group(wn_memgp group);

local void free_groups_children(wn_memgp group)
{
  wn_memgp child,next;

  next = group->children;

  for(;;)
  {
    if(next == NULL)
    {
      break;
    }

    child = next;
    next = child->sub_next;

    free_group(child);
  }
}


local void free_group_memory(wn_memgp group)
{
  wn_mem_block block,next;

  next = group->current_block;
  for(;;)
  {
    if(next == NULL)
    {
      break;
    }

    block = next;
    next = block->next;

    wn_system_free((ptr)block,block->size);
  }
}


local void remove_group_from_group_list(wn_memgp group)
{
  *(group->plast) = group->next;
  if(group->next != NULL)
  {
    group->next->plast = group->plast;
  }
}


local void remove_group_from_parent_subgroup_list(wn_memgp group)
{
  if(group->psub_last != NULL)
  {
    *(group->psub_last) = group->sub_next;
    if(group->sub_next != NULL)
    {
      group->sub_next->psub_last = group->psub_last;
    }
  }
}


local void free_group(wn_memgp group)
{
  free_groups_children(group);

  free_group_memory(group);

  remove_group_from_group_list(group);
  remove_group_from_parent_subgroup_list(group);

  wn_memset(group,'f',sizeof(struct wn_memgp_struct)); 

  wn_system_free((ptr)group,sizeof(struct wn_memgp_struct));
}


void wn_gpfree(void)
{
  free_group(current_gpstack->current_group);
  wn_gppop();
}


#define align(_size) \
{\
  (_size) = ((_size)+3)&(~3);\
}


ptr wn_zalloc(int size)
{
  ptr p;

  if(size == 0)
  {
    return(NULL);
  }
  if(!(initialized))
  {
    wn_gppush(wn_defaultgp());
  }

  align(size);

  p = (*(current_gpstack->current_group->palloc_piece))(
                            size,current_gpstack->current_group);

  wn_memzero(p,size);

  return(p);
}


ptr wn_alloc(int size)
{
  if(size == 0)
  {
    return(NULL);
  }
  if(!(initialized))
  {
    wn_gppush(wn_defaultgp());
  }

  align(size);

  return(
          (*(current_gpstack->current_group->palloc_piece))(
                            size,current_gpstack->current_group)
        );
}


void wn_free(ptr p)
{
  if(p == NULL)  /* zero size block from wn_alloc */
  {
    return;
  }

  (*(current_gpstack->current_group->pfree_piece))(
                                          p,current_gpstack->current_group);
}


void wn_realloc(ptr *pp,int old_size,int new_size)
{
  ptr new_p;

  new_p = wn_alloc(new_size);
  wn_memcpy(new_p,*pp,old_size);
  wn_free(*pp);

  *pp = new_p;
}


void wn_zrealloc(ptr *pp,int old_size,int new_size)
{
  ptr new_p;

  new_p = wn_zalloc(new_size);
  wn_memcpy(new_p,*pp,old_size);
  wn_free(*pp);

  *pp = new_p;
}


void wn_gperrfpush(void (*pfunc)(int size))
{
  perror_func = pfunc;
}


/*ARGSUSED*/ local void default_error_func(int size)
{
  fprintf(stderr,"out of memory\n");
  wn_assert_notreached();
}


void wn_gp_fill(void)
{
  wn_gp_fill_flag = TRUE;
}


void wn_gp_pad(void)
{
  wn_gp_pad_flag = TRUE;
}


local void stack_fill(int size)
{
  int filler[1000/sizeof(int)];

  if(size < 0)
  {
    return;
  }

  wn_memset(filler,'s',1000);

  stack_fill(size-1000);
}


void wn_stack_fill(void)
{
  stack_fill(100000);  /* fill 64k of stack */
}


void wn_gp_trap_address(ptr address)
{
  wn_gp_trap_address_flag = TRUE;
  wn_gp_trap_address_address = address;
}


local bool group_is_in_group_list(wn_memgp group)
{
  wn_memgp g;

  for(g=group_list;g != NULL;g=g->next)
  {
    if(group == g)  /* group is known */
    {
      return(TRUE);
    }
  }

  return(FALSE);
}


local void verify_group_stack_group(wn_memgp group)
{
  if(!(group_is_in_group_list(group)))
  {
    fprintf(stderr,"bad group in group stack.\n");
    wn_assert_notreached();
  }
}


local void generic_verify_group(wn_memgp group)
{
  wn_assert(group->begin_magic == BEGIN_MAGIC);
  wn_assert(group->end_magic == END_MAGIC);
}


local void verify_group(wn_memgp group)
{
  generic_verify_group(group);

  (*(group->pverify_group))(group);
}


local void verify_group_stack(void)
{
  int i;
  wn_memgp group;

  if(current_gpstack == NULL)   /* no stack created yet */
  {
    return;
  }
  if(current_gpstack->stack_ptr == -1)  /* stack is empty */
  {
    return;
  }

  verify_group_stack_group(current_gpstack->current_group);

  for(i=current_gpstack->stack_ptr;i>0;i--) /* don't include 0, 0 always NULL */
  {
    group = (current_gpstack->group_stack)[i];

    verify_group_stack_group(group);
  }
}


local void verify_all_groups(void)
{
  wn_memgp group;

  for(group=group_list;group != NULL;group=group->next)
  {
    verify_group(group);
  }
}


void wn_group_verify(wn_memgp group)
{
  if(!(group_is_in_group_list(group)))
  {
    fprintf(stderr,"bad group passed.\n");
    wn_assert_notreached();
  }

  verify_group(group);
}


void wn_allmem_verify(void)
{
  verify_group_stack();
  verify_all_groups();
}


local int memory_used_in_group(wn_memgp group)
{
  return(group->mem_used);
}


int wn_mem_used(void)
{
  int total;
  wn_memgp group;

  total = 0;

  for(group=group_list;group != NULL;group=group->next)
  {
    total += memory_used_in_group(group);
  }

  return(total);
}


void wn_print_mem_used(void)
{
  char string[150];

  (void)sprintf(string,"total memory used = %d\n",wn_mem_used());
  fputs(string,stdout);
  fflush(stdout);
}


void wn_print_group_mem_used(wn_memgp group)
{
  char string[150];

  (void)sprintf(string,"total memory used for group = %d\n",
                       wn_group_mem_used(group));
  fputs(string,stdout);
  fflush(stdout);
}


void wn_print_all_groups_mem_used(void)
{
  int total,used,num_groups;
  wn_memgp group;

  total = 0;
  num_groups = 0;

  printf("=======================================\n"); 
  printf("                       group       used\n"); 
  printf("=======================================\n"); 

  for(group=group_list;group != NULL;group=group->next)
  {
    used = memory_used_in_group(group);

    print_group(group);
    printf("  %9d\n",used); 

    total += used;
    num_groups++;
  }

  printf("---------------------------------------\n"); 
  printf("          total (%3d groups)  %9d\n",num_groups,total); 
  printf("=======================================\n"); 
}


int wn_group_mem_used(wn_memgp group)
{
  int total;

  total = memory_used_in_group(group);

  return(total);
}


local bool memory_is_in_block(ptr p,wn_mem_block block)
{
  long unsigned int start,fin,p_address;

  p_address = (long unsigned int)p;

  start = (long unsigned int)block;
  fin = start + block->size;

  return(
          (start <= p_address)
            &&
          (p_address < fin)
        );
}


bool wn_mem_in_group(ptr p,wn_memgp group)
{
  wn_mem_block block;

  for(block = group->current_block;
      block != NULL;
      block = block->next)
  {
    if(memory_is_in_block(p,block))
    {
      return(TRUE);
    }
  }

  return(FALSE);
}


