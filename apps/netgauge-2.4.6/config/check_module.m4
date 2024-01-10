# $1 - module name, $2 - module c-file, $3 - module define
AC_DEFUN([HTOR_CHECK_MODULE], [
AC_MSG_CHECKING([module $1])
AC_COMPILE_IFELSE([[
#define $3
#include "$2"
                   ]], [
AC_MSG_RESULT([yes])                   
AC_DEFINE($3, 1, module $1 ($2))                   
                   ], [
AC_MSG_RESULT([no])                   
])
])
