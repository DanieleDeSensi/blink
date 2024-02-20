#!/bin/bash
export BLINK_SYSTEM="local"
export BLINK_ROOT=$(pwd)

if [ -f conf/${BLINK_SYSTEM}.sh ]; then
    source conf/${BLINK_SYSTEM}.sh
    return 0
else
    echo "Unknown SYSTEM "$BLINK_SYSTEM
    exit 1
fi
