#!/bin/bash
#SBATCH --nodes=1
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=8
#SBATCH --gpus-per-node=8
#SBATCH -p ju-standard-g
#SBATCH -A project_465000997
#SBATCH --exclusive
#SBATCH --job-name=blink-intra-node
source conf.sh

# Various env variables
export NCCL_NET_GDR_LEVEL=3
export FI_CXI_ATS=0
export NCCL_BUFFSIZE=33554432

# List of core ID for each GPU (from 0 to 7)
CPUS_PER_GPU=("49" "57" "17" "25" "1" "9" "33" "41")

# Assuming the first GPU is 0, loop on the other
SRC_GPU=0
for DST_GPU in {1..7}
do
    EXTRA="${SRC_GPU}-${DST_GPU}"
    # GPU mapping and NUMA nodes mapping
    export USER_HIP_GPU_MAP="${SRC_GPU},${DST_GPU}"
    export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu:${CPUS_PER_GPU[$SRC_GPU]},${CPUS_PER_GPU[$DST_GPU]}" # Pinning flags

    ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 10 -e ${EXTRA}"
    # Baseline
    source src/microbench-gpu/moduleload/load_Baseline_modules.sh
    ./run.sh test_suites/gpubench_pp_baseline_1GiB ${ARGS} --ppn 2

    # NCCL
    source src/microbench-gpu/moduleload/load_Nccl_new_modules.sh
    ./run.sh test_suites/gpubench_pp_nccl_1GiB ${ARGS} --ppn 2

    # NVLink
    source src/microbench-gpu/moduleload/load_Nvlink_modules.sh
    ./run.sh test_suites/gpubench_pp_nvlink_1GiB ${ARGS} --ppn 2

    # CUDA-Aware MPI
    source src/microbench-gpu/moduleload/load_CudaAware_modules.sh
    export MPICH_GPU_SUPPORT_ENABLED=1
    ./run.sh test_suites/gpubench_pp_ca_1GiB ${ARGS} --ppn 2
    unset MPICH_GPU_SUPPORT_ENABLED
done
