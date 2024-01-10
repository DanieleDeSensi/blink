AC_DEFUN([NG_WITH_GM],
    [AC_ARG_WITH(gm,
        [  --with-gm[[=ARG]]       compile with Myrinet/GM support (ARG can be the path to the root GM directory) [[ARG=yes]]],
        [if test x"${withval-yes}" != xyes; then
            AC_CHECK_HEADER(${withval}/include/gm.h, ng_gm_path=${withval}, [AC_MSG_ERROR([Can't find the Myrinet/GM header files])])
        else
            AC_CHECK_HEADER(gm.h, , [AC_MSG_ERROR([Can't find the Myrinet/GM header files])])
        fi
        AC_DEFINE(NG_GM, 1, enables the Myrinet/GM specific code)
        if test x${ng_gm_path} = x; then
            LDFLAGS="${LDFLAGS} -lgm"
        else
            CFLAGS="${CFLAGS} -I${ng_gm_path}/include"
            LDFLAGS="${LDFLAGS} -lgm -L${ng_gm_path}/lib"
        fi
        ]
    )
   ]
)
