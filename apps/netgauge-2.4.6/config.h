/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* cpu affinity */
#define HAVE_CPUAFFINITY 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the `gethostbyname' function. */
#define HAVE_GETHOSTBYNAME 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `inet_ntoa' function. */
#define HAVE_INET_NTOA 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `c' library (-lc). */
/* #undef HAVE_LIBC */

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `papi' library (-lpapi). */
/* #undef HAVE_LIBPAPI */

/* Define to 1 if you have the `spe2' library (-lspe2). */
/* #undef HAVE_LIBSPE2 */

/* Define to 1 if you have the <linux/if_packet.h> header file. */
#define HAVE_LINUX_IF_PACKET_H 1

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <memory.h> header file. */
/* #undef HAVE_MEMORY_H */

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/ether.h> header file. */
#define HAVE_NETINET_ETHER_H 1

/* Define to 1 if you have the <netinet/if_ether.h> header file. */
#define HAVE_NETINET_IF_ETHER_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <net/ethernet.h> header file. */
#define HAVE_NET_ETHERNET_H 1

/* Define to 1 if you have the <papi.h> header file. */
/* #undef HAVE_PAPI_H */

/* Define to 1 if you have the `pow' function. */
#define HAVE_POW 1

/* Define to 1 if your system has a GNU libc compatible `realloc' function,
   and to 0 otherwise. */
#define HAVE_REALLOC 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the `sysctl' function. */
/* #undef HAVE_SYSCTL */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* highrestimer architecture */
#define HRT_ARCH 2

/* highrestimer resolution (ticks/sec) */
#define HRT_RESOLUTION 2918839157

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* enables the ARMCI module */
/* #undef NG_ARMCI */

/* enables the CELL specific code */
/* #undef NG_CELL */

/* enables the Myrinet/GM specific code */
/* #undef NG_GM */

/* module ARMCI (mod_armci.c) */
#define NG_MOD_ARMCI 1

/* module CELL (mod_cell.c) */
#define NG_MOD_CELL 1

/* module ENET-EDP (mod_enet_edp.c) */
#define NG_MOD_ENET_EDP 1

/* module ENET-ESP (mod_enet_esp.c) */
#define NG_MOD_ENET_ESP 1

/* module ETH (mod_eth.c) */
#define NG_MOD_ETH 1

/* module GM (mod_gm.c) */
#define NG_MOD_GM 1

/* module IB (mod_ib.c) */
#define NG_MOD_IB 1

/* module IBV (mod_ibv.c) */
#define NG_MOD_IBV 1

/* module LIBOF (mod_libof.c) */
/* #undef NG_MOD_LIBOF */

/* module MPI (mod_mpi.c) */
#define NG_MOD_MPI 1

/* module MX (mod_mx.c) */
#define NG_MOD_MX 1

/* module SCI (mod_sci.c) */
#define NG_MOD_SCI 1

/* module TCP (mod_tcp.c) */
#define NG_MOD_TCP 1

/* module UDP (mod_udp.c) */
#define NG_MOD_UDP 1

/* enables the MPI specific code */
#define NG_MPI 1

/* enables the mx module */
/* #undef NG_MX */

/* enables the ofed module */
#define NG_OFED 1

/* Define if compiler understands ppu intrinics */
/* #undef NG_PPU_INTRINSICS */

/* pattern 1toN (ptrn_1toN.cpp) */
#define NG_PTRN_1TON 1

/* pattern beff (ptrn_beff.cpp) */
#define NG_PTRN_BEFF 1

/* pattern collvsnoise (ptrn_collvsnoise.cpp) */
#define NG_PTRN_COLLVSNOISE 1

/* pattern cpu (ptrn_cpu.cpp) */
#define NG_PTRN_CPU 1

/* pattern disk (ptrn_disk.cpp) */
#define NG_PTRN_DISK 1

/* pattern distrtt (ptrn_distrtt.c) */
#define NG_PTRN_DISTRTT 1

/* pattern ebb (ptrn_ebb.cpp) */
#define NG_PTRN_EBB 1

/* pattern func_args (ptrn_func_args.cpp) */
#define NG_PTRN_FUNC_ARGS 1

/* pattern loggp (ptrn_loggp.c) */
#define NG_PTRN_LOGGP 1

/* pattern memory (ptrn_memory.cpp) */
#define NG_PTRN_MEMORY 1

/* pattern mprobe (ptrn_mprobe.cpp) */
#define NG_PTRN_MPROBE 1

/* pattern nbov (ptrn_nbov.c) */
#define NG_PTRN_NBOV 1

/* pattern noise (ptrn_noise.c) */
#define NG_PTRN_NOISE 1

/* pattern Nto1 (ptrn_Nto1.cpp) */
#define NG_PTRN_NTO1 1

/* pattern one_one (ptrn_one_one.cpp) */
#define NG_PTRN_ONE_ONE 1

/* pattern one_one_all (ptrn_one_one_all.cpp) */
#define NG_PTRN_ONE_ONE_ALL 1

/* pattern one_one_dtype (ptrn_one_one_dtype.cpp) */
#define NG_PTRN_ONE_ONE_DTYPE 1

/* pattern one_one_mpi_bidirect (ptrn_one_one_mpi_bidirect.cpp) */
#define NG_PTRN_ONE_ONE_MPI_BIDIRECT 1

/* pattern one_one_perturb (ptrn_one_one_perturb.cpp) */
#define NG_PTRN_ONE_ONE_PERTURB 1

/* pattern one_one_randbuf (ptrn_one_one_randbuf.cpp) */
#define NG_PTRN_ONE_ONE_RANDBUF 1

/* pattern one_one_randtag (ptrn_one_one_randtag.cpp) */
#define NG_PTRN_ONE_ONE_RANDTAG 1

/* pattern one_one_req_queue (ptrn_one_one_req_queue.cpp) */
#define NG_PTRN_ONE_ONE_REQ_QUEUE 1

/* pattern one_one_sync (ptrn_one_one_sync.cpp) */
#define NG_PTRN_ONE_ONE_SYNC 1

/* pattern overlap (ptrn_overlap.c) */
#define NG_PTRN_OVERLAP 1

/* pattern synctest (ptrn_synctest.cpp) */
#define NG_PTRN_SYNCTEST 1

/* enables the sisci module */
/* #undef NG_SISCI */

/* enables the VAPI specific code */
/* #undef NG_VAPI */

/* Name of package */
#define PACKAGE "netgauge"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "htor@cs.indiana.edu"

/* Define to the full name of this package. */
#define PACKAGE_NAME "netgauge"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "netgauge 2.4.6"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "netgauge"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.4.6"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Version number of package */
#define VERSION "2.4.6"

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */
