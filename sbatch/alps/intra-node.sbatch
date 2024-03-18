#!/bin/bash
#SBATCH --nodes=1
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=16
#SBATCH --gres=gpu:16
#SBATCH --partition=boost_usr_prod 
#SBATCH --exclusive
#SBATCH --job-name=blink-intra-node
#SBATCH --account=iscrc_sharp_0
source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"

./run.sh test_suites/gpubench_pp_noca ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll_noca ${ARGS} --ppn 4
./run.sh test_suites/gpubench_coll_noca ${ARGS} --ppn 8
./run.sh test_suites/gpubench_coll_noca ${ARGS} --ppn 16