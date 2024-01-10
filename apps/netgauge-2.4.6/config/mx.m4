AC_DEFUN([NG_WITH_MX],
    [AC_ARG_WITH(mx, AC_HELP_STRING([--with-mx], [compile with Myrinet/MX or OpenMX support (ARG can be the path to the root Open Fabrics directory)]))
    mx_found=no
    if test x"${with_mx}" = xyes; then
        AC_CHECK_HEADER(myriexpress.h, mx_found=yes, [AC_MSG_ERROR([MX support selected but headers not available!])])
    elif test x"${with_mx}" != x; then
        AC_CHECK_HEADER(${with_mx}/include/myriexpress.h, [ng_mx_path=${with_mx}; mx_found=yes], [AC_MSG_ERROR([Can't find the MX header files in ${with_mx}/include/])])
    else
        AC_CHECK_HEADER(myriexpress.h, mx_found=yes, [AC_MSG_NOTICE([Myrinet/MX support disabled])])
    fi
    if test x"${mx_found}" = xyes; then
        AC_DEFINE(NG_MX, 1, enables the mx module)
        AC_MSG_NOTICE([MX support enabled])
        if test x${ng_mx_path} = x; then
            LDFLAGS="${LDFLAGS} -lm"
        else
            CFLAGS="${CFLAGS} -I${ng_mx_path}/include"
            LDFLAGS="${LDFLAGS} -lmyriexpress -L${ng_mx_path}/lib -L${ng_mx_path}/lib64"
        fi
    fi
    ]
)
