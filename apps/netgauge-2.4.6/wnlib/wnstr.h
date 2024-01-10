/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/
#ifndef wnstrH
#define wnstrH


#include "wnlib.h"


void wn_stracpy(char *pout[],char in[]);
void wn_strncpy(char *out,char *in,int n);
void wn_stracat(char *pout[],char s1[],char s2[]);
void wn_stracat3
(
  char *pout[],
  char s1[],
  char s2[],
  char s3[]
);
void wn_stracat4
(
  char *pout[],
  char s1[],
  char s2[],
  char s3[],
  char s4[]
);
void wn_stracat5
(
  char *pout[],
  char s1[],
  char s2[],
  char s3[],
  char s4[],
  char s5[]
);
void wn_stracat6
(
  char *pout[],
  char s1[],
  char s2[],
  char s3[],
  char s4[],
  char s5[],
  char s6[]
);
void wn_strncat(char out[],char in[],int n);
bool wn_char_in_string(char c,char *s);


#endif
