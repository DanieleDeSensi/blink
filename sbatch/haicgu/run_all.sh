#!/bin/bash
NUM_NODES=("2" "4" "8" "16") # Max. number of nodes we want to run our code on
NTASKS_PER_NODE=16 # Number of cores each node have
PARTITIONS=("cn-eth" "cn-ib") # Slurm partitions we want to run our code on

# No modifications should be required after this point
LAST_JOB_ID=0
for PARTITION in "${PARTITIONS[@]}"
do
    # Run the tests on 2 nodes
    DEPENDENCY=""
    if [ ${LAST_JOB_ID} -gt 0 ]; then
        DEPENDENCY="--dependency=afterany:${LAST_JOB_ID}"
    fi
    LAST_JOB_ID=$(sbatch ${DEPENDENCY} --nodes=2 --parsable --partition=${PARTITION} --ntasks-per-node=${NTASKS_PER_NODE} sbatch/haicgu/pingpong.sbatch | cut -d ';' -f 1)
    # Run the tests on more than 2 nodes
    SBATCHES=("sbatch/haicgu/alltoall.sbatch" "sbatch/haicgu/allreduce.sbatch" "sbatch/haicgu/incast.sbatch")
    for SBATCH in "${SBATCHES[@]}"
    do
        for NODES in "${NUM_NODES[@]}"
        do
            DEPENDENCY=""
            if [ ${LAST_JOB_ID} -gt 0 ]; then
                DEPENDENCY="--dependency=afterany:${LAST_JOB_ID}"
            fi
            LAST_JOB_ID=$(sbatch ${DEPENDENCY} --nodes=${NODES} --parsable --partition=${PARTITION} --ntasks-per-node=${NTASKS_PER_NODE} ${SBATCH} | cut -d ';' -f 1)
        done
    done
done