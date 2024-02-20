/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/
#ifndef wncpyH
#define wncpyH


#include "wnlib.h"


void wn_ptrcpy(ptr *pp,ptr p);
void wn_intcpy(int *pi,int i);
void wn_pdoublecpy(double **pd,double *d);
void wn_memcpy(ptr out,ptr in,int size);
void wn_memacpy(ptr *pout,ptr in,int size);


#endif


