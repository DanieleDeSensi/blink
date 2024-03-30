#!/bin/bash
#SBATCH --time=02:00:00
#SBATCH --ntasks-per-node=8
#SBATCH --gpus-per-node=8
#SBATCH -p ju-standard-g
#SBATCH -A project_465000997
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-quick-cong

source conf.sh
ARGS="auto -n ${SLURM_NNODES} -sp 50:50 -am +r -ro +file --minruns 10 --maxruns 10"
PPN=8
SL=0
HCOLL=0 
EXTRA="SL${SL}_hcoll${HCOLL}"    
./run.sh test_suites/coll-gpu-fast-congestion-cudaaware ${ARGS} --ppn ${PPN} -e ${EXTRA}
./run.sh test_suites/coll-gpu-fast-congestion-nccl ${ARGS} --ppn ${PPN} -e ${EXTRA}

