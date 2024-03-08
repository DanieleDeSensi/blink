#!/bin/bash
#SBATCH --nodes=1
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod 
#SBATCH --exclusive
#SBATCH --job-name=blink-intra-node
#SBATCH --account=iscrc_sharp_0

ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"

./run.sh test_suites/gpubench_pp ${ARGS} --ppn 2
./run.sh test_suites/gpubench_coll ${ARGS} --ppn 4
./run.sh test_suites/nccl_sendrecv ${ARGS} --ppn 2 # Just to test bandwidth between a pair of GPUs
./run.sh test_suites/nccl_coll ${ARGS} --ppn 4
