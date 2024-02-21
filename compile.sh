#!/bin/bash
source conf.sh

GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NC=$(tput sgr0)

# Compile microbench
pushd src/microbench
    CC=$CC make
    if [ $? -ne 0 ]; then
        echo "${RED}[Error] internals compilation failed, please check error messages above.${NC}"
        exit 1
    fi
popd

# Download GPU microbench
if [ ! -d "src/microbench-gpu" ]; then
    git clone git@github.com:HicrestLaboratory/interconnect-banchmark.git src/microbench-gpu
    if [ $? -ne 0 ]; then
        echo "${RED}[Error] GPU microbench clone failed, please check error messages above.${NC}"
    fi
fi
# Compile GPU microbench
pushd src/microbench-gpu
    git checkout ${BLINK_GPU_MICROBENCH_COMMIT}
    MAKEFILE_NAME="Makefile.${BLINK_SYSTEM^^}"
    if [ -f ${MAKEFILE_NAME} ]; then
        make -f ${MAKEFILE_NAME}
        if [ $? -ne 0 ]; then
            echo "${RED}[Error] GPU microbench compilation failed, please check error messages above.${NC}"
            exit 1
        fi
    else
        echo "${RED}[Warning] GPU microbench not supported on this system (${BLINK_SYSTEM}).${NC}"
    fi
popd

# Compile netgauge
pushd src/netgauge-2.4.6
    if [ ! -f "Makefile" ]; then
        ./configure ${BLINK_NG_CONFIGURE_FLAGS}
        if [ $? -ne 0 ]; then
            echo "${RED}[Error] netgauge compilation failed, please check error messages above.${NC}"
            exit 1
        fi
    fi
    make
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
    ./make_script.sh all
    if [ $? -ne 0 ]; then
        echo "${RED}[Error] internals compilation failed, please check error messages above.${NC}"
        exit 1
    fi
popd

echo "${GREEN}Everything compiled successfully.${NC}"
