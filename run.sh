#!/bin/bash
source conf.sh
./generate_scripts.py $@
SCRIPT_NAME=generated_script.sh
bash -x ${SCRIPT_NAME}