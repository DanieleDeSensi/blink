#!/bin/bash
#SBATCH --nodes=2
#SBATCH --time=01:00:00
#SBATCH --exclusive
#SBATCH --job-name=blink-pingpong
source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file -e ${SBATCH_PARTITION} -mn 10" # -mn 10
# We run pingpong test with 1 PPN
./run.sh test_suites/cpu/pingping ${ARGS} --ppn 1
