AC_DEFUN([NG_WITH_VAPI],
    [AC_ARG_WITH(vapi,
        [  --with-vapi[[=ARG]]       compile with Infiniband (VAPI) support (ARG can be the path to the root VAPI directory, if MTHOME is not defined (correctly)) [[ARG=yes]]],
        [if test x"${withval-yes}" != xyes; then
            AC_CHECK_PROG(ng_vstat_found, vstat, yes, no, ${withval}/bin)
            ng_vapi_path=${withval}
        else
            AC_CHECK_PROG(ng_vstat_found, vstat, yes, no, ${MTHOME}/bin)
            ng_vapi_path=${MTHOME}
        fi
        if test x${ng_vstat_found} = xno; then
            AC_MSG_ERROR(${ng_vapi_path} is no valid VAPI directory)
        else
            AC_DEFINE(NG_VAPI, 1, enables the VAPI specific code)
            CFLAGS="${CFLAGS} -DIBA_USE_VAPI -I${ng_vapi_path}/include"
            LDFLAGS="${LDFLAGS} -lvapi -L${ng_vapi_path}/lib"
        fi
        #unset ng_vapi_path ng_vstatc_found
        ]
        )
       #AM_CONDITIONAL(WITH_VAPI, test x"${with_vapi-no}" != xno)
   ]
)
