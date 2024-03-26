#!/bin/bash
#SBATCH --nodes=2
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=8
#SBATCH --gpus-per-node=8
#SBATCH -p ju-standard-g
#SBATCH -A project_465000997
#SBATCH --exclusive
#SBATCH --job-name=blink-two-nodes
source conf.sh
ALLOCATION="same_switch"

EXTRA="${ALLOCATION}"
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"
PPN_GPU=8
PPN_CPU=4

export NCCL_BUFFSIZE=33554432

# GPU mapping and NUMA nodes mapping
export USER_HIP_GPU_MAP="1,3,5,7,0,2,4,6"
export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu:57,25,9,41,49,17,1,33" # Pinning flags


# NCCL    
source src/microbench-gpu/moduleload/load_Nccl_modules.sh
./run.sh test_suites/pp-nccl ${ARGS} --ppn ${PPN_GPU} -e ${EXTRA} 
./run.sh test_suites/two-nodes-coll-gpu-nccl ${ARGS} --ppn ${PPN_GPU} -e ${EXTRA}

# CUDA-Aware
export MPICH_GPU_SUPPORT_ENABLED=1
source src/microbench-gpu/moduleload/load_CudaAware_modules.sh
./run.sh test_suites/pp-cudaaware ${ARGS} --ppn ${PPN_GPU} -e ${EXTRA} 
./run.sh test_suites/two-nodes-coll-gpu-cudaaware ${ARGS} --ppn ${PPN_GPU} -e ${EXTRA}
unset MPICH_GPU_SUPPORT_ENABLED

# MPI
module purge
module load LUMI/23.09
module load partition/G
export HSA_ENABLE_SDMA=0
./run.sh test_suites/pp-mpi ${ARGS} --ppn 4 -e ${EXTRA} 
./run.sh test_suites/two-nodes-coll-cpu ${ARGS} --ppn ${PPN_CPU} -e ${EXTRA}

