#!/bin/bash
export BLINK_SYSTEM="haicgu"
export BLINK_ROOT=$(pwd)/
export BLINK_GPU_MICROBENCH_COMMIT="88e77f4"
export BLINK_NCCL_TESTS_COMMIT="128334135db5476a1553505ff30d26c9568df1ae"
export BLINK_DNN_PROXIES_COMMIT="128334135db5476a1553505ff30d26c9568df1ae"

if [ -f conf/${BLINK_SYSTEM}.sh ]; then
    source conf/${BLINK_SYSTEM}.sh
    return 0
else
    echo "Unknown SYSTEM "$BLINK_SYSTEM
    exit 1
fi
