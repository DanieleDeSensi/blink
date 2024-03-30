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
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 5"
PPN=4
for SL in 0 1; do
    export UCX_IB_SL=${SL}
    export NCCL_IB_SL=${SL}
    # HCOLL Disabled
    HCOLL=0 
    EXTRA="SL${SL}_hcoll${HCOLL}"    
    ./run.sh test_suites/coll-gpu-fast-congestion-cudaaware ${ARGS} --ppn ${PPN} -e ${EXTRA}
    ./run.sh test_suites/coll-gpu-fast-congestion-nccl ${ARGS} --ppn ${PPN} -e ${EXTRA}

    # HCOLL Enabled (CUDA-Aware only)
    HCOLL=1 
    EXTRA="SL${SL}_hcoll${HCOLL}"    
    HCOLL_GPU=cuda HCOLL_GPU_ENABLE=1 OMPI_MCA_coll_hcoll_enable=${HCOLL} ./run.sh test_suites/coll-gpu-fast-congestion-cudaaware ${ARGS} --ppn ${PPN} -e ${EXTRA}
done

