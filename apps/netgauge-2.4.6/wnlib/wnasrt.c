/*************************************************************************

wn_set_assert_print(passert_print)

wn_assert(cond)
wn_assert_notreached()
wn_assert_warn(cond)
wn_assert_warn_notreached()

*************************************************************************/
/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "wnlib.h"
#include "wnasrt.h"



local bool initialized = FALSE;
local void (*psaved_assert_print)(char string[]);



local void default_assert_print(char string[])
{
  fprintf(stderr,"%s",string);
}


local void print_user_message(void)
{
  (*psaved_assert_print)(
      "------------------------------------------------------------\n");
  (*psaved_assert_print)(
      "                       HELP!!!\n");
  (*psaved_assert_print)(
      "This program has encountered a severe internal error.\n");
  (*psaved_assert_print)(
      "Please report the following message to the\n");
  (*psaved_assert_print)(
      "program's developers so that they can fix the problem:\n");
  (*psaved_assert_print)(
      "------------------------------------------------------------\n");
}


local void initialize_assert(void)
{
  if(!(initialized))
  {
    psaved_assert_print = (default_assert_print);

    initialized = TRUE;
  }
}


void wn_set_assert_print
(
  void (*passert_print)(char string[])
)
{
  initialize_assert();

  psaved_assert_print = passert_print;
}


void wn_assert_routine(char file_name[],int line_num)
{
  char string[150];

  initialize_assert();

  print_user_message();
  (void)sprintf(string,"%s[%d]: assertion botched -- forcing crash\n",
                file_name,line_num);
  (*psaved_assert_print)(string);

  abort();
}


void wn_assert_notreached_routine(char file_name[],int line_num)
{
  char string[150];

  initialize_assert();

  print_user_message();
  (void)sprintf(string,"%s[%d]: wn_assert_notreached() called -- forcing crash\n",
                file_name,line_num);
  (*psaved_assert_print)(string);

  abort();
}


void wn_assert_warn_routine(char file_name[],int line_num)
{
  char string[150];

  initialize_assert();

  print_user_message();
  (void)sprintf(string,"%s[%d]: assertion botched -- NOT forcing crash\n",
                file_name,line_num);
  (*psaved_assert_print)(string);
}


void wn_assert_warn_notreached_r(char file_name[],int line_num)
{
  char string[150];

  initialize_assert();

  print_user_message();
  (void)sprintf(string,"%s[%d]: wn_assert_warn_notreached() called -- NOT forcing crash\n",
                file_name,line_num);
  (*psaved_assert_print)(string);
}


