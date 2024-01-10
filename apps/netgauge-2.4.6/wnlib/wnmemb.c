/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/

#include "wnlib.h"
#include <string.h>

#include "wnmem.h"
#include "wnmemb.h"



/*extern void *memset(void *,int,unsigned int); */
/*
extern int memcmp(void *,void *,unsigned int);
*/



void wn_memset(ptr out,char c,int n)
{
  (void)memset(out,(int)c,n);
}


void wn_memzero(ptr out,int n)
{
  wn_memset(out,'\0',n);
}



