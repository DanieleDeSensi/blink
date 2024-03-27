#!/bin/bash
#SBATCH --time=08:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-coll

source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 3 --timeout 300"
PPN_GPU=4
PPN_CPU=4

# GPU tests, PPN=4
# NCCL
./run.sh test_suites/coll-nccl-many ${ARGS} --ppn ${PPN_GPU}
# ./run.sh test_suites/comm-nccl ${ARGS} --ppn ${PPN_GPU}

# CUDA-Aware
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/coll-cudaaware-many ${ARGS} --ppn ${PPN_GPU}
./run.sh test_suites/comm-cudaaware ${ARGS} --ppn ${PPN_GPU}
unset MPICH_GPU_SUPPORT_ENABLED

# CPU tests, PPN=4
./run.sh test_suites/coll-cpu-many ${ARGS} --ppn ${PPN_CPU}
