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
#include "wnrtab.h"

#include "wnhash.h"



int wn_ptrhash(ptr p)
{
  int ret;

  ret = (int)((unsigned long)p);

  ret ^= (ret >> 2);

  return(ret);
}
