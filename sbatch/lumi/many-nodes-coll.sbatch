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
PPN=8
./run.sh test_suites/coll-nccl-many ${ARGS} --ppn ${PPN}
./run.sh test_suites/coll-cudaaware-many ${ARGS} --ppn ${PPN}
./run.sh test_suites/coll-cpu-many ${ARGS} --ppn ${PPN}