#!/bin/bash
#SBATCH --time=04:00:00
#SBATCH --ntasks-per-node=8
#SBATCH --gpus-per-node=8
#SBATCH -p ju-standard-g
#SBATCH -A project_465000997
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-coll

source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 3 --timeout 300"
PPN_GPU=8
PPN_CPU=4

export NCCL_BUFFSIZE=33554432

# GPU mapping and NUMA nodes mapping
#export USER_HIP_GPU_MAP="1,3,5,7,0,2,4,6"
#export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu:57,25,9,41,49,17,1,33" # Pinning flags
export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu:49,57,17,25,1,9,33,41"

# GPU tests, PPN=8
# NCCL
source src/microbench-gpu/moduleload/load_Nccl_modules.sh
./run.sh test_suites/coll-nccl-many ${ARGS} --ppn ${PPN_GPU}

# CUDA-Aware
source src/microbench-gpu/moduleload/load_CudaAware_modules.sh
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/coll-cudaaware-many ${ARGS} --ppn ${PPN_GPU}
unset MPICH_GPU_SUPPORT_ENABLED

# CPU tests, PPN=4
module purge
module load LUMI/23.09
module load partition/G
#export MPICH_OFI_NIC_POLICY=NUMA
export HSA_ENABLE_SDMA=0
export BLINK_PINNING_FLAGS="--cpu-bind=map_cpu=57,25,9,41"
./run.sh test_suites/coll-cpu-many ${ARGS} --ppn ${PPN_CPU}
