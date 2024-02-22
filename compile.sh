#!/bin/bash
source conf.sh

GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NC=$(tput sgr0)

# Compile microbench
pushd src/microbench
    mkdir -p bin
    CC=${BLINK_CC} make
    if [ $? -ne 0 ]; then
        echo "${RED}[Error] internals compilation failed, please check error messages above.${NC}"
        exit 1
    fi
popd

# Download dnn-proxies
if [ ! -d "src/dnn-proxies" ]; then
    git clone https://github.com/spcl/DNN-cpp-proxies src/dnn-proxies
    if [ $? -ne 0 ]; then
        echo "${RED}[Error] dnn-proxies clone failed, please check error messages above.${NC}"
    fi
fi
# Compile dnn-proxies
pushd src/dnn-proxies
    git checkout ${BLINK_DNN_PROXIES_COMMIT}
    mkdir -p bin
    for bench in "cosmoflow" "dlrm" "gpt3" "gpt3_moe" "resnet152_scal"; do
        ${BLINK_CXX} proxies/$bench.cpp -o bin/$bench
        if [ $? -ne 0 ]; then
            echo "${RED}[Error] dnn-proxies compilation failed, please check error messages above.${NC}"
            exit 1
        fi
    done
popd


if [ "$BLINK_GPU_BENCH" = "true" ]; then
    # Download GPU microbench
    if [ ! -d "src/microbench-gpu" ]; then
        git clone git@github.com:HicrestLaboratory/interconnect-benchmark.git src/microbench-gpu
        if [ $? -ne 0 ]; then
            echo "${RED}[Error] GPU microbench clone failed, please check error messages above.${NC}"
        fi
    fi
    # Compile GPU microbench
    pushd src/microbench-gpu
        git checkout ${BLINK_GPU_MICROBENCH_COMMIT}
        MAKEFILE_NAME="Makefile.${BLINK_SYSTEM^^}"
        if [ -f ${MAKEFILE_NAME} ]; then
            #CUDA_HOME=${BLINK_CUDA_HOME} MPI_HOME=${BLINK_MPI_HOME} MPI_CUDA_HOME=${BLINK_MPI_CUDA_HOME} NCCL_HOME=${BLINK_NCCL_HOME} make -f ${MAKEFILE_NAME}
	    make -f ${MAKEFILE_NAME}
            if [ $? -ne 0 ]; then
                echo "${RED}[Error] GPU microbench compilation failed, please check error messages above.${NC}"
                exit 1
            fi
        else
            echo "${RED}[Warning] GPU microbench not supported on this system (${BLINK_SYSTEM}).${NC}"
        fi
    popd

    # Download nccl-tests
    if [ ! -d "src/nccl-tests" ]; then
        git clone https://github.com/NVIDIA/nccl-tests src/nccl-tests
        if [ $? -ne 0 ]; then
            echo "${RED}[Error] nccl-tests clone failed, please check error messages above.${NC}"
        fi
    fi
    # Compile nccl-tests
    pushd src/nccl-tests
        git checkout ${BLINK_NCCL_TESTS_COMMIT}
        make MPI=1 NVCC=nvcc
        if [ $? -ne 0 ]; then
            echo "${RED}[Error] nccl-tests compilation failed, please check error messages above.${NC}"
            exit 1
        fi
    popd
fi

# Compile netgauge
pushd src/netgauge-2.4.6
    if [ ! -f "Makefile" ]; then
        CC=${BLINK_CC} ./configure ${BLINK_NG_CONFIGURE_FLAGS}
        if [ $? -ne 0 ]; then
            echo "${RED}[Error] netgauge compilation failed, please check error messages above.${NC}"
            exit 1
        fi
    fi
    CC=${BLINK_CC} make
    if [ $? -ne 0 ]; then
        echo "${RED}[Error] netgauge compilation failed, please check error messages above.${NC}"
        exit 1
    fi

    HAS_MPI=$(cat config.h | grep NG_MPI | cut -d ' ' -f 3)
    if [ "$HAS_MPI" != 1 ] ; then
        echo "${RED}[Error] netgauge did not find MPI. Please specify MPI path in ./configure${NC}"
        exit 1
    fi
popd

# Compile ember
pushd src/ember
    CC=${BLINK_CC} ./make_script.sh all
    if [ $? -ne 0 ]; then
        echo "${RED}[Error] internals compilation failed, please check error messages above.${NC}"
        exit 1
    fi
popd

echo "${GREEN}Everything compiled successfully.${NC}"
