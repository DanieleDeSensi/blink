#!/bin/bash
#SBATCH --time=01:00:00
#SBATCH --exclusive
#SBATCH --job-name=blink-allreduce
source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file -e ${SBATCH_PARTITION} -mn 10" # -mn 10
./run.sh test_suites/cpu/ar ${ARGS} --ppn 1
