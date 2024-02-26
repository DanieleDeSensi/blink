#!/bin/bash
NUM_NODES=2048
MIN_RUNS_SMALL=10
MIN_RUNS_BIG=1
TIMEOUT_SMALL=60
TIMEOUT_BIG=90 # Timeout for each test case inside the suite

# 9 test case in each ./run.sh invocation
# Worst case runtime (small) = 9*2*3*3*30 seconds = 1.35 hours
# Worst case runtime (big)   = 9*2*3*3*60 seconds = 2.7  hours

for am in "l" "r+" "i"
do
    for sp in "50:50" "90:10" "10:90"
    do
        NCCL_IB_ADAPTIVE_ROUTING=0 ./run.sh test_suites/leonardo_big   auto -n ${NUM_NODES} -sp ${sp} -am ${am} --ppn 1 -e AR0 -mn ${MIN_RUNS_BIG}   --timeout ${TIMEOUT_BIG}
        NCCL_IB_ADAPTIVE_ROUTING=1 ./run.sh test_suites/leonardo_big   auto -n ${NUM_NODES} -sp ${sp} -am ${am} --ppn 1 -e AR1 -mn ${MIN_RUNS_BIG}   --timeout ${TIMEOUT_BIG}
        NCCL_IB_ADAPTIVE_ROUTING=0 ./run.sh test_suites/leonardo_small auto -n ${NUM_NODES} -sp ${sp} -am ${am} --ppn 1 -e AR0 -mn ${MIN_RUNS_SMALL} --timeout ${TIMEOUT_SMALL}
        NCCL_IB_ADAPTIVE_ROUTING=1 ./run.sh test_suites/leonardo_small auto -n ${NUM_NODES} -sp ${sp} -am ${am} --ppn 1 -e AR1 -mn ${MIN_RUNS_SMALL} --timeout ${TIMEOUT_SMALL}
    done
done

# TODO: 
# 1. modify gpubench (pp) to support more ppn
# 2. run everything at max ppn
# 3. run aggressor on NCCL as well (so that routing affects both)