#!/bin/bash
#SBATCH --nodes=2
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod 
#SBATCH --exclusive
#SBATCH --job-name=blink-two-nodes
#SBATCH --account=iscrc_sharp_0
source conf.sh
ALLOC="same_switch"

for AR in 0 1
do
    ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l --ppn 4 -ro +file -e ${ALLOC}_AR${AR} --minruns 10"
    NCCL_IB_ADAPTIVE_ROUTING=${AR} ./run.sh test_suites/gpubench-pp-nccl-routing ${ARGS}
done

for SL in 0 1
do
    ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l --ppn 4 -ro +file -e ${ALLOC}_SL${SL} --minruns 10"
    NCCL_IB_SL=${SL} ./run.sh test_suites/gpubench-pp-nccl-routing ${ARGS}
done
