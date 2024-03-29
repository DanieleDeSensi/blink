#!/bin/bash
#SBATCH --time=08:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-coll

source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 3 --timeout 300"
PPN_GPU=4
PPN_CPU=4

# CUDA-Aware
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/coll-cudaaware-many-fast ${ARGS} --ppn ${PPN_GPU}
unset MPICH_GPU_SUPPORT_ENABLED
