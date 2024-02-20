# Modules load and any other needed command
#module load openmpi

# Mandatory variables to compile/run microbenchmarks
export CC=mpicc # MPI Compiler
export BLINK_MPIRUN="mpirun" # Command for running MPI applications
export BLINK_MPIRUN_MAP_BY_NODE_FLAG="--map-by node" # Flag to force ranks to be mapped by node (srun)
export BLINK_MPIRUN_HOSTNAMES_FLAG="-H" # Flag for specifying the hostnames
export BLINK_MPIRUN_PINNING_FLAGS="" # Pinning flags
export BLINK_MPIRUN_ADDITIONAL_FLAGS="" # Any additional flag that must be used by mpirun
export BLINK_INTERFACE_MASK="148.187.36.181/19" # Interface address + mask size of the two nodes
export BLINK_RUN_IB=false # Shall we run IB tests?
export BLINK_RUN_IBV=false # Shall we run IBV tests?
export BLINK_NET_NOISE_CONC=1 # How many concurrent connections to run to measure bandwidth network noise
export BLINK_BW_SATURATING_SIZE=16777216 # How many bytes to send to get max bw
export BLINK_NG_CONFIGURE_FLAGS="HRT_ARCH=6"
export BLINK_WL_MANAGER=mpi

# Optional variables specifying binary paths for applications
export BLINK_AMG_PATH=""
export BLINK_G500_PATH=""
export BLINK_MINIFE_PATH=""