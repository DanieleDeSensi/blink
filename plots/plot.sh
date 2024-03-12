#!/bin/bash

##################
#### Leonardo ####
##################
SYSTEM="leonardo"

########################
# Service levels tests #
########################
OUT_PATH="plots/out/${SYSTEM}/sl/"
BENCH="gpubench-mpp-nccl"
EXTRAS=SL0VAR0,SL0VAR1,SL1VAR0,SL1VAR1
for INPUT in 1B 1GiB
do
    TESTNAME=${BENCH}_${INPUT}
    ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 16 -am r -sp 10:90 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
    for AGGRESSOR in a2a_b inc_b 
    do
        TESTNAME=${BENCH}_${INPUT}_${AGGRESSOR}
        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} --aggressor_name ${AGGRESSOR} --aggressor_input 128KiB -n 16 -am r -sp 10:90 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
    done
done

EXTRAS=SL0,SL1
INPUT="1GiB"
AGGRESSOR="gpubench-a2a-nccl"
TESTNAME=${BENCH}_${INPUT}
./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 16 -am r -sp 10:90 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
TESTNAME=${BENCH}_${INPUT}_${AGGRESSOR}
./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} --aggressor_name ${AGGRESSOR} --aggressor_input 128KiB -n 16 -am r -sp 10:90 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}

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
        EXTRAS=diff_switch_SL0,diff_switch_SL1,diff_groups_SL0,diff_groups_SL1
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



###################
# Two-nodes tests #
###################
OUT_PATH="plots/out/${SYSTEM}/two-nodes/"
EXTRAS=same_switch_SL0,same_switch_SL1,diff_switch_SL0,diff_switch_SL1,diff_groups_SL0,diff_groups_SL1
for BENCH in "gpubench-mpp-nccl" "gpubench-mpp-cudaaware"
do
    for INPUT in 1B 1GiB
    do
        TESTNAME=${BENCH}_${INPUT}
        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 2 -am l -sp 100 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
    done
done