# Modules load and any other needed command
uenv start /bret/scratch/cscs/bcumming/images/prgenv-gnu-24.2-nccl.squashfs
uenv view default
source /bret/scratch/cscs/lfusco/spack/share/spack/setup-env.sh
spack load python
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/user-environment/env/default/lib:/user-environment/env/default/lib64:/user-environment/linux-sles15-neoverse_v2/gcc-12.3.0/cray-gtl-8.1.28-zrpwudcmtmevtnjgtbug7ykbdyxkqmrn/lib
export MPICH_SMP_SINGLE_COPY_MODE=CMA
export MPICH_MALLOC_FALLBACK=1
export NCCL_IGNORE_CPU_AFFINITY=1
# Mandatory variables to compile/run microbenchmarks
export BLINK_CC=mpicc # MPI C Compiler
export BLINK_CXX=mpicxx # MPI C++ Compiler
export BLINK_GPU_BENCH="true" # Shall we run GPU interconnect tests?
export BLINK_XCCL_BENCH="false" # Shall we run xCCL tests?
export BLINK_NG_BENCH="false" # Shall we run Netgauge tests?
export BLINK_CUDA_HOME="/user-environment/env/default"
export BLINK_NCCL_HOME="/user-environment/env/default"
export BLINK_MPI_HOME="/user-environment/env/default" # MPI home folder
export BLINK_MPI_CUDA_HOME="/user-environment/env/default" # MPI CUDA-aware home folder
export BLINK_MPIRUN="srun" # Command for running MPI applications
export BLINK_MPIRUN_MAP_BY_NODE_FLAG="" # Flag to force ranks to be mapped by node (srun)
export BLINK_MPIRUN_HOSTFILE_FLAG="" # Flag for specifying the hostfile
export BLINK_MPIRUN_HOSTFILE_LONG_FLAG="" # Flag for specifying the hostfile (16 hosts)
export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu=1,73,145,217" #"--cpu-bind=socket" # Pinning flags
export BLINK_MPIRUN_ADDITIONAL_FLAGS=""    # Any additional flag that must be used by mpirun
export BLINK_INTERFACE_MASK="148.187.36.181/19" # Interface address + mask size of the two nodes
export BLINK_RUN_IB=false # Shall we run IB tests?
export BLINK_RUN_IBV=false # Shall we run IBV tests?
export BLINK_NET_NOISE_CONC=1 # How many concurrent connections to run to measure bandwidth network noise
export BLINK_BW_SATURATING_SIZE=16777216 # How many bytes to send to get max bw
export BLINK_WL_MANAGER="slurm"
export BLINK_NG_CONFIGURE_FLAGS="HRT_ARCH=6 --with-mpi=/user-environment/env/default"
export UCX_LOG_LEVEL=error
#export BLINK_IB_DEVICES="mlx5_0#mlx5_1#mlx5_2#mlx5_3"

# Optional variables specifying binary paths for applications
export BLINK_AMG_PATH=""
export BLINK_G500_PATH=""
export BLINK_MINIFE_PATH=""