AC_DEFUN([NG_WITH_OFED],
    [AC_ARG_WITH(ofed, AC_HELP_STRING([--with-ofed], [compile with Open Fabrics support (ARG can be the path to the root Open Fabrics directory)]))
    ofed_found=no
    AC_MSG_NOTICE([with_ofed = ${with_ofed}])
    if test x"${with_ofed}" == xno; then
      AC_MSG_NOTICE([with_ofed = ${with_ofed}])
    else
    if test x"${with_ofed}" = xyes; then
        AC_CHECK_HEADER(infiniband/verbs.h, ofed_found=yes, [AC_MSG_ERROR([OFED selected but not available!])])
    elif test x"${with_ofed}" != x; then
        AC_CHECK_HEADER(${with_ofed}/include/infiniband/verbs.h, [ng_ofed_path=${with_ofed}; ofed_found=yes], [AC_MSG_ERROR([Can't find OFED in ${with_ofed}])])
    else
        AC_CHECK_HEADER(infiniband/verbs.h, ofed_found=yes, AC_MSG_NOTICE([OFED support disabled]))
    fi
    fi
    if test x"${ofed_found}" = xyes; then
        AC_DEFINE(NG_OFED, 1, enables the ofed module)
        AC_MSG_NOTICE([OFED support enabled])
        if test x${ng_ofed_path} = x; then
            LDFLAGS="${LDFLAGS} -libverbs"
        else
            CFLAGS="${CFLAGS} -I${ng_ofed_path}/include"
            LIBS="${LIBS} -libverbs -L${ng_ofed_path}/lib"
        fi
    fi
    ]
)
