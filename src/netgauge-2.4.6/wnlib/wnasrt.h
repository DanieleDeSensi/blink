/****************************************************************************

COPYRIGHT NOTICE:

  The source code in this file is provided free of charge
  to the author's consulting clients.  It is in the
  public domain and therefore may be used by anybody for
  any purpose.

AUTHOR:

  Will Naylor

****************************************************************************/


#define wn_assert(_cond)\
{\
  if(!(_cond))\
  {\
    wn_assert_routine(__FILE__,__LINE__);\
  }\
}
       

#define wn_assert_notreached()  wn_assert_notreached_routine(__FILE__,__LINE__)


#define wn_assert_warn(_cond)\
{\
  if(!(_cond))\
  {\
    wn_assert_warn_routine(__FILE__,__LINE__);\
  }\
}
       

#define wn_assert_warn_notreached() \
		wn_assert_warn_notreached_r(__FILE__,__LINE__)


void wn_set_assert_print
(
  void (*passert_print)(char string[])
);


void wn_assert_routine(char file_name[],int line_num);

void wn_assert_notreached_routine(char file_name[],int line_num);

void wn_assert_warn_routine(char file_name[],int line_num);

void wn_assert_warn_notreached_r(char file_name[],int line_num);

