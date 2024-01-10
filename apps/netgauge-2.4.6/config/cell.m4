AC_DEFUN([NG_HAS_PPU_INTRINSICS],
    [AC_REQUIRE([AC_PROG_CC])
    AC_LANG_PUSH([C])
    #AC_CACHE_CHECK([for ppu intrinsics], [NG_PPU_INTRINSICS],
    #[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <ppu_intrinsics.h>],[__mftb(); return 0;])],
    #    [NG_PPU_INTRINSICS=yes], [NG_PPU_INTRINSICS=no])])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <ppu_intrinsics.h>],[__mftb(); return 0;])],
        [NG_PPU_INTRINSICS=yes], [NG_PPU_INTRINSICS=no])
    AC_LANG_POP([C])

    if test x"$NG_PPU_INTRINSICS" = "xyes"; then
        AC_DEFINE(NG_PPU_INTRINSICS, yes, [Define if compiler understands ppu intrinics])
    fi
    ]
)


AC_DEFUN([NG_WITH_CELL],
    [
    NG_CELL="no"
    AC_ARG_WITH(cell,
        [  --with-cell[[=ARG]]       compile with CELL support (ARG can be the path to the root Cell directory, if spu-cc is not in PATH) [[ARG=yes]]],
        [

        NG_HAS_PPU_INTRINSICS
        if test x"${withval-yes}" != xyes; then
            AC_CHECK_PROG(ng_spucc_found, spu-cc, yes, no, ${withval}/bin)
            AC_CHECK_PROG(ng_embedspu_found, ppu-embedspu, yes, no, ${withval}/bin)
            AC_CHECK_PROG(ng_ar_found, ppu-ar, yes, no, ${withval}/bin)
            ng_spucc_path=${withval}/bin/
        else
            AC_CHECK_PROG(ng_spucc_found, spu-cc, yes, no)
            AC_CHECK_PROG(ng_embedspu_found, ppu-embedspu, yes, no)
            AC_CHECK_PROG(ng_ar_found, ppu-ar, yes, no)
            ng_spucc_path=
        fi

        if test x${ng_spucc_found} = xno; then
            AC_MSG_ERROR(${ng_spucc_path}spu-cc not found)
        else
            SPUCC=${ng_spucc_path}spu-cc
            AC_SUBST(SPUCC)
            AC_SUBST(SPUCFLAGS)
        fi

        if test x${ng_embedspu_found} = xno; then
            AC_MSG_ERROR(${ng_spucc_path}ppu-embedspu not found)
        else
            EMBEDSPU=${ng_spucc_path}ppu-embedspu
            AC_SUBST(EMBEDSPU)
        fi

        if test x${ng_ar_found} = xno; then
            AC_MSG_ERROR(${ng_spucc_path}ppu-ar not found)
        else
            ARSPU=${ng_spucc_path}ppu-ar
            AC_SUBST(ARSPU)
        fi

        AC_CHECK_LIB(spe2, spe_gang_context_create, [], [
            AC_MSG_ERROR(Valid libspe2 not found)
            ])

        if test x${NG_PPU_INTRINSICS} = xno; then
            if test x${ng_mpicc_found} = xyes; then
                AC_MSG_ERROR(Your MPI configuration is incompatible to the cell sdk. Try to set the CC that our MPI implementation uses to ppu32-gcc.)
            else
                AC_MSG_ERROR(No CC was found which understand the Cell PPU intrinsics. Try to set your CC environment variable to ppu32-gcc and run configure again: CC=ppu32-gcc ./configure --with-cell)
            fi
        else
            AC_DEFINE(NG_CELL, 1, enables the CELL specific code)
            NG_CELL="yes"
        fi

        #unset ng_spucc_path ng_spucc_found
    ]
    )
    AM_CONDITIONAL(NG_CELL, test x"$NG_CELL" == x"yes")
   ]
)
