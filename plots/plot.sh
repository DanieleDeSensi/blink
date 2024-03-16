#!/bin/bash

##################
#### Leonardo ####
##################
SYSTEM="leonardo"

##################
# 64 nodes tests #
##################
OUT_PATH="plots/out/${SYSTEM}/64-nodes/"
PLOT_TYPE="line"
for SL in 0 1
do
    for HCOLL in 0 
    do
        EXTRA="SL${SL}_hcoll${HCOLL}"
        INNER_YLIM="[0, 100]"
        INNER_POS="[0.2, 0.6, .3, .2]"
        TREND_LIMIT=Bandwidth:0
        # AR
        INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB,8GiB"
        TESTNAME="allsizes_ar_SL${SL}_hcoll${HCOLL}"
        ./plots/plot_inputs.py -s ${SYSTEM} -vn gpubench-ar-nccl -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}"
        # A2A
        INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB"
        TESTNAME="allsizes_a2a_SL${SL}_hcoll${HCOLL}"
        INNER_YLIM="[0, 10000]"
        TREND_LIMIT=Bandwidth:0
        ./plots/plot_inputs.py -s ${SYSTEM} -vn gpubench-a2a-nccl -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT}
    done
done
exit 0

########################
# Service levels tests #
########################
OUT_PATH="plots/out/${SYSTEM}/two-nodes/"
PLOT_TYPE="box,line"
EXTRAS=diff_group_RC_SL0,diff_group_UC_SL0,diff_group_RC_SL1,diff_group_UC_SL1
for BENCH in "ib_send_lat"
do
    for SIZE in "1B" "8B" "64B" "512B" "4KiB" "32KiB" "256KiB" "2MiB" "16MiB" "128MiB" "1GiB"
    do
        TESTNAME=sl_${BENCH}_${SIZE}
        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${SIZE} -n 2 -am l -sp 50:50 --metrics "Runtime" -o ${OUT_PATH}/${TESTNAME} --ppn 1 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
    done
done


#####################
# Single-node tests #
#####################
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB,8GiB"
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
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
for PPN in 4
do
    # P2P
    INNER_YLIM="[0, 30]"
    INNER_POS="[0.2, 0.6, .3, .2]"
    TREND_LIMIT=Bandwidth:400
    TESTNAME="allsizes_pp_PPN${PPN}"
    ./plots/plot_inputs.py -s ${SYSTEM} -vn gpubench-mpp-nccl,gpubench-mpp-cudaaware,pw-ping-pong_b,ib_send_lat -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT} --inner_pos "${INNER_POS}"
    # AR
    TESTNAME="allsizes_ar_PPN${PPN}"
    ./plots/plot_inputs.py -s ${SYSTEM} -vn gpubench-ar-nccl,gpubench-ar-cudaaware,ardc_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE}
    # A2A
    TESTNAME="allsizes_a2a_PPN${PPN}"
    INNER_YLIM="[0, 30]"
    TREND_LIMIT=Bandwidth:400
    ./plots/plot_inputs.py -s ${SYSTEM} -vn gpubench-a2a-nccl,gpubench-a2a-cudaaware,a2a_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT}
done

#######################
# IB transports tests #
#######################
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
OUT_PATH="plots/out/${SYSTEM}/two-nodes/"
for TRANSPORT in "RC" "UC"
do
    TESTNAME="ib_transports_${TRANSPORT}"
    EXTRA="diff_group_${TRANSPORT}_SL0"
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ib_send_lat -vi ${INPUTS} -n 2 -am l -sp 50:50 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 1 -e ${EXTRA} --plot_types ${PLOT_TYPE}
done

##################
# Distance tests #
##################
#OUT_PATH="plots/out/${SYSTEM}/two-nodes/"
#PLOT_TYPE="box,violin"
#for SL in 0 1
#do
#    EXTRAS=same_switch_SL${SL},diff_switch_SL${SL},diff_group_SL${SL}
#    # Buffer on CPU memory
#    # PPN=1 for latency, pingpong
#    TESTNAME="latency_dist_SL${SL}"
#    ./plots/plot_extras.py -s ${SYSTEM} -vn "ping-pong_b" -vi 1B -n 2 -am l -sp 100 --metrics "Runtime" -o ${OUT_PATH}/${TESTNAME} --ppn 1 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
#    # Buffer on GPU memory
#    # PPN=4 for bandwidth
#    TESTNAME="bandwidth_dist_SL${SL}"
#    ./plots/plot_extras.py -s ${SYSTEM} -vn "gpubench-mpp-nccl" -vi 1GiB -n 2 -am l -sp 100 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
#done

########################
# Distance tests (IBV) #
########################
OUT_PATH="plots/out/${SYSTEM}/two-nodes/"
PLOT_TYPE="box,violin"
for SL in 1
do
    EXTRAS=same_switch_SL${SL},diff_switch_SL${SL},diff_group_SL${SL}
    # Buffer on CPU memory
    # PPN=1 for latency, pingpong
    TESTNAME="latency_dist_ib_SL${SL}"
    TITLE=""
    XTICKLABELS="[\"Same Switch\", \"Different Switch\", \"Different Group\"]"
    ./plots/plot_extras.py -s ${SYSTEM} -vn "ib_send_lat" -vi 1B -n 2 -am l -sp 50:50 --metrics "Runtime" -o ${OUT_PATH}/${TESTNAME} --ppn 1 -e ${EXTRAS} --plot_types ${PLOT_TYPE} --title "${TITLE}" --xticklabels "${XTICKLABELS}"
    # Buffer on GPU memory
    # PPN=4 for bandwidth
    TESTNAME="bandwidth_dist_ib_SL${SL}"
    ./plots/plot_extras.py -s ${SYSTEM} -vn "ib_send_lat" -vi 1GiB -n 2 -am l -sp 50:50 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 1 -e ${EXTRAS} --plot_types ${PLOT_TYPE} --title "${TITLE}" --xticklabels "${XTICKLABELS}"
done



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