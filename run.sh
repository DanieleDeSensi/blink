#!/bin/bash
source conf.sh
SCRIPT_NAME=generated_script.sh
rm -f ${SCRIPT_NAME}
./generate_scripts.py $@
if [ -f ${SCRIPT_NAME} ]; then
    bash -x ${SCRIPT_NAME}
fi