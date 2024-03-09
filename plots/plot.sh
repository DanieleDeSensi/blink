#!/bin/bash

##################
#### Leonardo ####
##################
SYSTEM="leonardo"

##########################
# Adaptive routing tests #
##########################
OUT_PATH="plots/out/${SYSTEM}/routing"

# Two nodes, different switches
for BENCH in gpubench-mpp-nccl
do
    for INPUT in 1B 1GiB
    do
        TESTNAME=${BENCH}_${INPUT}
        EXTRAS=diff_groups_AR0,diff_groups_AR1,diff_groups_SL0,diff_groups_SL1
        #EXTRAS=same_switch_AR0,same_switch_AR1,diff_switch_AR0,diff_switch_AR1,diff_groups_AR0,diff_groups_AR1
        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 2 -am l -sp 100 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
    done
done



#####################
# Single-node tests #
#####################
INPUTS="8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
OUT_PATH="plots/out/${SYSTEM}/single_node"

#NCCL Tests - Point-to-Point
for TESTNAME in nccl-sendrecv
do
    TREND_LIMIT=0_busbw-ip_GB/s:80
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${INPUTS} -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_busbw-oop_GB/s -o ${OUT_PATH}/${TESTNAME} --ppn 2 --trend_limit ${TREND_LIMIT}
done

#NCCL Tests - Collectives
for TESTNAME in nccl-allreduce nccl-alltoall
do
    TREND_LIMIT=0_busbw-ip_GB/s:230
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${INPUTS} -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_busbw-oop_GB/s -o ${OUT_PATH}/${TESTNAME} --ppn 4 --trend_limit ${TREND_LIMIT}
done

#GPUBench - Point-to-Point
for TESTNAME in gpubench-pp
do
    TREND_LIMIT=0_Bandwidth_GB/s:80
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware,${TESTNAME}-nvlink -vi ${INPUTS} -n 1 -am l -sp 100 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o ${OUT_PATH}/${TESTNAME} --ppn 2 --trend_limit ${TREND_LIMIT}
done

# GPUBench - Collectives
for TESTNAME in gpubench-a2a gpubench-ar
do
    TREND_LIMIT=0_Bandwidth_GB/s:230
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware,${TESTNAME}-nvlink -vi ${INPUTS} -n 1 -am l -sp 100 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o ${OUT_PATH}/${TESTNAME} --ppn 4 --trend_limit ${TREND_LIMIT}
done