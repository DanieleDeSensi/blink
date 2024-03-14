#!/bin/bash

##################
#### Leonardo ####
##################
SYSTEM="leonardo"


###############
# HCOLL tests #
###############
#OUT_PATH="plots/out/${SYSTEM}/two-nodes/"
#PLOT_TYPE="box,violin"
#for COLL in "gpubench-ar-nccl" "gpubench-ar-cudaaware" "gpubench-a2a-nccl" "gpubench-a2a-cudaaware" "ardc_b" "a2a_b"
#do
#    for PPN in 1 4
#    do
#        EXTRAS=same_switch_SL1_hcoll0,same_switch_SL1_hcoll1
#        TESTNAME="hcoll_latency_ppn"${PPN}"_"${COLL}
#        ./plots/plot_extras.py -s ${SYSTEM} -vn ${COLL} -vi 1B -n 2 -am l -sp 100 --metrics "Runtime" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRAS} --plot_types ${PLOT_TYPE}
#        TESTNAME="hcoll_bandwidth_ppn"${PPN}"_"${COLL}
#        ./plots/plot_extras.py -s ${SYSTEM} -vn ${COLL} -vi 128MiB -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRAS} --plot_types ${PLOT_TYPE}
#    done
#done

#####################
# Single-node tests #
#####################
INPUTS="8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
OUT_PATH="plots/out/${SYSTEM}/single_node"
PLOT_TYPE="line"
#GPUBench - Point-to-Point
for TESTNAME in gpubench-pp
do
    #TREND_LIMIT=Bandwidth:704
    TREND_LIMIT=Bandwidth:800
    INNER_YLIM="[0, 30]"
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware,${TESTNAME}-nvlink -vi ${INPUTS} -n 1 -am l -sp 100 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 2 --trend_limit ${TREND_LIMIT} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}"
done

# GPUBench - A2A
TESTNAME=gpubench-a2a
#TREND_LIMIT=Bandwidth:1840
TREND_LIMIT=Bandwidth:2400
./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware,${TESTNAME}-nvlink -vi ${INPUTS} -n 1 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 --trend_limit ${TREND_LIMIT} --plot_types ${PLOT_TYPE}

# GPUBench - AR
TESTNAME=gpubench-ar
#TREND_LIMIT=Bandwidth:1840
TREND_LIMIT=Bandwidth:2400
./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware -vi ${INPUTS} -n 1 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 --trend_limit ${TREND_LIMIT} --plot_types ${PLOT_TYPE}

###################
# Two-nodes tests #
###################
OUT_PATH="plots/out/${SYSTEM}/two-nodes/"
EXTRA="same_switch_SL1"
PLOT_TYPE="line"
INPUTS="8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
for PPN in 1 4
do
    # P2P
    TESTNAME="allsizes_pp_PPN${PPN}"
    ./plots/plot_inputs.py -s ${SYSTEM} -vn gpubench-mpp-nccl,gpubench-mpp-cudaaware,pw-ping-pong_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE}
    # AR
    TESTNAME="allsizes_ar_PPN${PPN}"
    ./plots/plot_inputs.py -s ${SYSTEM} -vn gpubench-ar-nccl,gpubench-ar-cudaaware,ardc_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE}
    # A2A
    TESTNAME="allsizes_a2a_PPN${PPN}"
    INNER_YLIM="[0, 30]"
    TREND_LIMIT=Bandwidth:400
    ./plots/plot_inputs.py -s ${SYSTEM} -vn gpubench-a2a-nccl,gpubench-a2a-cudaaware,a2a_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT}
done


##################
# Distance tests #
##################
PLOT_TYPE="box,violin"
for SL in 0 1
do
    EXTRAS=same_switch_SL${SL},diff_switch_SL${SL},diff_group_SL${SL}
    # Buffer on CPU memory
    # PPN=1 for latency, pingpong
    TESTNAME="latency_dist_SL${SL}"
    ./plots/plot_extras.py -s ${SYSTEM} -vn "ping-pong_b" -vi 1B -n 2 -am l -sp 100 --metrics "Runtime" -o ${OUT_PATH}/${TESTNAME} --ppn 1 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
    # Buffer on GPU memory
    # PPN=4 for bandwidth
    TESTNAME="bandwidth_dist_SL${SL}"
    ./plots/plot_extras.py -s ${SYSTEM} -vn "gpubench-mpp-nccl" -vi 1GiB -n 2 -am l -sp 100 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
done




########################
# Service levels tests #
########################
PLOT_TYPE="box,line"
EXTRAS=same_switch_SL0,same_switch_SL1,diff_switch_SL0,diff_switch_SL1,diff_group_SL0,diff_group_SL1
# Buffer on GPU memory
for BENCH in "gpubench-mpp-nccl" "gpubench-mpp-cudaaware"
do
    # PPN=1 for latency
    TESTNAME=${BENCH}_1B
    ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi 1B -n 2 -am l -sp 100 --metrics "Runtime" -o ${OUT_PATH}/${TESTNAME} --ppn 1 -e ${EXTRAS} --plot_types ${PLOT_TYPE}

    # PPN=4 for bandwidth
    TESTNAME=${BENCH}_1GiB
    ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi 1GiB -n 2 -am l -sp 100 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
done
# Buffer on CPU memory
# PPN=1 for latency, pingpong
TESTNAME=ping-pong_b_1B
./plots/plot_extras.py -s ${SYSTEM} -vn "ping-pong_b" -vi 1B -n 2 -am l -sp 100 --metrics "Runtime" -o ${OUT_PATH}/${TESTNAME} --ppn 1 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
# PPN=4 for bandwidth, pw-pingpong
TESTNAME=pw-ping-pong_b_1GiB
./plots/plot_extras.py -s ${SYSTEM} -vn "pw-ping-pong_b" -vi 1GiB -n 2 -am l -sp 100 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} --plot_types ${PLOT_TYPE}


exit 0
# Old tests
INPUTS="8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
OUT_PATH="plots/out/${SYSTEM}/single_node"

#NCCL Tests - Point-to-Point
for TESTNAME in nccl-sendrecv
do
    TREND_LIMIT=Bandwidth:80
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${INPUTS} -n 1 -am l -sp 100 --metrics Bandwidth -o ${OUT_PATH}/${TESTNAME} --ppn 2 --trend_limit ${TREND_LIMIT}
done

#NCCL Tests - Collectives
for TESTNAME in nccl-allreduce nccl-alltoall
do
    TREND_LIMIT=Bandwidth:230
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${INPUTS} -n 1 -am l -sp 100 --metrics Bandwidth -o ${OUT_PATH}/${TESTNAME} --ppn 4 --trend_limit ${TREND_LIMIT}
done

########################
# Service levels tests #
########################
OUT_PATH="plots/out/${SYSTEM}/sl/"
BENCH="gpubench-mpp-nccl"
EXTRAS=SL0VAR0,SL0VAR1,SL1VAR0,SL1VAR1
for INPUT in 1B 1GiB
do
    TESTNAME=${BENCH}_${INPUT}
    ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 16 -am r -sp 10:90 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
    for AGGRESSOR in a2a_b inc_b 
    do
        TESTNAME=${BENCH}_${INPUT}_${AGGRESSOR}
        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} --aggressor_name ${AGGRESSOR} --aggressor_input 128KiB -n 16 -am r -sp 10:90 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
    done
done

EXTRAS=SL0,SL1
INPUT="1GiB"
AGGRESSOR="gpubench-a2a-nccl"
TESTNAME=${BENCH}_${INPUT}
./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 16 -am r -sp 10:90 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
TESTNAME=${BENCH}_${INPUT}_${AGGRESSOR}
./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} --aggressor_name ${AGGRESSOR} --aggressor_input 128KiB -n 16 -am r -sp 10:90 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}

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
        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 2 -am l -sp 100 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS}
    done
done