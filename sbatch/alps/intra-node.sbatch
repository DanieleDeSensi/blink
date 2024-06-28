#!/bin/bash
#SBATCH --nodes=1
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --exclusive
#SBATCH --job-name=blink-intra-node
source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"

# Baseline
./run.sh test_suites/gpubench_pp_baseline ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_baseline ${ARGS} --ppn 4

# NCCL
./run.sh test_suites/gpubench_pp_nccl ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_nccl ${ARGS} --ppn 4
#./run.sh test_suites/comm-nccl ${ARGS} --ppn 4

# NVLink
# ./run.sh test_suites/gpubench_pp_nvlink ${ARGS} --ppn 2
# ./run.sh test_suites/gpubench_coll_nvlink ${ARGS} --ppn 4

# CUDA-Aware MPI
export MPICH_GPU_SUPPORT_ENABLED=1
./run.sh test_suites/gpubench_pp_ca ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_ca ${ARGS} --ppn 4
#./run.sh test_suites/comm-cudaaware ${ARGS} --ppn 4
unset MPICH_GPU_SUPPORT_ENABLED
