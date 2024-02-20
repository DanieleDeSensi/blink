#!/bin/bash
SYSTEM="local"

export BLINK_ROOT=$(pwd)

if [ -f conf/${SYSTEM}.sh ]; then
    source conf/${SYSTEM}.sh
    return 0
else
    echo "Unknown SYSTEM "$SYSTEM
    exit 1
fi
