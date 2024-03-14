#!/bin/bash
#SBATCH --nodes=2
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod 
#SBATCH --exclusive
#SBATCH --job-name=blink-two-nodes
#SBATCH --account=iscrc_sharp_0
source conf.sh
ALLOCATION="diff_group"

for SL in 0 1; do
    export UCX_IB_SL=${SL}
    export NCCL_IB_SL=${SL}
    EXTRA="${ALLOCATION}_SL${SL}"
    ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file -e ${EXTRA}"
    # We run latency test with 1 PPN
    ./run.sh test_suites/latency ${ARGS} --ppn 1
    # We run bandwidth test with 4 PPN
    ./run.sh test_suites/bandwidth ${ARGS} --ppn 4
done
