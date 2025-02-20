#!/bin/bash
rm -rf plots/out/*
PLOT_SINGLE_NODE=1
PLOT_TWO_NODES=1
PLOT_DISTANCE=1
PLOT_COLL_SCALABILITY=1
PLOT_COLL_SCALABILITY_HEATMAP=1
PLOT_COLL_SCALABILITY_NOISE=1
PLOT_LUMI_GPU_PAIRS=1
PLOT_SL_CONG=1
PLOT_HCOLL=0
PLOT_HCOLL_NODES=0
PLOT_CONG_NODES=0
#ERRORBAR="(\"ci\", 90)"
ERRORBAR="(\"pi\", 50)"

#################################
# Single-node tests -- Fig. 1-3 #
#################################
if [[ $PLOT_SINGLE_NODE = 1 ]]; then
    for SYSTEM in "alps" "leonardo" "lumi"
    do
        INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
        OUT_PATH="plots/out/single_node/${SYSTEM}"
        PLOT_TYPE="line"
        for TESTNAME in "gpubench-pp" "gpubench-a2a" "gpubench-ar"
        do
            if [ ${SYSTEM} == "alps" ]; then
                VICTIM_NAMES="${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware"
                LABELS="*CCL,Trivial Staging,GPU-Aware MPI"
            else
                VICTIM_NAMES="${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware,${TESTNAME}-nvlink"
                LABELS="*CCL,Trivial Staging,GPU-Aware MPI,Device-Device Copy"
            fi
            EXTRA="NULL"
            if [ ${SYSTEM} == "lumi" ]; then
                EXTRA="0-1"            
            fi

            if [ ${TESTNAME} == "gpubench-pp" ]; then
                PPN=2
                if [ ${SYSTEM} == "lumi" ]; then
                    INNER_YLIM="[0, 30]"
                    TREND_LIMIT=Goodput:1600:Expected_Goodput,Goodput:168:Exp._Trivial_Gdpt
                elif [ ${SYSTEM} == "leonardo" ]; then
                    INNER_YLIM="[0, 30]"
                    TREND_LIMIT=Goodput:800:Expected_Goodput,Goodput:112:Exp._Trivial_Gdpt
                else # Alps
                    INNER_YLIM="[0, 30]"
                    TREND_LIMIT=Goodput:1200:Expected_Goodput,Goodput:168:Exp._Trivial_Gdpt
                fi
            fi
            if [ ${TESTNAME} == "gpubench-a2a" ]; then
                if [ ${SYSTEM} == "lumi" ]; then
                    INNER_YLIM="[0, 100]"
                    TREND_LIMIT=Goodput:600:Expected_Goodput
                    PPN=8
                elif [ ${SYSTEM} == "alps" ]; then
                    INNER_YLIM="[0, 100]"
                    TREND_LIMIT=Goodput:3600:Expected_Goodput
                    PPN=4
                else
                    INNER_YLIM="[0, 100]"                
                    TREND_LIMIT=Goodput:2400:Expected_Goodput
                    PPN=4
                fi
            fi
            if [ ${TESTNAME} == "gpubench-ar" ]; then
                if [ ${SYSTEM} == "lumi" ]; then
                    INNER_YLIM="[0, 100]"
                    TREND_LIMIT=Goodput:800:Expected_Goodput
                    PPN=8
                elif [ ${SYSTEM} == "alps" ]; then
                    INNER_YLIM="[0, 100]"
                    TREND_LIMIT=Goodput:3600:Expected_Goodput
                    PPN=4
                else
                    INNER_YLIM="[0, 100]"
                    TREND_LIMIT=Goodput:2400:Expected_Goodput
                    PPN=4
                fi
            fi    
            # [left, bottom, width, height]     
            INNER_POS="[0.21, 0.45, .35, .2625]"   
            ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn "${VICTIM_NAMES}" -vi ${INPUTS} -n 1 -am l -sp 100 --metrics "Goodput" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} --trend_limit ${TREND_LIMIT} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --labels "${LABELS}" -e "${EXTRA}" --errorbar "${ERRORBAR}" --inner_pos "${INNER_POS}" > /dev/null
        done
    done
fi

##############################
# Pingpong 2 Nodes -- Fig. 4 #
##############################
if [[ $PLOT_TWO_NODES = 1 ]]; then
    EXTRA="#same_switch"
    PLOT_TYPE="line"
    INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
    #BENCH_NAMES="gpubench-mpp-nccl,gpubench-mpp-cudaaware,pw-ping-pong_b,ib_send_lat"
    #LABELS="*CCL,GPU-Aware MPI,MPI (host mem. buffers),IB Verbs (host mem. buffers)"
    BENCH_NAMES="gpubench-mpp-nccl,gpubench-mpp-cudaaware,pw-ping-pong_b"
    LABELS="*CCL,GPU-Aware MPI,MPI (host mem. buffers)"
    PPN=DEFAULT_MULTINODE
    for SYSTEM in "lumi" "leonardo" "alps"
    do
        OUT_PATH="plots/out/two-nodes/pingpong/${SYSTEM}"
        # P2P
        INNER_YLIM="[0, 50]"
        # [left, bottom, width, height]     
        INNER_POS="[0.21, 0.55, .3, .2625]"   
        #INNER_POS="[0.2, 0.6, .3, .2]"
        if [ ${SYSTEM} == "alps" ]; then
            TREND_LIMIT=Goodput:800:Expected_Goodput
        fi
        if [ ${SYSTEM} == "lumi" ]; then
            TREND_LIMIT=Goodput:800:Expected_Goodput
        fi
        if [ ${SYSTEM} == "leonardo" ]; then
            TREND_LIMIT=Goodput:400:Expected_Goodput
        fi
        ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ${BENCH_NAMES} -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Goodput" -o ${OUT_PATH} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT} --inner_pos "${INNER_POS}" --labels "${LABELS}"  --errorbar "${ERRORBAR}" > /dev/null
    done
fi

############################
# Distance tests -- Fig. 5 #
############################
if [[ $PLOT_DISTANCE = 1 ]]; then
    PLOT_TYPE="box,boxnofliers,violin,dist"
    EXTRAS=#same_switch,#diff_switch,#diff_group
    INPUT="#"
    XTICKLABELS="[\"Same\nSwitch\", \"Diff.\nSwitch\", \"Diff.\nGroup\"]"
    for DIST in "distance-gpu" "distance-cpu"
    do        
        SYSTEMS="alps,leonardo,lumi"
        if [ ${DIST} = "distance-cpu" ]; then            
            MAX_Y="alps|Latency:6,leonardo|Latency:6,lumi|Latency:6"
        else
            MAX_Y="alps|Latency:40,leonardo|Latency:40,lumi|Latency:40"
        fi
        OUT_PATH="plots/out/two-nodes/${DIST}"        
        ./plots/plot_extras.py -s ${SYSTEMS} -vn "#${DIST}" -vi ${INPUT} -n 2 -am l -sp 100 --metrics "Latency,Goodput" -o ${OUT_PATH} --ppn DEFAULT_MULTINODE -e ${EXTRAS} --plot_types ${PLOT_TYPE} --xticklabels "${XTICKLABELS}" > /dev/null # --max_y "${MAX_Y}" 
    done
fi

if [[ $PLOT_COLL_SCALABILITY = 1 ]]; then
    #########################################
    # Multi nodes tests - Coll. scalability #
    #########################################
    SYSTEMS="alps,leonardo,lumi"
    OUT_PATH="plots/out/multi-nodes/"
    PLOT_TYPE="line,box,bar"
    EXTRA="#"
    NNODES="2,4,8,16,32,64,128,256,512"
    for BENCH in "ar" "a2a"
    do
        if [ ${BENCH} == "ar" ]; then
            #declare -a INPUTS=("1B" "8B" "64B" "512B" "4KiB" "32KiB" "256KiB" "2MiB" "16MiB" "128MiB" "1GiB")
            #declare -a INPUTS=("1B" "1GiB")
            declare -a INPUTS=("1GiB")
            FULLNAME="Allreduce"
        else
            #declare -a INPUTS=("1B" "8B" "64B" "512B" "4KiB" "32KiB" "256KiB" "2MiB" "16MiB")
            #declare -a INPUTS=("1B" "16MiB")
            declare -a INPUTS=("2MiB" "16MiB")
            FULLNAME="Alltoall"
        fi

        for INPUT in "${INPUTS[@]}"
        do  
            CPU_BENCH=""
            if [ ${BENCH} == "ar" ]; then
                CPU_BENCH="ardc_b"
                TREND_LIMIT_GPU="X:X:X"
                TREND_LIMIT_CPU="Goodput:100:Expected_Goodput_(Leonardo),Goodput:200:Expected_Goodput_(LUMI)"
            else
                CPU_BENCH="a2a_b"
                TREND_LIMIT_GPU="Goodput:100:Asymptotically_Expected_Goodput_(Leonardo_and_LUMI),Goodput:200:Asymptotically_Expected_Goodput_(Alps)"
                #TREND_LIMIT_GPU="AUTO"
                TREND_LIMIT_CPU="Goodput:100:Asymptotically_Expected_Goodput_(Leonardo),Goodput:200:Asymptotically_Expected_Goodput_(LUMI)"
            fi
            TESTNAME="${BENCH}"_gpu_${INPUT}            
            ./plots/plot_nodes_multisystem.py -s ${SYSTEMS} -vn gpubench-${BENCH}-nccl,gpubench-${BENCH}-cudaaware -vi ${INPUT} -n ${NNODES} -am "l" -sp 100 --metric "Goodput" -o ${OUT_PATH}/${TESTNAME} --ppn DEFAULT_MULTINODE --plot_types ${PLOT_TYPE} -e ${EXTRA} --errorbar "${ERRORBAR}" --trend_limit "${TREND_LIMIT_GPU}" > /dev/null #--bw_per_node #
            TESTNAME="${BENCH}"_cpu_${INPUT}
            ./plots/plot_nodes_multisystem.py -s ${SYSTEMS} -vn ${CPU_BENCH} -vi ${INPUT} -n ${NNODES} -am "l" -sp 100 --metric "Goodput" -o ${OUT_PATH}/${TESTNAME} --ppn DEFAULT_MULTINODE --plot_types ${PLOT_TYPE} -e ${EXTRA} --errorbar "${ERRORBAR}" --trend_limit "${TREND_LIMIT_CPU}" > /dev/null #--bw_per_node #
        done
    done
fi


if [[ $PLOT_COLL_SCALABILITY_NOISE = 1 ]]; then
    #########################################
    # Multi nodes tests - Coll. scalability #
    #########################################
    SYSTEM="leonardo"
    OUT_PATH="plots/out/multi-nodes/${SYSTEM}/noise"
    PLOT_TYPE="line,box,bar"
    EXTRA="SL0_hcoll0,SL1_hcoll0"
    NNODES="2,4,8,16,32,64,128,256"
    LABELS="Default Service Level, Non-Default Service Level"    
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
            ./plots/plot_inputs_multinodes.py -s ${SYSTEM} -vn gpubench-${BENCH}-nccl -vi ${INPUT} -n ${NNODES} -am "l" -sp 100 --metric "Goodput" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --labels "${LABELS}" --errorbar "${ERRORBAR}" --no_inner > /dev/null
        done
    done
fi

#################################################
# Multi nodes tests - Coll. scalability Heatmap #
#################################################
if [[ $PLOT_COLL_SCALABILITY_HEATMAP = 1 ]]; then
    PLOT_TYPE="line,box,bar"
    EXTRA="#"
    NNODES="2,4,8,16,32,64" #,128,256,512"
    for BENCH in "ar" "a2a"
    do
        if [ ${BENCH} == "ar" ]; then
            INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
            FULLNAME="Allreduce"
        else
            INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB"
            FULLNAME="Alltoall"
        fi

        for SYSTEM in "alps" "leonardo" "lumi"
        do  
            OUT_PATH="plots/out/multi-nodes/heatmap/${SYSTEM}/${BENCH}"
            CPU_BENCH=""
            CBARLABEL="*CCL/GPU-aware MPI Goodput"
            if [ ${BENCH} == "ar" ]; then
                TREND_LIMIT_GPU="X:X:X"
            else
                TREND_LIMIT_GPU="Bandwidth:100:Expected_Bandwidth_(Leonardo_and_LUMI)"
            fi
            TESTNAME="${BENCH}"_gpu_${SYSTEM}            
            ./plots/plot_heatmap_nodes_v_size.py -s ${SYSTEM} -vn gpubench-${BENCH} --victim_types nccl,cudaaware -vi ${INPUTS} -n ${NNODES} -am "l" -sp 100 --metric "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn DEFAULT_MULTINODE --plot_types ${PLOT_TYPE} -e ${EXTRA} --errorbar "${ERRORBAR}" --trend_limit "${TREND_LIMIT_GPU}" --cbarlabel "${CBARLABEL}" > /dev/null #--bw_per_node #
        done
    done
fi

##########################
# 64 nodes tests - HCOLL #
##########################
if [[ $PLOT_HCOLL = 1 ]]; then
    SYSTEM="leonardo"
    OUT_PATH="plots/out/multi-nodes/hcoll/${SYSTEM}"
    PLOT_TYPE="line,box"
    PPN=4
    SL=1 #TODO Redo for SL=0!
    EXTRA="SL${SL}_hcoll0,SL${SL}_hcoll1"
    INNER_YLIM="[50, 150]"
    INNER_POS="[0.2, 0.6, .3, .2]"
    TREND_LIMIT=Bandwidth:0
    # AR
    INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB,8GiB"
    TESTNAME="ar"
    ./plots/plot_inputs_multiextras.py -s ${SYSTEM} -vn ar-nccl -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" > /dev/null
    # A2A
    INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB"
    TESTNAME="a2a"
    INNER_YLIM="[0, 50]"
    TREND_LIMIT=Bandwidth:0
    ./plots/plot_inputs_multiextras.py -s ${SYSTEM} -vn a2a-nccl -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT} > /dev/null
fi

##########################
# 64 nodes tests - HCOLL #
##########################
if [[ $PLOT_HCOLL_NODES = 1 ]]; then
    SYSTEM="leonardo"
    OUT_PATH="plots/out/multi-nodes/hcoll_nodes/${SYSTEM}"
    PLOT_TYPE="line"
    PPN=4
    EXTRA="SL0_hcoll0,SL0_hcoll1,SL1_hcoll0,SL1_hcoll1"
    NODES="2,4,8,16,32,64,128,256"
    # AR
    INPUTS="1GiB"
    for TESTNAME in "ar-cudaaware" "ar-nccl"
    do
        ./plots/plot_extras_multinodes.py -s ${SYSTEM} -vn gpubench-${TESTNAME} -vi ${INPUTS} -n ${NODES} -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} > /dev/null
    done
    # A2A
    INPUTS="2MiB"
    for TESTNAME in "a2a-cudaaware" "a2a-nccl"
    do
        ./plots/plot_extras_multinodes.py -s ${SYSTEM} -vn gpubench-${TESTNAME} -vi ${INPUTS} -n ${NODES} -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} > /dev/null
    done
fi


##################
# LUMI GPU Pairs #
##################
if [[ $PLOT_LUMI_GPU_PAIRS = 1 ]]; then
    SYSTEM="lumi"
    PLOT_TYPE="line,box,bar"
    EXTRAS="0-1,0-2,0-3,0-4,0-5,0-6,0-7"
    INPUT="1GiB"    
    OUT_PATH="plots/out/single_node/gpupairs/${SYSTEM}"
    BENCHS="gpubench-pp-nccl,gpubench-pp-baseline,gpubench-pp-cudaaware,gpubench-pp-nvlink"
    LABELS="RCCL,Trivial Staging,GPU-Aware MPI,Device-Device Copy"
    BARPLOT_TOPS="1600,400,400,400,400,800,800"
    ./plots/plot_extras_multivictim.py -s ${SYSTEM} -vn "${BENCHS}" -vi ${INPUT} -n 1 -am l -sp 100 --metrics "Goodput" -o ${OUT_PATH} --ppn 2 -e ${EXTRAS} --plot_types ${PLOT_TYPE} --labels "${LABELS}" --barplot_tops "${BARPLOT_TOPS}" > /dev/null
fi

###################
# Cong vs. #nodes #
###################
if [[ $PLOT_CONG_NODES = 1 ]]; then
    for SYSTEM in "lumi" "leonardo"
    do
        OUT_PATH="plots/out/cong/${SYSTEM}"
        PLOT_TYPE="line,box"
        EXTRA="SL0_hcoll0"
        for BENCH in "gpubench-ar-nccl" "gpubench-a2a-nccl" "gpubench-a2a-cudaaware"
        do
            if [ ${BENCH} == "gpubench-ar-nccl" ] || [ ${BENCH} == "gpubench-ar-cudaaware" ]; then
                INPUT="1GiB"
                FULLNAME="Allreduce"
            else
                INPUT="2MiB"
                FULLNAME="Alltoall"
            fi
            if [ ${SYSTEM} == "lumi" ]; then
                PPN=8
                NUMNODES="4,8,16,32,64,128,256,512"
            else
                PPN=4
                NUMNODES="4,8,16,32,64,128"
            fi
            #LABELS="${FULLNAME}\nIsolated,${FULLNAME}\n+Alltoall,${FULLNAME}\n+Incast"
            TESTNAME="${BENCH}"
            ./plots/plot_agg_multinodes.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} --aggressor_names ",a2a_man,inc_b" --aggressor_inputs ",128KiB,128KiB" -n ${NUMNODES} -am "+r" -sp 50:50 --metric "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} > /dev/null #--labels "${LABELS}"
        done
    done
fi

###############################
# 64 nodes tests - SL1 - Cong #
###############################
if [[ $PLOT_SL_CONG = 1 ]]; then
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
            ./plots/plot_va.py -s ${SYSTEM} -vn gpubench-${BENCH}-nccl -vi ${INPUT} --aggressor_names ",a2a_b,inc_b" -n 64 -am "l,r" -sp 50:50 --metric "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --xticklabels "${XTICKLABELS}" > /dev/null
        done
    done
fi

##########
# Rename #
##########
mkdir -p plots/paper
rm -rf plots/paper/*
mkdir plots/paper/fig3/
for SYSTEM in "alps" "leonardo" "lumi"; do
    cp plots/out/single_node/${SYSTEM}/gpubench-pp/goodput_line.pdf plots/paper/fig3/${SYSTEM}.pdf
done

mkdir plots/paper/fig4/
cp plots/out/single_node/gpupairs/lumi/goodput_bar.pdf plots/paper/fig4/

mkdir plots/paper/fig5/
for SYSTEM in "alps" "leonardo" "lumi"; do
    cp plots/out/single_node/${SYSTEM}/gpubench-a2a/goodput_line.pdf plots/paper/fig5/${SYSTEM}.pdf
done

mkdir plots/paper/fig6/
for SYSTEM in "alps" "leonardo" "lumi"; do
    cp plots/out/single_node/${SYSTEM}/gpubench-ar/goodput_line.pdf plots/paper/fig6/${SYSTEM}.pdf
done

mkdir plots/paper/fig7/
for SYSTEM in "alps" "leonardo" "lumi"; do
    cp plots/out/two-nodes/pingpong/${SYSTEM}/goodput_line.pdf plots/paper/fig7/${SYSTEM}.pdf
done

mkdir plots/paper/fig8/
for DIST in "cpu" "gpu"; do
    cp plots/out/two-nodes/distance-${DIST}/boxnofliers.pdf plots/paper/fig8/${DIST}.pdf
done

mkdir plots/paper/fig9/
cp plots/out/multi-nodes/a2a_gpu_2mib/goodput_line_nol.pdf plots/paper/fig9/

mkdir plots/paper/fig10/
cp plots/out/multi-nodes/ar_gpu_1gib/goodput_line_nol.pdf plots/paper/fig10/

mkdir plots/paper/fig11/
for COLL in "ar" "a2a"; do
    cp plots/out/multi-nodes/heatmap/lumi/${COLL}/${COLL}_gpu_lumi/bandwidth_hm.pdf plots/paper/fig11/${COLL}.pdf
done

mkdir plots/paper/fig12/
cp plots/out/64-nodes/cong/leonardo/ar_1gib/box.pdf plots/paper/fig12/

mkdir plots/paper/fig13/
cp plots/out/multi-nodes/leonardo/noise/a2a_2mib/goodput_box.pdf plots/paper/fig13/a2a.pdf
cp plots/out/multi-nodes/leonardo/noise/ar_1gib/goodput_box.pdf plots/paper/fig13/ar.pdf

exit 0


############
# TC tests #
############
OUT_PATH="plots/out/lumi/two-nodes/tc"
EXTRAS=diff_group_TC_BE,diff_group_TC_LL
./plots/plot_extras.py -s lumi -vn ping-pong_b -vi "1B" -n 2 -am l -sp 100 --metrics "Runtime" -o ${OUT_PATH} --ppn 1 -e ${EXTRAS} > /dev/null

exit 0

#################
# 8 nodes tests #
#################
# Allreduce
NNODES=4
EXTRA=""
for SYSTEM in "alps"
do
    OUT_PATH="plots/out/${NNODES}-nodes/allreduce/${SYSTEM}"
    PLOT_TYPE="line,box"
    INNER_YLIM="[50, 150]"
    INNER_POS="[0.2, 0.6, .3, .2]"
    TREND_LIMIT=Bandwidth:200
    INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
    TESTNAME="allsizes"
    BENCHMARKS="gpubench-ar-nccl,gpubench-ar-cudaaware,ardc_b"
    LABELS="*CCL,GPU-Aware MPI,MPI - Host Mem."
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ${BENCHMARKS} -vi ${INPUTS} -n ${NNODES} -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn DEFAULT_MULTINODE --plot_types "${PLOT_TYPE}" --labels "${LABELS}" --trend_limit ${TREND_LIMIT} > /dev/null #-e ${EXTRA} #--inner_ylim "${INNER_YLIM}" 
done

# Alltoall
for SYSTEM in "alps"
do
    OUT_PATH="plots/out/${NNODES}-nodes/alltoall/${SYSTEM}"
    PLOT_TYPE="line,box"
    INNER_YLIM="[50, 150]"
    INNER_POS="[0.2, 0.6, .3, .2]"
    TREND_LIMIT=Bandwidth:200
    INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
    TESTNAME="allsizes"
    BENCHMARKS="gpubench-a2a-nccl,gpubench-a2a-cudaaware,a2a_b"
    LABELS="*CCL,GPU-Aware MPI,MPI - Host Mem."
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ${BENCHMARKS} -vi ${INPUTS} -n ${NNODES} -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn DEFAULT_MULTINODE --plot_types "${PLOT_TYPE}" --labels "${LABELS}" --trend_limit ${TREND_LIMIT} > /dev/null #-e ${EXTRA} #--inner_ylim "${INNER_YLIM}" 
done

exit 0

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
        for ALLOCATION in "r" "l" "i"
        do
            TESTNAME="${BENCH}"_${INPUT}_${ALLOCATION}
            ./plots/plot_va.py -s ${SYSTEM} -vn gpubench-${BENCH}-nccl -vi ${INPUT} --aggressor_names ",a2a_b,inc_b" -n 64 -am ${ALLOCATION} -sp 50:50 --metric "Goodput" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --xticklabels "${XTICKLABELS}" > /dev/null
        done
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
./plots/plot_inputs_multiextras.py -s ${SYSTEM} -vn gpubench-ar-nccl -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" > /dev/null
# A2A
INPUTS="1B,8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB"
TESTNAME="allsizes_a2a_SL${SL}"
INNER_YLIM="[0, 50]"
TREND_LIMIT=Bandwidth:0
./plots/plot_inputs_multiextras.py -s ${SYSTEM} -vn gpubench-a2a-nccl -vi ${INPUTS} -n 64 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT} > /dev/null

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
    ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${SIZE} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} --plot_types ${PLOT_TYPE} > /dev/null
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
#    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ib_send_lat -vi ${INPUTS} -n 2 -am l -sp 50:50 --metrics "Bandwidth" -o ${OUT_PATH}/${RC} --ppn 1 -e ${EXTRA} --plot_types ${PLOT_TYPE} > /dev/null
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
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn gpubench-ar-nccl,gpubench-ar-cudaaware,ardc_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} > /dev/null
    # A2A
    TESTNAME="allsizes_a2a_PPN${PPN}"
    INNER_YLIM="[0, 30]"
    TREND_LIMIT=Bandwidth:400
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn gpubench-a2a-nccl,gpubench-a2a-cudaaware,a2a_b -vi ${INPUTS} -n 2 -am l -sp 100 --metrics "Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn ${PPN} -e ${EXTRA} --plot_types ${PLOT_TYPE} --inner_ylim "${INNER_YLIM}" --trend_limit ${TREND_LIMIT} > /dev/null
done






exit 0
# Old tests
INPUTS="8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
OUT_PATH="plots/out/${SYSTEM}/single_node"

#NCCL Tests - Point-to-Point
for TESTNAME in nccl-sendrecv
do
    TREND_LIMIT=Bandwidth:80
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${INPUTS} -n 1 -am l -sp 100 --metrics Bandwidth -o ${OUT_PATH}/${TESTNAME} --ppn 2 --trend_limit ${TREND_LIMIT} > /dev/null
done

#NCCL Tests - Collectives
for TESTNAME in nccl-allreduce nccl-alltoall
do
    TREND_LIMIT=Bandwidth:230
    ./plots/plot_inputs_multivictim.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${INPUTS} -n 1 -am l -sp 100 --metrics Bandwidth -o ${OUT_PATH}/${TESTNAME} --ppn 4 --trend_limit ${TREND_LIMIT} > /dev/null
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
    ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 16 -am r -sp 10:90 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} > /dev/null
    for AGGRESSOR in a2a_b inc_b 
    do
        TESTNAME=${BENCH}_${INPUT}_${AGGRESSOR}
        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} --aggressor_name ${AGGRESSOR} --aggressor_input 128KiB -n 16 -am r -sp 10:90 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} > /dev/null
    done
done

EXTRAS=SL0,SL1
INPUT="1GiB"
AGGRESSOR="gpubench-a2a-nccl"
TESTNAME=${BENCH}_${INPUT}
./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 16 -am r -sp 10:90 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} > /dev/null
TESTNAME=${BENCH}_${INPUT}_${AGGRESSOR}
./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} --aggressor_name ${AGGRESSOR} --aggressor_input 128KiB -n 16 -am r -sp 10:90 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} > /dev/null

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
        ./plots/plot_extras.py -s ${SYSTEM} -vn ${BENCH} -vi ${INPUT} -n 2 -am l -sp 100 --metrics "Runtime,Bandwidth" -o ${OUT_PATH}/${TESTNAME} --ppn 4 -e ${EXTRAS} > /dev/null
    done
done

