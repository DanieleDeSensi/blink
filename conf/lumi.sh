# Modules load and any other needed command
module load cray-python

# Mandatory variables to compile/run microbenchmarks
export BLINK_CC=cc # MPI C Compiler
export BLINK_CXX=CC # MPI C++ Compiler
export BLINK_GPU_BENCH="true" # Shall we run GPU interconnect tests?
export BLINK_XCCL_BENCH="false" # Shall we run xCCL tests?
export BLINK_NG_BENCH="false" # Shall we run Netgauge tests?
export BLINK_CUDA_HOME=""
export BLINK_NCCL_HOME=""
export BLINK_MPI_HOME="" # MPI home folder
export BLINK_MPI_CUDA_HOME="" # MPI CUDA-aware home folder
export BLINK_MPIRUN="srun" # Command for running MPI applications
export BLINK_MPIRUN_MAP_BY_NODE_FLAG="" # Flag to force ranks to be mapped by node (srun)
export BLINK_MPIRUN_HOSTFILE_FLAG="" # Flag for specifying the hostfile
export BLINK_MPIRUN_HOSTFILE_LONG_FLAG="" # Flag for specifying the hostfile (16 hosts)
if [ -n "${BLINK_PINNING_FLAGS}" ]; then
    export BLINK_PINNING_FLAGS=${BLINK_PINNING_FLAGS}
else
    export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu:57,25,41,9,49,17,33,1" # Pinning flags
    #export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu:9,25,41,57,1,17,33,49" # Pinning flags
fi
export BLINK_MPIRUN_ADDITIONAL_FLAGS=""    # Any additional flag that must be used by mpirun
export BLINK_INTERFACE_MASK="148.187.36.181/19" # Interface address + mask size of the two nodes
export BLINK_RUN_IB=false # Shall we run IB tests?
export BLINK_RUN_IBV=false # Shall we run IBV tests?
export BLINK_NET_NOISE_CONC=1 # How many concurrent connections to run to measure bandwidth network noise
export BLINK_BW_SATURATING_SIZE=16777216 # How many bytes to send to get max bw
export BLINK_WL_MANAGER="slurm"
export BLINK_NG_CONFIGURE_FLAGS="HRT_ARCH=6"

# Optional variables specifying binary paths for applications
export BLINK_AMG_PATH=""
export BLINK_G500_PATH=""
export BLINK_MINIFE_PATH=""
