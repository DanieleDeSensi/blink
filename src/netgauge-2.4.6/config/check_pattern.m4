# $1 - pattern name, $2 - pattern c-file, $3 - pattern define
AC_DEFUN([HTOR_CHECK_PATTERN], [
AC_MSG_CHECKING([pattern $1])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[
#define $3
#include "$2"
                   ]],[[]])],[
AC_MSG_RESULT([yes])                   
AC_DEFINE($3, 1, pattern $1 ($2))                   
                   ], [
AC_MSG_RESULT([no])                   
])
])
