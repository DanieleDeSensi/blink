CC=mpicc # MPI Compiler
MPIRUN="mpirun" # Command for running MPI applications
MPIRUN_MAP_BY_NODE_FLAG="--map-by node" # Flag to force ranks to be mapped by node (srun)
MPIRUN_HOSTNAMES_FLAG="-H" # Flag for specifying the hostnames
MPIRUN_PINNING_FLAGS="--cpu-bind=map_cpu=2,2" # Pinning flags
MPIRUN_ADDITIONAL_FLAGS="" # Any additional flag that must be used by mpirun
INTERFACE_MASK="148.187.36.181/19" # Interface address + mask size of the two nodes
RUN_IB=false # Shall we run IB tests?
RUN_IBV=false # Shall we run IBV tests?
NET_NOISE_CONC=1 # How many concurrent connections to run to measure bandwidth network noise
BW_SATURATING_SIZE=16777216 # How many bytes to send to get max bw
NG_CONFIGURE_FLAGS="HRT_ARCH=6"
