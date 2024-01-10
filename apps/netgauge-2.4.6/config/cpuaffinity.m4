AC_DEFUN([HTOR_CHECK_CPUAFFINITY], [
AC_MSG_CHECKING([Linux CPU affinity])
AC_RUN_IFELSE(
  [AC_LANG_PROGRAM([[
#define __USE_GNU
#define _GNU_SOURCE 
#include <sched.h>
  ]],[[
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(0, &mask);
  if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
    return 1;
  }
  return 0;
                   ]])],[
AC_MSG_RESULT([yes])                   
AC_DEFINE(HAVE_CPUAFFINITY, 1, cpu affinity)                   
                   ], [
AC_MSG_RESULT([no])                   
], [
AC_MSG_RESULT([no])                   
])
])
