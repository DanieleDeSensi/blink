#!/bin/bash
#SBATCH --nodes=2
#SBATCH --time=01:00:00
#SBATCH --exclusive
#SBATCH --job-name=blink-pingpong
source conf.sh
ARGS="/root/hpc/host/hostfile11_12 -n 2 -sp 100 -am l -ro +file -e nanjin  -mn 10" # -mn 10
# We run pingpong test with 1 PPN
./run.sh test_suites/cpu/pingpong ${ARGS} --ppn 1
