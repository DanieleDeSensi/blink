#!/bin/bash
#####################
# Single-node tests #
#####################
for SYSTEM in "alps" "leonardo" "lumi"
do
    INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
    OUT_PATH="plots/out/single_node/${SYSTEM}"
    PLOT_TYPE="line"
    for TESTNAME in "gpubench-pp" "gpubench-a2a" "gpubench-ar"
    do
        VICTIM_NAMES="${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware,${TESTNAME}-nvlink"
        LABELS="NCCL,Host Mem. Staging,CUDA-Aware,CUDA IPC"
        if [ ${TESTNAME} == "gpubench-pp" ]; then
            PPN=2
            if [ ${SYSTEM} == "lumi" ]; then
                TREND_LIMIT=Bandwidth:400  
            elif [ ${SYSTEM} == "leonardo" ]; then
                TREND_LIMIT=Bandwidth:800
            else # Alps
                TREND_LIMIT=Bandwidth:1200
            fi
        fi
        if [ ${TESTNAME} == "gpubench-a2a" ]; then
            if [ ${SYSTEM} == "lumi" ]; then
                TREND_LIMIT=Bandwidth:2400
                PPN=8
            elif [ ${SYSTEM} == "alps" ]; then
                TREND_LIMIT=Bandwidth:3600
                PPN=4
            else
                TREND_LIMIT=Bandwidth:2400
                PPN=4
            fi
        fi
        if [ ${TESTNAME} == "gpubench-ar" ]; then
            if [ ${SYSTEM} == "lumi" ]; then
                TREND_LIMIT=Bandwidth:2400
                PPN=8
            elif [ ${SYSTEM} == "alps" ]; then
                TREND_LIMIT=Bandwidth:3600
                PPN=4
            else
                TREND_LIMIT=Bandwidth:2400
                PPN=4
            fi
        fi
        INNER_YLIM="[0, 30]"
        ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn "${VICTIM_NAMES}" -vi ${INPUTS} -n 1 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} --trend_limit ${TREND_LIMIT} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --labels "${LABELS}"
    done
done

####################
# Pingpong 2 Nodes #
####################
EXTRA="#same_switch"
PLOT_TYPE="line"
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
BENCH_NAMES="gpubench-mpp-nccl,gpubench-mpp-cudaaware,pw-ping-pong_b,ib_send_lat"
LABELS="*CCL,GPU-Aware,MPI (host mem. buffers),IB Verbs"
PPN=DEFAULT_MULTINODE
for SYSTEM in "lumi" "leonardo" "alps"
do
    OUT_PATH="plots/out/two-nodes/pingpong/${SYSTEM}"
    # P2P
    INNER_YLIM="[0, 30]"
    INNER_POS="[0.2, 0.6, .3, .2]"
    if [ ${SYSTEM} == "lumi" ]; then
        TREND_LIMIT=Bandwidth:800
    fi
    if [ ${SYSTEM} == "leonardo" ]; then
        TREND_LIMIT=Bandwidth:400
    fi
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ${BENCH_NAMES} -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT} --inner_pos "${INNER_POS}" --labels "${LABELS}"
done

exit 0

###################
# 256 nodes tests #
###################
NUMNODES=256
SYSTEM="lumi"
OUT_PATH="plots/out/${NUMNODES}-nodes/${SYSTEM}"
PLOT_TYPE="line,box"
INNER_YLIM="[50, 150]"
INNER_POS="[0.2, 0.6, .3, .2]"
TREND_LIMIT=Bandwidth:0
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
TESTNAME="allsizes"
./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn gpubench-a2a-nccl,gpubench-ar-nccl,a2a_b,ardc_b -vi ${INPUTS} -n ${NUMNODES} -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn DEFAULT_MULTINODE --plot_types "${PLOT_TYPE}" #--inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT}


#################
# 8 nodes tests #
#################
SYSTEM="lumi"
OUT_PATH="plots/out/8-nodes/${SYSTEM}"
PLOT_TYPE="line,box"
INNER_YLIM="[50, 150]"
INNER_POS="[0.2, 0.6, .3, .2]"
TREND_LIMIT=Bandwidth:0
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
TESTNAME="allsizes"
./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn gpubench-a2a-nccl,gpubench-ar-nccl,a2a_b,ardc_b -vi ${INPUTS} -n 8 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn DEFAULT_MULTINODE --plot_types "${PLOT_TYPE}" #--inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT}





#########################################
# Multi nodes tests - Coll. scalability #
#########################################
SYSTEM="leonardo"
OUT_PATH="plots/out/multi-nodes/${SYSTEM}"
PLOT_TYPE="line,box,bar"
EXTRA="SL0_hcoll0,SL1_hcoll0"
NNODES="8,16,32,64,128,256"
for BENCH in "ar" "a2a"
do
    if [ ${BENCH} == "ar" ]; then
        declare -a INPUTS=("1B" "8B" "64B" "512B" "4KiB" "32KiB" "256KiB" "2MiB" "16MiB" "128MiB" "1GiB" "8GiB")
        FULLNAME="Allreduce"
    else
        declare -a INPUTS=("1B" "8B" "64B" "512B" "4KiB" "32KiB" "256KiB" "2MiB" "16MiB")
        FULLNAME="Alltoall"
    fi

    for INPUT in "${INPUTS[@]}"
    do        
        TESTNAME="${BENCH}"_${INPUT}
        ./plots/plot_inputs_multinodes.py -s ${SYSTEM} -vn gpubench-${BENCH}-nccl -vi ${INPUT} -n ${NNODES} -am "l" -sp 100 --metric "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE}
    done
done



#######################################
# Service levels + IB Transport tests #
#######################################
#SYSTEM="leonardo"
#OUT_PATH="plots/out/two-nodes/sl/${SYSTEM}"
#PLOT_TYPE="box,line,violin"
#EXTRAS=diff_group_RC_SL0,diff_group_UC_SL0,diff_group_RC_SL1,diff_group_UC_SL1
#for BENCH in "ib_send_lat"
#do
#    for SIZE in "1B" "8B" "64B" "512B" "4KiB" "32KiB" "256KiB" "2MiB" "16MiB" "128MiB" "1GiB"
#    do
#        TESTNAME=sl_${BENCH}_${SIZE}
#        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${SIZE} -n 2 -am l -sp 100 --metrics "Runtime" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
#    done
#done

##########################
# 64 nodes tests - HCOLL #
##########################
#SYSTEM="leonardo"
#OUT_PATH="plots/out/64-nodes/hcoll/${SYSTEM}"
#PLOT_TYPE="line,box"
#PPN=4
#SL=1 #TODO Redo for SL=0!
#EXTRA="SL${SL}_hcoll0,SL${SL}_hcoll1"
#INNER_YLIM="[50, 150]"
#INNER_POS="[0.2, 0.6, .3, .2]"
#TREND_LIMIT=Bandwidth:0
## AR
#INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB,8GiB"
#TESTNAME="ar"
#./plots/plot_inputs_multiextras.py -s ${SYSTEM} -vn ardc_b -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}"
## A2A
#INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB"
#TESTNAME="a2a"
#INNER_YLIM="[0, 50]"
#TREND_LIMIT=Bandwidth:0
#./plots/plot_inputs_multiextras.py -s ${SYSTEM} -vn a2a_b -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT}

###############################
# 64 nodes tests - SL1 - Cong #
###############################
SYSTEM="leonardo"
OUT_PATH="plots/out/64-nodes/cong/${SYSTEM}"
PLOT_TYPE="box"
EXTRA="SL0,SL1"
for BENCH in "ar" "a2a"
do
    if [ ${BENCH} == "ar" ]; then
        declare -a INPUTS=("1B" "1GiB")
        FULLNAME="Allreduce"
    else
        declare -a INPUTS=("1B" "16MiB")
        FULLNAME="Alltoall"
    fi
    XTICKLABELS="[\"${FULLNAME}\nIsolated\", \"${FULLNAME}\n+Alltoall\", \"${FULLNAME}\n+Incast\"]"
    for INPUT in "${INPUTS[@]}"
    do        
        TESTNAME="${BENCH}"_${INPUT}
        ./plots/plot_va.py -s ${SYSTEM} -vn gpubench-${BENCH}-nccl -vi ${INPUT} --aggressor_names ",a2a_b,inc_b" -n 64 -am "l,r" -sp 50:50 --metric "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --xticklabels "${XTICKLABELS}"
    done
done


#######################
# 64 nodes tests - SL #
#######################
SYSTEM="leonardo"
OUT_PATH="plots/out/64-nodes/sl/${SYSTEM}"
PLOT_TYPE="line,box"

EXTRA="SL0_hcoll0,SL1_hcoll0"
INNER_YLIM="[50, 150]"
INNER_POS="[0.2, 0.6, .3, .2]"
TREND_LIMIT=Bandwidth:0
# AR
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB,8GiB"
TESTNAME="allsizes_ar_SL${SL}"
./plots/plot_inputs_multiextras.py -s ${SYSTEM} -vn gpubench-ar-nccl -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}"
# A2A
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB"
TESTNAME="allsizes_a2a_SL${SL}"
INNER_YLIM="[0, 50]"
TREND_LIMIT=Bandwidth:0
./plots/plot_inputs_multiextras.py -s ${SYSTEM} -vn gpubench-a2a-nccl -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT}

exit 0

##################
# Service levels #
##################
SYSTEM="leonardo"
OUT_PATH="plots/out/two-nodes/sl/${SYSTEM}"
PLOT_TYPE="box,line,violin,boxnofliers"
EXTRAS=#same_switch#SL0,#same_switch#SL1,#diff_switch#SL0,#diff_switch#SL1,#diff_group#SL0,#diff_group#SL1
for SIZE in "1B" "1GiB"
do
    BENCH="ib_send_lat" #"gpubench-mpp-nccl" #"ib_send_lat"
    TESTNAME=${BENCH}_${SIZE}
    ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${SIZE} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} --plot_types ${PLOT_TYPE}
done

##################
# Distance tests #
##################
SYSTEMS="lumi,leonardo"
OUT_PATH="plots/out/two-nodes/distance"
PLOT_TYPE="box,boxnofliers,violin,dist"
for SL in 1
do
    EXTRAS=#same_switch,#diff_switch,#diff_group
    VICTIM_NAME="#distance-cpu"
    INPUT="#"
    XTICKLABELS="[\"Same Switch\", \"Different Switch\", \"Different Group\"]"
    MAX_Y="leonardo|Latency:6,lumi|Latency:6"
    ./plots/plot_extras.py -s ${SYSTEMS} -vn ${VICTIM_NAME} -vi ${INPUT} -n 2 -am l -sp 100 --metrics "Latency,Bandwidth" -o ${OUT_PATH} --ppn 4 -e ${EXTRAS} --plot_types ${PLOT_TYPE} --max_y "${MAX_Y}" #--xticklabels "${XTICKLABELS}"    
done


#######################
# IB transports tests #
#######################
#SYSTEM="leonardo"
#INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
#OUT_PATH="plots/out/two-nodes/ibtransport/${SYSTEM}"
#for TRANSPORT in "RC" "UC"
#do
#    EXTRA="diff_group_${TRANSPORT}_SL0"
#    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ib_send_lat -vi ${INPUTS} -n 2 -am l -sp 50:50 --metrics "Bandwidth" -o ${OUT_PATH}/${RC} --ppn 1 -e ${EXTRA} --plot_types ${PLOT_TYPE}
#done

exit 0


##################################
# Two-nodes tests -- collectives #
##################################
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
    # AR
    TESTNAME="allsizes_ar_PPN${PPN}"
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn gpubench-ar-nccl,gpubench-ar-cudaaware,ardc_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE}
    # A2A
    TESTNAME="allsizes_a2a_PPN${PPN}"
    INNER_YLIM="[0, 30]"
    TREND_LIMIT=Bandwidth:400
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn gpubench-a2a-nccl,gpubench-a2a-cudaaware,a2a_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT}
done





exit 0
# Old tests
INPUTS="8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
OUT_PATH="plots/out/${SYSTEM}/single_node"

#NCCL Tests - Point-to-Point
for TESTNAME in nccl-sendrecv
do
    TREND_LIMIT=Bandwidth:80
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${INPUTS} -n 1 -am l -sp 100 --metrics Bandwidth -o ${OUT_PATH}/${TESTNAME} --ppn 2 --trend_limit ${TREND_LIMIT}
done

#NCCL Tests - Collectives
for TESTNAME in nccl-allreduce nccl-alltoall
do
    TREND_LIMIT=Bandwidth:230
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${INPUTS} -n 1 -am l -sp 100 --metrics Bandwidth -o ${OUT_PATH}/${TESTNAME} --ppn 4 --trend_limit ${TREND_LIMIT}
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