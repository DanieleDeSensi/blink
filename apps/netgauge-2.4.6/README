# README file for netgauge benchmark suite

Installing
----------

 * run "bootstrap.sh" after checking out from Subversion to generate
   the configure script

 * the installation itself boils down to the usual UNIX admin battle :)
   ./configure
    make
    and if desired: make install

 * the configure script supports (in addition to the standard parameters)
   the following options:

   --with-mpi[=ARG] - compile with MPI support (i.e. compile with mpicc),
      the argument ARG can be the root path to the MPI installation if,
      for instance, mpicc is not in the PATH environment variable;

      only give the root path: if, for example, your mpicc resides in
      /opt/openmpi/bin/ use "--with-mpi=/opt/openmpi"

   --with-vapi[=ARG] - compile with Infiniband VAPI support (the ib1
      module needs this and MPI suport, too),
      the argument ARG can be the root path of your VAPI installation
      if the environment variable MTHOME is not (or incorrectly) defined;

      only give the root path: if, for example, your VAPI libraries
      reside in /usr/local/infiniband/vapi/lib/ use
      "--with-vapi=/usr/local/infiniband/vapi"

   --with-openib[=ARG] - compile with OpenIB Infiniband Verbs support (the ib1
      module needs this and MPI suport, too),
      the argument ARG can be the root path of your OpenIB installation

      only give the root path: if, for example, your OpenIB libraries
      reside in /usr/local/infiniband/openib/lib/ use
      "--with-openib=/usr/local/infiniband/openib"

   --with-gm[=ARG] - compile with Myrinet (GM) support
      the gm module needs this and MPI suport, too
      the argument ARG can be the root path of your GM installation

   --with-cell[=ARG] - compile with Cell BE support,
      the argument ARG can be the root path to the Cell SDK installation if,
      for instance, spu-cc is not in the PATH environment variable;

      only give the root path: if, for example, your spu-cc resides in
      /opt/cell/bin/ use "--with-cell=/opt/cell/"

      Be sure that your C compiler understands ppu specific intrinsics. If
      not, try to compile it with another compiler like ppu-cc using
      "CC=ppu32-gcc ./configure --with-cell --without-mpi"

   --enable-static - causes "-static" to be added to the LDFLAGS variable;
      if you use this, make sure all needed libraries are also available
      in static form (*.a files)



Running
-------

 * try "netgauge -h" or "netgauge -m <module> -h" for a list of valid
   command line options



Code hints
----------

 * the semantic of the ng_info() function has changed:
   when using MPI (generally, not the MPI module) only the process
   with (MPI_COMM_WORLD) rank 0 prints info messages;

   to get all processes to print a certain info msg. set an additional
   bit in the "vlevel" argument of ng_info():
   this can be achieved with the macro "NG_VPALL"

      use, for instance, "NG_VLEV1 | NG_VPALL" as "vlevel" value to have an
      info msg. printed at verbosity level 1 (parameter -v one time or more)
       _and_ have that message been printed by all started processes

   (there is also a short explanation in netgauge.h where NG_VPALL is defined)

 * Some suggestions from me (Matthias Treydte, "trem")
   ---------------------------------------------------

   I don't play any special role in the development of this tool,
   especially I'm not in the position to specify any rules here. But
   I'm on this for quite a while now and here are some suggestions
   to make life for everyone a little easier:

    * Please us spaces instead of tabs to align the code. This makes
      the code look the same everywhere and a decent editor should
      have an option to automagically convert tabs into spaces. I'd
      suggest 3 spaces for a basic indentation.

    * Make sure you don't use any MPI specific code if MPI is not
      to be used for the current compile. That means putting
      "#if NG_MPI" and "#endif" around the relevant sections.

    * Check if it compiles, at least.

The Module Lifecycle
--------------------

First the module gets it's chance to parse it's supported options via the
module->getopt(...) function.

Next, the module->init(...) function is called. Here the module may check
it's prerequisites (e.g. if a network protocol is aviable) and set it up.
If this function returns non-zero (indicating a problem) the modules' life
ends here (especially the module->shutdown(...) function will not be called).

Otherwise a call to module->setup_channels(...) will instruct the module to
set up channels between all peers. If this function returns non-zero the next
call will be module->shutdown(...), being the last in the modules' lifecycle.

If setting up the channels succeeds the main program will do calls to the
data transmission function in an unpredictable manner, controlled by the
communication pattern currently used.

When the communication pattern is finished the main program will call
module->shutdown(...) to give the module a chance to free all occupied
resources.

The functions getopt(...), init(...), setup_channels(...) and shutdown(...)
are optional. This means that if such a function is not set by the module 
(funtion pointer == 0) no function call will be made and the man program 
will go on as if the function call was made and succeeded.

File Naming Conventions
-----------------------

All implementations of communication MODULES should be called like mod_*.[ch].
All implementations of communication PATTERNS should be called like ptrn_*.[ch].

Citation
--------
Any published work which uses this software should include the following
citation:
----------------------------------------------------------------------
T. Hoefler, T. Mehlan, A. Lumsdaine, and W. Rehm: Netgauge: A Network
Performance Measurement Framework. Proceedings of High Performance
Computing and Communications, HPCC'07, Houston, TX, LNCS volume 4782,
pages 659-671
----------------------------------------------------------------------
and for LogGP parameter measurement:
----------------------------------------------------------------------
T. Hoefler, A. Lichei, and W. Rehm: Low-Overhead LogGP Parameter
Assessment for Modern Interconnection Networks. Proceedings of the 21st
IEEE International Parallel \& Distributed Processing Symposium, PMEO'07
Workshop, IPDPS'07, Long Beach, CA, ISBN 1-4244-0909-8
----------------------------------------------------------------------
