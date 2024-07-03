#!/bin/bash
NTASKS_PER_NODE=16 # Number of cores each node have
PARTITIONS=("cn-eth" "cn-ib") # Slurm partitions we want to run our code on

# No modifications should be required after this point
SBATCHES=("sbatch/haicgu/pingpong.sbatch" "sbatch/haicgu/alltoall.sbatch" "sbatch/haicgu/allreduce.sbatch" "sbatch/haicgu/incast.sbatch")
LAST_JOB_ID=0
for PARTITION in "${PARTITIONS[@]}"
do
    for SBATCH in "${SBATCHES[@]}"
    do
        DEPENDENCY=""
        if [ ${LAST_JOB_ID} -gt 0 ]; then
            DEPENDENCY="--dependency=afterany:${LAST_JOB_ID}"
        fi
        LAST_JOB_ID=$(sbatch ${DEPENDENCY} --parsable --partition=${PARTITION} --ntasks-per-node=${NTASKS_PER_NODE} ${SBATCH} | cut -d ';' -f 1)
    done
done