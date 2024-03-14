#!/bin/bash
#SBATCH --nodes=8
#SBATCH --time=04:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod 
#SBATCH --exclusive
#SBATCH --job-name=blink-sl-congestion
#SBATCH --account=iscrc_sharp_0
source conf.sh
for SL in 0 1
do
    for VAR in 0 1
    do
        ARGS="auto -n ${SLURM_NNODES} -sp 10:90 -am r --ppn 4 -ro +file -e SL${SL}VAR${VAR} --timeout 600"
        NCCL_IB_ADAPTIVE_ROUTING=${VAR} NCCL_IB_SL=${SL} UCX_IB_SL=${SL} ./run.sh test_suites/gpubench_mpp_congestion ${ARGS}
    done
done
