#!/bin/bash
#SBATCH --time=03:00:00
#SBATCH --ntasks-per-node=8
#SBATCH --gpus-per-node=8
#SBATCH -p ju-standard-g
#SBATCH -A project_465000997
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-coll

source conf.sh
ARGS="auto -n ${SLURM_NNODES} -ro +file --maxruns 5 --timeout 300"

# GPU tests, PPN=8
# NCCL
source src/microbench-gpu/moduleload/load_Nccl_modules.sh

for SP in "10:90" "50:50" "90:10"
do
    for AM in "l" "r" "i"
    do
        for ERDVZ in 1024 4096 16384 65536 262144 1048576
        do
            export FI_CXI_RDZV_EAGER_SIZE=${ERDVZ}
            ./run.sh test_suites/gpubench_lumi_routing ${ARGS} -sp ${SP} -am ${AM} --ppn 8 --extra ERDVZ${ERDVZ}
        done
    done
done
