#!/bin/bash
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --mem=0
#SBATCH --partition=boost_usr_prod
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-coll
#SBATCH --account=iscrc_sharp_0

source conf.sh

for SL in 1 0; do
    export UCX_IB_SL=${SL}
    export NCCL_IB_SL=${SL}
    EXTRA="SL${SL}"
    for SP in "50:50"; do
        for ALLOC_MODE in "l" "r" "i"; do
            ARGS="auto -n ${SLURM_NNODES} -sp "${SP}" -am ${ALLOC_MODE} -ro +file --maxruns 5"
            ./run.sh test_suites/gpubench_congestion ${ARGS} --ppn 4 -e ${EXTRA}
        done
    done
done
