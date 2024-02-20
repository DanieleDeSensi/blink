AC_DEFUN([NG_WITH_SISCI],
    [AC_ARG_WITH(sisci, AC_HELP_STRING([--with-sisci], [compile with SISCI support (ARG can be the path to the root SISCI directory)]))
    sisci_found=no
    if test x"${with_sisci}" = xyes; then
        AC_CHECK_HEADER(sisci_api.h, sisci_found=yes, [AC_MSG_ERROR([SISCI selected but not available!])])
    elif test x"${with_sisci}" != x; then
        AC_CHECK_HEADER(${with_sisci}/include/sisci_api.h, [ng_sisci_path=${with_sisci}; sisci_found=yes], [AC_MSG_ERROR([Can't find sisci in ${with_sisci}])])
    else
        AC_CHECK_HEADER(sisci_api.h, sisci_found=yes, AC_MSG_NOTICE([SISCI support disabled]))
    fi
    if test x"${sisci_found}" = xyes; then
        AC_DEFINE(NG_SISCI, 1, enables the sisci module)
        AC_MSG_NOTICE([SISCI support enabled])
        if test x${ng_sisci_path} = x; then
            LDFLAGS="${LDFLAGS} -lsisci"
        else
            CFLAGS="${CFLAGS} -I${ng_sisci_path}/include"
            LIBS="${LIBS} -lsisci -L${ng_sisci_path}/lib"
        fi
    fi
    ]
)
