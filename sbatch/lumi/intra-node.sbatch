#!/bin/bash
#SBATCH --nodes=1
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=8
#SBATCH --gpus-per-node=8
#SBATCH -p ju-standard-g
#SBATCH -A project_465000997
#SBATCH --exclusive
#SBATCH --job-name=blink-intra-node
source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"

./run.sh test_suites/gpubench_pp ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll ${ARGS} --ppn 8