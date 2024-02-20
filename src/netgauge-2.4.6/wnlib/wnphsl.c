/**************************************************************************

wn_mkptrhtab(&table)

**************************************************************************/
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
#include "wnmem.h"
#include "wnstr.h"
#include "wneq.h"
#include "wncpy.h"
#include "wnnop.h"
#include "wnhash.h"
#include "wnhtab.h"

#include "wnhtbl.h"


/*#if defined(wn_mkptrhtab)*/
void wn_mkptrhtab(wn_htab *ptable)
{
  wn_mkhtab(ptable,(wn_ptrhash),(wn_ptreq),(wn_ptrcpy),
	    (void (*)(ptr))(wn_do_nothing));
}
/*#endif*/



