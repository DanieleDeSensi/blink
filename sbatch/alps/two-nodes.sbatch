#!/bin/bash
#SBATCH --nodelist=nid001058,nid001056
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --exclusive
#SBATCH --job-name=blink-two-nodes
source conf.sh
ALLOCATION="same_switch"

EXTRA="${ALLOCATION}"
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"
PPN=4
##################
# Point-to-point #
##################
# NCCL    
./run.sh test_suites/pp-nccl ${ARGS} --ppn ${PPN} -e ${EXTRA} 
# CUDA-Aware
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/pp-cudaaware ${ARGS} --ppn ${PPN} -e ${EXTRA} 
unset MPICH_GPU_SUPPORT_ENABLED
# MPI
./run.sh test_suites/pp-mpi ${ARGS} --ppn ${PPN} -e ${EXTRA} 

# Collectives
./run.sh test_suites/two-nodes-coll-cpu ${ARGS} --ppn ${PPN} -e ${EXTRA}
./run.sh test_suites/two-nodes-coll-gpu-nccl ${ARGS} --ppn ${PPN} -e ${EXTRA}
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/two-nodes-coll-gpu-cudaaware ${ARGS} --ppn ${PPN} -e ${EXTRA}
unset MPICH_GPU_SUPPORT_ENABLED