#!/bin/bash
#SBATCH --nodes=2
#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gpus-per-node=8
#SBATCH --partition=ju-standard-g
#SBATCH --exclusive
#SBATCH --job-name=blink-two-nodes
#SBATCH --account=project_465000997
source conf.sh
ALLOCATION="same_switch"
EXTRA="${ALLOCATION}_SL0"
ARGS="auto -n ${SLURM_NNODES} -sp 100 -am l -ro +file"
for PPN in 4; do
    ##################
    # Point-to-point #
    ##################
    # NCCL    
    #./run.sh test_suites/pp-nccl ${ARGS} --ppn ${PPN} -e ${EXTRA} 
    # CUDA-Aware
    #./run.sh test_suites/pp-cudaaware ${ARGS} --ppn ${PPN} -e ${EXTRA} 
    # MPI
    ./run.sh test_suites/pp-mpi ${ARGS} --ppn ${PPN} -e ${EXTRA} 

    # Collectives (with/without HCOLL)
    #./run.sh test_suites/two-nodes-coll-cpu ${ARGS} --ppn ${PPN} -e ${EXTRA}_hcoll0
    #./run.sh test_suites/two-nodes-coll-gpu ${ARGS} --ppn ${PPN} -e ${EXTRA}_hcoll0
done