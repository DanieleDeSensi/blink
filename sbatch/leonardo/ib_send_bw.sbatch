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
    for DEVS in "mlx5_0#mlx5_1#mlx5_2#mlx5_3"; do
	# Even if its is PPN=1, it spawn 4 processes (one per device)	
        ./run.sh test_suites/pp-ib ${ARGS} --ppn 1 -e ${EXTRA} --replace_mix_args "%SERVER_ADDRESS:${SERVER_HOSTNAME},%SERVICE_LEVEL:${SL},%TRANSPORT:${TRANSPORT},%IB_DEVICES_LIST:${DEVS}"
    done

done
