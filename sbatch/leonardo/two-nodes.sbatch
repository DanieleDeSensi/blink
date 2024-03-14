#!/bin/bash
#SBATCH --nodes=2
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod 
#SBATCH --exclusive
#SBATCH --job-name=blink-two-nodes
#SBATCH --account=iscrc_sharp_0

ALLOCATION="same_switch"

for SL in 1; do
    export UCX_IB_SL=${SL}
    export NCCL_IB_SL=${SL}
    EXTRA="${ALLOCATION}_SL${SL}"
    ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"
    for PPN in 1 4; do
        ./run.sh test_suites/two-nodes-pp ${ARGS} --ppn ${PPN} -e ${EXTRA}
        for HCOLL in 0 1
        do
            OMPI_MCA_coll_hcoll_enable=${HCOLL} ./run.sh test_suites/two-nodes-coll-cpu ${ARGS} --ppn ${PPN} -e ${EXTRA}_hcoll${HCOLL}
            HCOLL_GPU=cuda HCOLL_GPU_ENABLE=1 OMPI_MCA_coll_hcoll_enable=${HCOLL} ./run.sh test_suites/two-nodes-coll-gpu ${ARGS} --ppn ${PPN} -e ${EXTRA}_hcoll${HCOLL}
        done
    done
done