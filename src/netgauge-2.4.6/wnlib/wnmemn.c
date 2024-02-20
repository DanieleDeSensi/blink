/**********************************************************************

  wn_initialize_group_for_no_free(group)

  local ptr alloc_piece(size,group)
  local ptr free_piece(p,group)

  local void verify_group(group)
  local int memory_used_in_group(group)

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

#include <stdio.h>

#include "wnlib.h"
#include "wnmax.h"
#include "wnasrt.h"

#include "wnmem.h"


#define PAD_BEGIN_MAGIC  (0x53562946)
#define PAD_END_MAGIC    (0xabcd5524)

typedef struct pad_type_struct *pad_type;
struct pad_type_struct
{
  int begin_magic;
  pad_type next;
  int end_magic;
};


extern bool wn_gp_fill_flag,wn_gp_pad_flag,wn_gp_trap_address_flag;
extern ptr wn_gp_trap_address_address;
extern ptr wn_system_alloc(int);


local void get_more_memory(int size,wn_memgp group)
{
  wn_mem_block block;
  int size_to_get,size_to_alloc;

  if(group->current_block != NULL)
  {
    group->current_block->leftover += group->block_mem_left;
  }

  size_to_get = wn_max(size,group->block_size);
  size_to_alloc = size_to_get + sizeof(struct wn_mem_block_struct) 
                  - sizeof(int); 
  block = (wn_mem_block)wn_system_alloc(size_to_alloc);

  block->next = group->current_block;
  group->current_block = block;

  block->leftover = 0;
  block->size = size_to_alloc;

  group->block_mem_left = size_to_get;
  group->block_ptr = (ptr)(&(block->memory));
}


local void fill_pad(pad_type pad,wn_memgp group)
{
  pad->begin_magic = PAD_BEGIN_MAGIC;
  pad->end_magic = PAD_END_MAGIC;

  pad->next = (pad_type)(group->pad_list);
  group->pad_list = (ptr)pad;
}


local ptr alloc_piece(int size,wn_memgp group)
{
  int old_size;
  ptr ret;

  if(wn_gp_pad_flag)
  {
    old_size = size;

    size += sizeof(struct pad_type_struct);
  }

  size += 4;

  if(size >= group->block_mem_left)
  {
    get_more_memory(size,group);
  }

  /* 8 align */
  ret = group->block_ptr;
  if((((unsigned long int)ret)&7) != 0)
  {
    ret = (ptr)(((char *)ret)+4);
  }  
  wn_assert((((unsigned long int)ret)&7) == 0);

  group->block_ptr = ((char *)(group->block_ptr)) + size;
  group->block_mem_left -= size;
  group->mem_used += size;

  if(wn_gp_pad_flag)
  {
    fill_pad((pad_type)(((char *)ret)+old_size),group);
  }
  if(wn_gp_trap_address_flag)
  {
    if(ret == wn_gp_trap_address_address)
    {
      fprintf(stderr,"address found.\n");
      wn_assert_notreached();
    }
  }

  return(ret);
}


/*ARGSUSED*/ local void free_piece(ptr p,wn_memgp group)
{
}

 
local void verify_pad(pad_type pad)
{
  wn_assert(pad->begin_magic == PAD_BEGIN_MAGIC);
  wn_assert(pad->end_magic == PAD_END_MAGIC);
}


local void verify_pad_list(pad_type pad_list)
{
  pad_type pad;

  for(pad=pad_list;pad != NULL;pad=pad->next)
  {
    verify_pad(pad);
  }
}


local void verify_group(wn_memgp group)
{
  if(wn_gp_pad_flag)
  {
    verify_pad_list((pad_type)(group->pad_list));
  }
}


void wn_initialize_group_for_no_free(wn_memgp group)
{
  group->pverify_group = (verify_group);
  group->pfree_piece = (free_piece);
  group->palloc_piece = (alloc_piece);
}



