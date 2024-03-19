# Modules load and any other needed command
module load openmpi
module load nvhpc
module load nccl

# Mandatory variables to compile/run microbenchmarks
export BLINK_CC=mpicc # MPI C Compiler
export BLINK_CXX=mpicxx # MPI C++ Compiler
export BLINK_GPU_BENCH="true" # Shall we run GPU interconnect tests?
export BLINK_XCCL_BENCH="true"
export BLINK_CUDA_HOME="/opt/cuda/11.8.0/targets/x86_64-linux"
export BLINK_NCCL_HOME="/home/root/opt/nvhpc/Linux_x86_64/23.1/comm_libs/11.8/nccl"
export BLINK_MPI_HOME="/leonardo/prod/spack/03/install/0.19/linux-rhel8-icelake/nvhpc-23.1/openmpi-4.1.4-6ek2oqarjw755glr5papxirjmamqwvgd" # MPI home folder
export BLINK_MPI_CUDA_HOME="/leonardo/prod/spack/03/install/0.19/linux-rhel8-icelake/nvhpc-23.1/openmpi-4.1.4-6ek2oqarjw755glr5papxirjmamqwvgd" # MPI CUDA-aware home folder
export BLINK_MPIRUN="srun" # Command for running MPI applications
export BLINK_MPIRUN_MAP_BY_NODE_FLAG="" # Flag to force ranks to be mapped by node (srun)
export BLINK_MPIRUN_HOSTFILE_FLAG="" # Flag for specifying the hostfile
export BLINK_MPIRUN_HOSTFILE_LONG_FLAG="" # Flag for specifying the hostfile (16 hosts)
export BLINK_PINNING_FLAGS="--cpu-bind=socket" # Pinning flags
export BLINK_MPIRUN_ADDITIONAL_FLAGS=""    # Any additional flag that must be used by mpirun
export BLINK_INTERFACE_MASK="148.187.36.181/19" # Interface address + mask size of the two nodes
export BLINK_RUN_IB=false # Shall we run IB tests?
export BLINK_RUN_IBV=false # Shall we run IBV tests?
export BLINK_NET_NOISE_CONC=1 # How many concurrent connections to run to measure bandwidth network noise
export BLINK_BW_SATURATING_SIZE=16777216 # How many bytes to send to get max bw
export BLINK_WL_MANAGER="slurm"
export BLINK_NG_CONFIGURE_FLAGS="HRT_ARCH=6 --with-mpi=/leonardo/prod/spack/03/install/0.19/linux-rhel8-icelake/nvhpc-23.1/openmpi-4.1.4-6ek2oqarjw755glr5papxirjmamqwvgd/"
export UCX_LOG_LEVEL=error
export BLINK_IB_DEVICES="mlx5_0#mlx5_1#mlx5_2#mlx5_3"

# Optional variables specifying binary paths for applications
export BLINK_AMG_PATH=""
export BLINK_G500_PATH=""
export BLINK_MINIFE_PATH=""
if [ -n "${NCCL_IB_SL}" ]; then
    export NCCL_IB_SL=${NCCL_IB_SL}
else
    export NCCL_IB_SL=1
fi
if [ -n "${UCX_IB_SL}" ]; then 
    export UCX_IB_SL=${UCX_IB_SL}
else
    export UCX_IB_SL=1
fi

