AC_DEFUN([NG_WITH_ARMCI],
    [AC_ARG_WITH(armci, AC_HELP_STRING([--with-armci], [compile with ARMCI support (ARG can be the path to the root MPI directory)]))
    armci_found=no
    if test x"${with_armci}" = xyes; then
        AC_CHECK_HEADER(armci.h, armci_found=yes, [AC_MSG_ERROR([ARMCI selected but not available!])])
    elif test x"${with_armci}" != x; then
        AC_CHECK_HEADER(${with_armci}/include/armci.h, [ng_armci_path=${with_armci}; armci_found=yes], [AC_MSG_ERROR([Can't find ARMCI in ${with_armci}])])
    else
        AC_CHECK_HEADER(armci.h, armci_found=yes, AC_MSG_NOTICE([ARMCI support disabled]))
    fi
    if test x"${armci_found}" = xyes; then
        AC_DEFINE(NG_ARMCI, 1, enables the ARMCI module)
        AC_MSG_NOTICE([ARMCI support enabled])
        if test x${ng_armci_path} = x; then
            LDFLAGS="${LDFLAGS} -larmci"
        else
            CFLAGS="${CFLAGS} -I${ng_armci_path}/include"
            LIBS="${LIBS} -larmci -L${ng_armci_path}/lib"
        fi
    fi
    ]
)
