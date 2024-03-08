#!/bin/bash
export BLINK_SYSTEM="leonardo"
export BLINK_ROOT=$(pwd)/
export BLINK_GPU_MICROBENCH_COMMIT="f979a93"
export BLINK_NCCL_TESTS_COMMIT="c6afef0"
export BLINK_DNN_PROXIES_COMMIT="1d32dce"

if [ -f conf/${BLINK_SYSTEM}.sh ]; then
    source conf/${BLINK_SYSTEM}.sh
    return 0
else
    echo "Unknown SYSTEM "$BLINK_SYSTEM
    exit 1
fi
