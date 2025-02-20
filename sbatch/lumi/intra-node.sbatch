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

# GPU mapping and NUMA nodes mapping
export USER_HIP_GPU_MAP="0,1,2,3,4,5,6,7"
export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu:49,57,17,25,1,9,33,41" # Pinning flags
EXTRA="0-1"
export MPICH_GPU_ALLREDUCE_BLK_SIZE=134217728


ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 10 -e ${EXTRA}"
# Baseline
source src/microbench-gpu/moduleload/load_Baseline_modules.sh
./run.sh test_suites/gpubench_pp_baseline ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_baseline ${ARGS} --ppn 8

# NCCL
source src/microbench-gpu/moduleload/load_Nccl_new_modules.sh
./run.sh test_suites/gpubench_pp_nccl ${ARGS} --ppn 2
source src/microbench-gpu/moduleload/load_Nccl_modules.sh
./run.sh test_suites/gpubench_coll_nccl ${ARGS} --ppn 8
#./run.sh test_suites/comm-nccl ${ARGS} --ppn 8

# NVLink
source src/microbench-gpu/moduleload/load_Nvlink_modules.sh
./run.sh test_suites/gpubench_pp_nvlink ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_nvlink ${ARGS} --ppn 8

# CUDA-Aware MPI
source src/microbench-gpu/moduleload/load_CudaAware_modules.sh
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/gpubench_pp_ca ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_ca ${ARGS} --ppn 8
#./run.sh test_suites/comm-cudaaware ${ARGS} --ppn 8
unset MPICH_GPU_SUPPORT_ENABLED
