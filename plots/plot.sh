#!/bin/bash

##################
#### Leonardo ####
##################
SYSTEM="leonardo"
# Compare nccl-allreduce and gpubench-allreduce-nccl
NCCL_INPUTS="8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
./plots/plot_inputs.py -s ${SYSTEM} -e diff_groups_AR1 -vn nccl-allreduce,gpubench-ar-nccl -vi ${NCCL_INPUTS},8GiB -n 2 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/out/${SYSTEM}/nccl-allreduce --ppn 4 --trend_limit 0_busbw-ip_GB/s:230
