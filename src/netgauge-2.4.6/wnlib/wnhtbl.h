/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/
#ifndef wnhtblH
#define wnhtblH


#include "wnlib.h"
#include "wnhtab.h"


void wn_mkhashhtab(wn_htab *ptable);
void wn_mkstrhtab(wn_htab *ptable);
void wn_mkptrhtab(wn_htab *ptable);
void wn_mkinthtab(wn_htab *ptable);


#endif

