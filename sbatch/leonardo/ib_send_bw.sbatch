#!/bin/bash
#SBATCH --nodes=2
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod 
#SBATCH --exclusive
#SBATCH --job-name=blink-ib_send_bw
#SBATCH --account=iscrc_sharp_0

SL=1
export UCX_IB_SL=${SL}
export NCCL_IB_SL=${SL}
ALLOCATION="same_switch"
# The server goes first, thus the first hostname is that of the server
SERVER_HOSTNAME=$(scontrol show hostname | head -n 1)

for TRANSPORT in RC UC; do
    EXTRA="${ALLOCATION}_${TRANSPORT}"
    ARGS="auto -n ${SLURM_NNODES} -sp 50:50 -am l -ro +file --minruns 10"   
    ./run.sh test_suites/ib_send_bw ${ARGS} --ppn 4 -e ${EXTRA} --replace_mix_args "%SERVER_ADDRESS:${SERVER_HOSTNAME},%SERVICE_LEVEL:${SL},%TRANSPORT:${TRANSPORT}"
done
