#!/bin/bash
source conf.sh
SCRIPT_NAME=generated_script_$$.sh
rm -f ${SCRIPT_NAME}
./generate_scripts.py $@ --scriptname ${SCRIPT_NAME}
if [ -f ${SCRIPT_NAME} ]; then
    bash -x ${SCRIPT_NAME}
fi
rm ${SCRIPT_NAME}
