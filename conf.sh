#!/bin/bash
SYSTEM="local"

if [ -f conf/${SYSTEM}.sh ]; then
    source conf/${SYSTEM}.sh
    return 0
else
    echo "Unknown SYSTEM "$SYSTEM
    exit 1
fi
