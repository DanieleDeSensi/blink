bin_PROGRAMS = netgauge

#CFLAGS = -O0
#CXXFLAGS = -O0

# hack to enable C++ and C mixture (link with CXX)
CCLD=$(CXX) $(CXXFLAGS)

# hack to enable C++ and C mixture (compile cpp files)
%.o: %.cpp *h hrtimer/*
	$(CXX) $(CXXFLAGS) -c $< -o $@


netgauge_SOURCES = mod_eth.c \
	mod_ib.c mod_ibv.c mod_mpi.c mod_dummy.c netgauge.c fullresult.c \
	cpustat.c statistics.c \
	mod_tcp.c mod_udp.c \
	mod_enet_edp.c mod_enet_esp.c mod_gm.c mod_mx.c \
	ptrn_loggp.c \
	mod_armci.c mod_sci.c ptrn_distrtt.c \
	mod_cell.c mod_cell_dma.c mod_cell_dmalist.c mod_cell_mail.c  \
	mod_libof.c ptrn_nbov.c netgauge_cmdline.c \
	ptrn_noise.c ptrn_noise_cmdline.c ng_sync.c \
	ptrn_collvsnoise_cmdline.c ptrn_mprobe_cmdline.c ptrn_memory_cmdline.c ptrn_disk_cmdline.c ptrn_ebb_cmdline.c \
	getopt_long.c rpl_alloc.c ptrn_overlap.c ptrn_overlap_cmdline.c
	
netgauge_CPPSOURCES = ptrn_one_one.cpp ptrn_one_one_perturb.cpp ptrn_one_one_sync.cpp ptrn_one_one_req_queue.cpp ptrn_one_one_dtype.cpp ptrn_one_one_randtag.cpp ptrn_one_one_randbuf.cpp ptrn_1toN.cpp ptrn_Nto1.cpp ptrn_synctest.cpp ptrn_collvsnoise.cpp ptrn_beff.cpp ptrn_mprobe.cpp librecv_dynsize.cpp ptrn_memory.cpp ptrn_disk.cpp ptrn_ebb.cpp ptrn_func_args.cpp ptrn_func_args_callee.cpp ptrn_one_one_mpi_bidirect.cpp ptrn_cpu.cpp ptrn_one_one_all.cpp 
netgauge_CPPOBJECTS = ptrn_one_one.o ptrn_one_one_perturb.o ptrn_one_one_sync.o ptrn_one_one_req_queue.o ptrn_one_one_dtype.o ptrn_one_one_randtag.o ptrn_one_one_randbuf.o ptrn_1toN.o ptrn_Nto1.o ptrn_synctest.o ptrn_collvsnoise.o ptrn_beff.o ptrn_mprobe.o librecv_dynsize.o ptrn_memory.o ptrn_disk.o ptrn_ebb.o ptrn_func_args.o ptrn_func_args_callee.o ptrn_one_one_mpi_bidirect.o ptrn_cpu.o ptrn_one_one_all.o 
noinst_HEADERS = af_enet.h cpustat.h netgauge.h fullresult.h librecv_dynsize.h \
	statistics.h mod_inet.h mod_enet.h iba.h iba_openib.h getopt.h llscript.ll \
	iba_vapi.h mod_cell.h mod_cell_task.h eth_helpers.h mod_ibv.h mod_ib.h \
	hrtimer/getres.c hrtimer/sanity-check.c hrtimer/calibrate.h hrtimer/ppc-gcc-tb.h hrtimer/x86_32-gcc-rdtsc.h hrtimer/mpi-wtime.h \
	hrtimer/x86_64-gcc-rdtsc.h hrtimer/hrtimer.h ng_tools.hpp mersenne/MersenneTwister.h hrtimer/ia64-gcc-itc.h hrtimer/mips64-sicortex-gcc.h \
	ptrn_noise_cmdline.h ptrn_noise.h netgauge_cmdline.h ptrn_collvsnoise_cmdline.h ng_sync.h ptrn_beff_cmdline.h ptrn_memory_cmdline.h ptrn_disk_cmdline.h \
	ptrn_mprobe_cmdline.h ptrn_overlap_cmdline.h ptrn_ebb_cmdline.h LICENSE ptrn_func_args_callee.h MersenneTwister.h

SUBDIRS = wnlib

EXTRA_DIST = AUTHORS README LICENSE\
	mod_cell_spu.c mod_cell_dma_spu.c \
	$(netgauge_CPPSOURCES)

if NG_CELL
CELL_ADD=mod_cell_spu.a mod_cell_dma_spu.a 

mod_cell_spu: mod_cell_spu.c mod_cell.h mod_cell_task.h
	$(SPUCC) $(SPUCFLAGS) -o $@ $<

mod_cell_dma_spu: mod_cell_dma_spu.c mod_cell.h mod_cell_task.h
	$(SPUCC) $(SPUCFLAGS) -o $@ $<

mod_cell_spu.o: mod_cell_spu
	$(EMBEDSPU) -m32 $< $< $@
	
mod_cell_dma_spu.o: mod_cell_dma_spu
	$(EMBEDSPU) -m32 $< $< $@
	
mod_cell_spu.a: mod_cell_spu.o
	$(ARSPU) -qcs $@ $<
	
mod_cell_dma_spu.a: mod_cell_dma_spu.o
	$(ARSPU) -qcs $@ $<
	
clean-generic:
	rm -f mod_cell_spu mod_cell_dma_spu mod_cell_spu.o \
	mod_cell_dma_spu.o mod_cell_spu.a mod_cell_dma_spu.a
endif

netgauge_LDADD = $(CELL_ADD) wnlib/.libs/libwn.a $(netgauge_CPPOBJECTS) 
