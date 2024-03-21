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
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 10"

# Various env variables
export NCCL_NET_GDR_LEVEL=3
export FI_CXI_ATS=0
export NCCL_BUFFSIZE=33554432
export USER_HIP_GPU_MAP="1,3,7,5,0,2,6,4"

# Baseline
source src/microbench-gpu/moduleload/load_Baseline_modules.sh
./run.sh test_suites/gpubench_pp_baseline ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_baseline ${ARGS} --ppn 8

# NCCL
source src/microbench-gpu/moduleload/load_Nccl_modules.sh
./run.sh test_suites/gpubench_pp_nccl ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_nccl ${ARGS} --ppn 8

# NVLink
source src/microbench-gpu/moduleload/load_Nvlink_modules.sh
./run.sh test_suites/gpubench_pp_nvlink ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_nvlink ${ARGS} --ppn 8

# CUDA-Aware MPI
source src/microbench-gpu/moduleload/load_CudaAware_modules.sh
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/gpubench_pp_ca ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_ca ${ARGS} --ppn 8
unset MPICH_GPU_SUPPORT_ENABLED