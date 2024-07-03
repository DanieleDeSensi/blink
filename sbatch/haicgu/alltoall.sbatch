#!/bin/bash
#SBATCH --time=01:00:00
#SBATCH --exclusive
#SBATCH --job-name=blink-alltoall
source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file -e ${SBATCH_PARTITION} -mn 10" # -mn 10
# We run alltoall test with 1 PPN
./run.sh test_suites/cpu/a2a ${ARGS} --ppn 1
# We run alltoall test with full PPN
./run.sh test_suites/cpu/a2a ${ARGS} --ppn ${SLURM_NTASKS_PER_NODE}
