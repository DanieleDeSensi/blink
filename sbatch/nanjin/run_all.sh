#!/bin/bash
NUM_NODES=("8") # Max. number of nodes we want to run our code on
NTASKS_PER_NODE=64 # Number of cores each node have
PARTITIONS=("cn-eth") # Slurm partitions we want to run our code on

# No modifications should be required after this point
LAST_JOB_ID=0
for PARTITION in "${PARTITIONS[@]}"
do
    #LAST_JOB_ID=$(source  sbatch/nanjin/pingpong.sbatch --nodes=2 )
    # Run the tests on more than 2 nodes
    #SBATCHES=("sbatch/haicgu/alltoall.sbatch" "sbatch/haicgu/allreduce.sbatch" "sbatch/haicgu/incast.sbatch")
    SBATCHES=("sbatch/nanjin/allreduce.sbatch")
    for SBATCH in "${SBATCHES[@]}"
    do
        for NODES in "${NUM_NODES[@]}"
        do
            LAST_JOB_ID=$(source ${SBATCH} --nodes=${NODES})
        done
    done
done
