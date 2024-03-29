#!/bin/bash
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=17
#SBATCH --gres=gpu:4
#SBATCH --mem=0
#SBATCH --partition=boost_usr_prod
#SBATCH --exclusive
#SBATCH --job-name=blink-many-nodes-coll
#SBATCH --account=iscrc_sharp_0

source conf.sh
EXTRA="SL0"
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file --maxruns 5"
PPN=4
HCOLL=0
./run.sh test_suites/coll-gpu-fast ${ARGS} --ppn ${PPN} -e ${EXTRA}_hcoll${HCOLL}

