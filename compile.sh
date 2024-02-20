#!/bin/bash
source conf.sh

GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NC=$(tput sgr0)

pushd src/microbench
    CC=$CC make
    if [ ! -f "bin/a2a_b" ]; then
        echo "${RED}[Error] internals compilation failed, please check error messages above.${NC}"
        exit 1
    fi
popd

# Compile netgauge
pushd src/netgauge-2.4.6
    ./configure ${BLINK_NG_CONFIGURE_FLAGS}
    make
    if [ ! -f "netgauge" ]; then
        echo "${RED}[Error] netgauge compilation failed, please check error messages above.${NC}"
        exit 1
    fi

    HAS_MPI=$(cat config.h | grep NG_MPI | cut -d ' ' -f 3)
    if [ "$HAS_MPI" != 1 ] ; then
        echo "${RED}[Error] netgauge did not find MPI. Please specify MPI path in ./configure${NC}"
        exit 1
    fi
popd

echo "${GREEN}Everything compiled successfully.${NC}"
