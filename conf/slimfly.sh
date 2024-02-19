MPIRUN="/scratch/2/t2hx/dep/openmpi/bin/mpirun" # Command for running MPI applications
MPIRUN_MAP_BY_NODE_FLAG="--map-by node" # Flag to force ranks to be mapped by node (srun)
MPIRUN_HOSTNAMES_FLAG="-H" # Flag for specifying the hostnames
MPIRUN_PINNING_FLAGS="--cpu-bind=map_cpu=2,2" # Pinning flags
MPIRUN_ADDITIONAL_FLAGS="-mca plm_rsh_no_tree_spawn 1 --map-by node -mca btl openib,self,sm -mca btl_openib_if_include mlx4_0 -mca orte_base_help_aggregate 0" # Any additional flag that must be used by mpirun
INTERFACE_MASK="148.187.36.181/19" # Interface address + mask size of the two nodes
RUN_IB=false # Shall we run IB tests?
RUN_IBV=false # Shall we run IBV tests?
NET_NOISE_CONC=1 # How many concurrent connections to run to measure bandwidth network noise
BW_SATURATING_SIZE=16777216 # How many bytes to send to get max bw
CC=mpicc # MPI Compiler
NG_CONFIGURE_FLAGS="HRT_ARCH=6 --with-mpi=/leonardo/prod/spack/03/install/0.19/linux-rhel8-icelake/nvhpc-23.1/openmpi-4.1.4-6ek2oqarjw755glr5papxirjmamqwvgd/"

#export LD_LIBRAY_PATH=$LD_LIBRARY_PATH:/users/desensi/amd-blis/lib/lp64/

module load openmpi
