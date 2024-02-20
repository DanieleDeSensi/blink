/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/
#ifndef wneqH
#define wneqH



#include "wnlib.h"



bool wn_streq(char s1[],char s2[]);
bool wn_streqnc(char s1[],char s2[]);
bool wn_inteq(int i1,int i2);
bool wn_ptreq(ptr p1,ptr p2);
bool wn_ptrNULLeq(bool *psuccess,ptr p1,ptr p2);
bool wn_memeq(ptr m1,ptr m2,int len);



#endif
