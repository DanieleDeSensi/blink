/**********************************************************************

  wn_initialize_group_for_general_free(group)

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
#include "wnmemb.h"

#include "wnmem.h"


extern bool wn_gp_fill_flag,wn_gp_pad_flag,wn_gp_trap_address_flag;
extern ptr wn_gp_trap_address_address;
extern ptr wn_system_alloc(int);



#define PAD_BEGIN_MAGIC  0x53562946
#define PAD_END_MAGIC    0xabcd5524

typedef struct pad_type_struct *pad_type;
struct pad_type_struct
{
  int begin_magic;
  pad_type *plast,next;
  int end_magic;
};



#define SMALL        100
#define SMALL_SIZE   (SMALL<<2)

typedef struct free_block_type_struct *free_block_type;
typedef struct free_list_type_struct *free_list_type;
struct free_list_type_struct
{
  free_block_type small_blocks[SMALL];
  free_block_type big_blocks;
};
  
struct free_block_type_struct
{
  int size;
  free_block_type next;    
};
  
typedef struct allocated_block_type_struct *allocated_block_type;
struct allocated_block_type_struct
{
  int size;
  int memory;     
};
  


local void insert_into_block_list
(
  free_block_type *pblock_list,
  free_block_type free_block
)
{
  free_block->next = *pblock_list;
  *pblock_list = free_block;
}

 
local void insert_free_block_into_free_list
(
  free_list_type free_list,
  free_block_type free_block
)
{
  int size;

  /* assert proper alignment */
  wn_assert((((long unsigned int)free_block)&7) == 4);

  size = free_block->size;

  if(size < SMALL_SIZE)
  {
    insert_into_block_list(&((free_list->small_blocks)[size>>2]),free_block);  
  }
  else
  {
    insert_into_block_list(&(free_list->big_blocks),free_block);  
  }
}


local void remove_free_block_from_list(free_block_type *pfree_block)
{
  *pfree_block = (*pfree_block)->next;
}


local void get_memory_from_small_blocks
(
  allocated_block_type *pallocated_block,
  int size,
  free_list_type free_list
)
{
  free_block_type *pfree_block;

  pfree_block = &((free_list->small_blocks)[size>>2]);

  if(*pfree_block != NULL)
  {
    *pallocated_block = (allocated_block_type)(*pfree_block); 

    remove_free_block_from_list(pfree_block);
  }
  else
  {
    *pallocated_block = NULL;
  }
}


local void get_memory_from_big_blocks
(
  allocated_block_type *pallocated_block,
  int size,
  free_list_type free_list
)
{
  free_block_type *pfree_block,*pfound_free_block;

  pfound_free_block = NULL;

  for(pfree_block = &(free_list->big_blocks);
      *pfree_block != NULL;
      pfree_block = &((*pfree_block)->next))
  {
    if(((*pfree_block)->size >= size)&&((*pfree_block)->size < 2*size))
    {
      if(
          (pfound_free_block == NULL)
            ||
          ((*pfree_block)->size < (*pfound_free_block)->size)
        )
      {
        pfound_free_block = pfree_block;
      }
    }
  }

  if(pfound_free_block != NULL)
  {
    *pallocated_block = (allocated_block_type)(*pfound_free_block); 

    remove_free_block_from_list(pfound_free_block);
  }
  else
  {
    *pallocated_block = NULL;
  }
}


local void get_memory_from_free_list
(
  allocated_block_type *pallocated_block,
  int size,
  free_list_type free_list
)
{
  if(size < SMALL_SIZE)
  {
    get_memory_from_small_blocks(pallocated_block,size,free_list);
  }
  else
  {
    get_memory_from_big_blocks(pallocated_block,size,free_list);
  }
}


local void get_more_memory(int size,wn_memgp group)
{
  free_block_type free_block;
  wn_mem_block block;
  int size_to_get,size_to_alloc;

  if(group->block_mem_left > 4)
  {
    free_block = (free_block_type)(group->block_ptr);
    if((((long unsigned int)free_block)&7) == 0)
    {
      free_block = (free_block_type)(((char *)free_block)+4);
      group->current_block->leftover += 4;
    }
    free_block->size = group->block_mem_left - sizeof(int);

    insert_free_block_into_free_list((free_list_type)(group->free_list),
                                     free_block);
  }
  else if(group->block_mem_left > 0)
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

   
local void simple_alloc(ptr *pmem,int size,wn_memgp group)
{
  if(size >= group->block_mem_left)
  {
    get_more_memory(size,group);
  }

  *pmem = group->block_ptr;
  group->block_ptr = ((char *)(group->block_ptr)) + size;
  group->block_mem_left -= size;
}


local void get_new_memory
(
  allocated_block_type *pallocated_block,
  int size,
  wn_memgp group
)
{
  simple_alloc((ptr *)pallocated_block,
               size+sizeof(int)+4,group);

  if((((long unsigned int)(*pallocated_block))&7) == 0)
  {
    *pallocated_block = (allocated_block_type)(((char *)(*pallocated_block))+4);
  }
  wn_assert((((long unsigned int)(*pallocated_block))&7) == 4);

  (*pallocated_block)->size = size;
}


local void allocate_block
(
  allocated_block_type *pallocated_block,
  int size,
  wn_memgp group
)
{
  get_memory_from_free_list(pallocated_block,
                            size,(free_list_type)(group->free_list));

  if(*pallocated_block == NULL)
  {
    get_new_memory(pallocated_block,size,group);
  }
}


local void make_pad(pad_type pad,wn_memgp group)
{
  pad_type next;

  pad->begin_magic = PAD_BEGIN_MAGIC;
  pad->end_magic = PAD_END_MAGIC;

  next = (pad_type)(group->pad_list);
  if(next != NULL)
  {
    next->plast = &(pad->next);
  }
  pad->next = next;
  group->pad_list = (ptr)pad;
  pad->plast = (pad_type *)(&(group->pad_list));
}


local void delete_pad(pad_type pad)
{
  pad_type next;

  next = pad->next;
  if(next != NULL)
  {
    next->plast = pad->plast;
  }
  *(pad->plast) = next;
}


local ptr alloc_piece(int size,wn_memgp group)
{
  int old_size,pad_offset;
  allocated_block_type allocated_block;
  ptr ret;

  if(wn_gp_pad_flag)
  {
    old_size = size;
    size += sizeof(struct pad_type_struct);
  }

  allocate_block(&allocated_block,size,group);

  ret = (ptr)(&(allocated_block->memory));

  if(wn_gp_pad_flag)
  {
    pad_offset = allocated_block->size - sizeof(struct pad_type_struct);

    make_pad((pad_type)(((char *)ret) + pad_offset),group);

    group->mem_used += pad_offset;
  }
  else
  {
    group->mem_used += allocated_block->size;
  }
  if(wn_gp_fill_flag)
  {
    if(wn_gp_pad_flag)
    {
      wn_memset(ret,'a',old_size);
    }
    else
    {
      wn_memset(ret,'a',size);
    }
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
  allocated_block_type allocated_block;

  allocated_block = (allocated_block_type)(((char *)p)-sizeof(int));

  if(wn_gp_pad_flag)
  {
    delete_pad(
       (pad_type)(((char *)p) + 
		  allocated_block->size - sizeof(struct pad_type_struct))
              );
    group->mem_used -=
        (allocated_block->size - sizeof(struct pad_type_struct));
  }
  else
  {
    group->mem_used -= allocated_block->size;
  }

  if(wn_gp_fill_flag)
  {
    wn_memset(p,'f',allocated_block->size);
  }

  insert_free_block_into_free_list((free_list_type)(group->free_list),
                                   (free_block_type)allocated_block);
}


local void verify_pad(pad_type pad)
{
  wn_assert(pad->begin_magic == PAD_BEGIN_MAGIC);
  wn_assert(pad->end_magic == PAD_END_MAGIC);
  wn_assert(*(pad->plast) == pad);
}


local void verify_pad_list(pad_type pad_list)
{
  pad_type pad;

  for(pad=pad_list;pad != NULL;pad=pad->next)
  {
    verify_pad(pad);
  }
}


local void verify_small_free_block
(
  free_block_type free_block,
  int size,
  wn_memgp group
)
{
  wn_assert(free_block->size == size);
  wn_assert(free_block->next != free_block);
  wn_assert(wn_mem_in_group(free_block,group));
}


local void verify_small_free_block_list
(
  free_block_type free_block_list,
  int size,
  wn_memgp group
)
{
  free_block_type free_block;

  for(free_block = free_block_list;
      free_block != NULL;
      free_block = free_block->next)
  {
    verify_small_free_block(free_block,size,group);
  }
}


local void verify_big_free_block(free_block_type free_block,wn_memgp group)
{
  wn_assert(free_block->size >= SMALL_SIZE);
  wn_assert(free_block->next != free_block);
  wn_assert(wn_mem_in_group(free_block,group));
}


local void verify_big_free_block_list(free_block_type free_block_list,
				      wn_memgp group)
{
  free_block_type free_block;

  for(free_block = free_block_list;
      free_block != NULL;
      free_block = free_block->next)
  {
    verify_big_free_block(free_block,group);
  }
}


local void verify_free_list(wn_memgp group)
{
  free_list_type free_list;
  int size;

  free_list = (free_list_type)(group->free_list);

  for(size = 4;size < SMALL_SIZE;size += 4)
  {
    verify_small_free_block_list((free_list->small_blocks)[size>>2],size,
                                 group);
  }

  verify_big_free_block_list(free_list->big_blocks,group);
}


local void verify_group(wn_memgp group)
{
  if(wn_gp_pad_flag)
  {
    verify_pad_list((pad_type)(group->pad_list));
  }

  verify_free_list(group);
}


local void make_free_list(free_list_type *pfree_list,wn_memgp group)
{
  simple_alloc((ptr *)pfree_list,
               sizeof(struct free_list_type_struct),group);

  wn_memzero(*pfree_list,sizeof(struct free_list_type_struct)); 
}


void wn_initialize_group_for_general_free(wn_memgp group)
{
  group->pverify_group = (verify_group);
  group->pfree_piece = (free_piece);
  group->palloc_piece = (alloc_piece);

  make_free_list((free_list_type *)(&(group->free_list)),group);
}




