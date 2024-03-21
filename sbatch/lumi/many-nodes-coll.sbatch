#!/bin/bash
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=8
#SBATCH --gpus-per-node=8
#SBATCH -p ju-standard-g
#SBATCH -A project_465000997
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-coll

source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 3"
# GPU tests, PPN=8
./run.sh test_suites/coll-nccl-many ${ARGS} --ppn 8
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/coll-cudaaware-many ${ARGS} --ppn 8
unset MPICH_GPU_SUPPORT_ENABLED

# CPU tests, PPN=4
./run.sh test_suites/coll-cpu-many ${ARGS} --ppn 4
