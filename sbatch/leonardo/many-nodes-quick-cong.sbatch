#!/bin/bash
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=17
#SBATCH --gres=gpu:4
#SBATCH --mem=0
#SBATCH --partition=boost_usr_prod
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-coll
#SBATCH --account=iscrc_sharp_0

source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 50:50 -am +r -ro +file --minruns 10 --maxruns 10"
PPN=4
for SL in 0 1; do
    export UCX_IB_SL=${SL}
    export NCCL_IB_SL=${SL}
    # HCOLL Disabled
    HCOLL=0 
    EXTRA="SL${SL}_hcoll${HCOLL}"    
    ./run.sh test_suites/coll-gpu-fast-congestion-cudaaware-leo ${ARGS} --ppn ${PPN} -e ${EXTRA}
    ./run.sh test_suites/coll-gpu-fast-congestion-nccl ${ARGS} --ppn ${PPN} -e ${EXTRA}
done

