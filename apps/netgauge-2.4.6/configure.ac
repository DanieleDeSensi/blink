#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(netgauge, [2.4.6], [htor@cs.indiana.edu])
AC_CONFIG_AUX_DIR(config)
#AC_CANONICAL_SYSTEM
#AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(netgauge, 2.4.6)
#AC_CONFIG_SRCDIR([netgauge.c])
AC_CONFIG_SRCDIR([af_enet.h])
AM_CONFIG_HEADER([config.h])
echo "" > config.h

# check for "--with-mpi"
NG_WITH_MPICC
NG_WITH_MPICXX

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([c], [sysctl])
AC_CHECK_LIB([papi], [PAPI_create_eventset])
LDFLAGS="${LDFLAGS} -L/usr/local/lib64"

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h sys/ioctl.h sys/socket.h sys/time.h unistd.h papi.h])
# we don't have malloc.h on mac os but we should use stdlib.h instead
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([getopt.h])
AC_CHECK_HEADERS([stdlib.h])
#we do not have netinet/ether.h on mac os but we can use the ones listed below
AC_CHECK_HEADERS(netinet/ether.h)
AC_CHECK_HEADERS(sys/types.h sys/socket.h net/ethernet.h netinet/if_ether.h)
AC_CHECK_HEADERS(linux/if_packet.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gethostbyname gettimeofday inet_ntoa memset pow socket sqrt strchr strstr sysctl])

HTOR_CHECK_CPUAFFINITY

AC_MSG_NOTICE([**** checking for available high precision timers ****])
#AC_MSG_CHECKING([high resolution timer architecture])

if test "x$HRT_ARCH" = "x"; then
# not enforced on command line

hr_subdir="hrtimer"
HTOR_TIMERARCH(x86_64,2, $hr_subdir)
if test "x$hrt_arch" = "x"; then # if not found yet 
HTOR_TIMERARCH(x86_32,1,$hr_subdir)
fi
if test "x$hrt_arch" = "x"; then # if not found yet
HTOR_TIMERARCH(ppc,3, $hr_subdir)
fi
if test "x$hrt_arch" = "x"; then # if not found yet
HTOR_TIMERARCH(ia64,4, $hr_subdir)
fi
if test "x$hrt_arch" = "x"; then # if not found yet
HTOR_TIMERARCH(SiCortex,5, $hr_subdir)
fi
if test "x$hrt_arch" = "x"; then # if not found yet
HTOR_TIMERARCH(wtime,6, $hr_subdir)
fi
#AC_MSG_RESULT([$hrt_arch])

if test "x$hrt_arch" != "x"; then
######################################################
# getting ticks guess ... should be portable
printf "compiling tick-counter benchmark ... "
$CC $CFLAGS -DHRT_ARCH=$hrt_arch_num $hr_subdir/getres.c -o getres
if test "$?" != "0"; then exit 1; fi;
printf "done\n" 
if test "$hrt_arch" != "wtime"; then
  printf "getting clock resolution (may take some seconds)"
  this_ticks=$(./getres)
  if test "$?" != "0"; then exit 1; fi;
  printf " $this_ticks ticks/sec\n"
  rm -f getres

  AC_DEFINE_UNQUOTED([HRT_RESOLUTION], $this_ticks, [highrestimer resolution
             (ticks/sec)])
fi
else
  printf "*** warning - no high resolution timer found! ***\n"
fi
else
  hrt_arch=$HRT_ARCH;
  AC_DEFINE_UNQUOTED([HRT_ARCH], [${HRT_ARCH}], [highrestimer architecture])
  printf "*** warning - HRT_RESOLUTION not set because timer was forced to $hrt_arch!\n"
fi


######################################################
# low level networking ...
AC_MSG_NOTICE([**** checking for various network APIs ****])

# check for "--with-armci"
NG_WITH_ARMCI

# check for "-with-vapi"
NG_WITH_VAPI

# check for "-with-ofed"
NG_WITH_OFED

# check for "-with-gm"
NG_WITH_GM

# check for "-with-sisci"
NG_WITH_SISCI

# check for "--with-cell"
NG_WITH_CELL

# check for "--with-mx"
NG_WITH_MX


AC_MSG_NOTICE([**** checking if Netgauge modules compile ****])
HTOR_CHECK_MODULE(TCP,mod_tcp.c,NG_MOD_TCP)
HTOR_CHECK_MODULE(UDP,mod_udp.c,NG_MOD_UDP)
HTOR_CHECK_MODULE(MPI,mod_mpi.c,NG_MOD_MPI)
HTOR_CHECK_MODULE(ENET-EDP,mod_enet_edp.c,NG_MOD_ENET_EDP)
HTOR_CHECK_MODULE(ENET-ESP,mod_enet_esp.c,NG_MOD_ENET_ESP)
HTOR_CHECK_MODULE(ETH,mod_eth.c,NG_MOD_ETH)
HTOR_CHECK_MODULE(ARMCI,mod_armci.c,NG_MOD_ARMCI)
HTOR_CHECK_MODULE(LIBOF,mod_libof.c,NG_MOD_LIBOF)
HTOR_CHECK_MODULE(IB,mod_ib.c,NG_MOD_IB)
HTOR_CHECK_MODULE(IBV,mod_ibv.c,NG_MOD_IBV)
HTOR_CHECK_MODULE(GM,mod_gm.c,NG_MOD_GM)
HTOR_CHECK_MODULE(SCI,mod_sci.c,NG_MOD_SCI)
HTOR_CHECK_MODULE(CELL,mod_cell.c,NG_MOD_CELL)
HTOR_CHECK_MODULE(MX,mod_mx.c,NG_MOD_MX)

AC_MSG_NOTICE([**** checking if Netgauge patterns compile ****])
HTOR_CHECK_PATTERN(distrtt,ptrn_distrtt.c,NG_PTRN_DISTRTT)
HTOR_CHECK_PATTERN(nbov,ptrn_nbov.c,NG_PTRN_NBOV)
HTOR_CHECK_PATTERN(loggp,ptrn_loggp.c,NG_PTRN_LOGGP)
HTOR_CHECK_PATTERN(noise,ptrn_noise.c,NG_PTRN_NOISE)
HTOR_CHECK_PATTERN(overlap,ptrn_overlap.c,NG_PTRN_OVERLAP)
AC_LANG_PUSH(C++)
HTOR_CHECK_PATTERN(one_one,ptrn_one_one.cpp,NG_PTRN_ONE_ONE)
HTOR_CHECK_PATTERN(one_one_all,ptrn_one_one_all.cpp,NG_PTRN_ONE_ONE_ALL)
HTOR_CHECK_PATTERN(one_one_mpi_bidirect,ptrn_one_one_mpi_bidirect.cpp,NG_PTRN_ONE_ONE_MPI_BIDIRECT)
HTOR_CHECK_PATTERN(one_one_sync,ptrn_one_one_sync.cpp,NG_PTRN_ONE_ONE_SYNC)
HTOR_CHECK_PATTERN(one_one_perturb,ptrn_one_one_perturb.cpp,NG_PTRN_ONE_ONE_PERTURB)
HTOR_CHECK_PATTERN(one_one_req_queue,ptrn_one_one_req_queue.cpp,NG_PTRN_ONE_ONE_REQ_QUEUE)
HTOR_CHECK_PATTERN(one_one_dtype,ptrn_one_one_dtype.cpp,NG_PTRN_ONE_ONE_DTYPE)
HTOR_CHECK_PATTERN(one_one_randtag,ptrn_one_one_randtag.cpp,NG_PTRN_ONE_ONE_RANDTAG)
HTOR_CHECK_PATTERN(one_one_randbuf,ptrn_one_one_randbuf.cpp,NG_PTRN_ONE_ONE_RANDBUF)
HTOR_CHECK_PATTERN(Nto1,ptrn_Nto1.cpp,NG_PTRN_NTO1)
HTOR_CHECK_PATTERN(1toN,ptrn_1toN.cpp,NG_PTRN_1TON)
HTOR_CHECK_PATTERN(synctest,ptrn_synctest.cpp,NG_PTRN_SYNCTEST)
HTOR_CHECK_PATTERN(collvsnoise,ptrn_collvsnoise.cpp,NG_PTRN_COLLVSNOISE)
HTOR_CHECK_PATTERN(beff,ptrn_beff.cpp,NG_PTRN_BEFF)
HTOR_CHECK_PATTERN(mprobe,ptrn_mprobe.cpp,NG_PTRN_MPROBE)
HTOR_CHECK_PATTERN(memory,ptrn_memory.cpp,NG_PTRN_MEMORY)
HTOR_CHECK_PATTERN(disk,ptrn_disk.cpp,NG_PTRN_DISK)
HTOR_CHECK_PATTERN(ebb,ptrn_ebb.cpp,NG_PTRN_EBB)
HTOR_CHECK_PATTERN(func_args,ptrn_func_args.cpp,NG_PTRN_FUNC_ARGS)
HTOR_CHECK_PATTERN(cpu,ptrn_cpu.cpp,NG_PTRN_CPU)
AC_LANG_POP()


AC_CONFIG_FILES([Makefile wnlib/Makefile])
AC_OUTPUT
