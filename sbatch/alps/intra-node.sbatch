#!/bin/bash
#SBATCH --nodes=1
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --exclusive
#SBATCH --job-name=blink-intra-node
source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"

./run.sh test_suites/gpubench_pp_alps ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_alps ${ARGS} --ppn 4