#!/bin/bash

##################
#### Leonardo ####
##################
SYSTEM="leonardo"
# Point-to-point - Diff switches
# Trend
./plotter.py -s ${SYSTEM} -v ping-pong_b:1B:8B:64B:512B:4KiB:32KiB:256KiB:2MiB:16MiB:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s,0_MainRank-Bandwidth_Gb/s -o plots/${SYSTEM}/pingpong_diff_groups -e diff_groups
# Violin - 1B
./plotter.py -s ${SYSTEM} -v ping-pong_b:1B -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s,0_MainRank-Bandwidth_Gb/s -o plots/${SYSTEM}/pingpong_diff_groups_1B -e diff_groups
# Violin - 128MiB
./plotter.py -s ${SYSTEM} -v ping-pong_b:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s,0_MainRank-Bandwidth_Gb/s -o plots/${SYSTEM}/pingpong_diff_groups_128MiB -e diff_groups


# NCCL - Point-to-point 
NCCL_INPUTS="4B:8B:64B:512B:4KiB:32KiB:256KiB:2MiB:16MiB:128MiB:1GiB"
./plotter.py -s ${SYSTEM} -v nccl-sendrecv:${NCCL_INPUTS} -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/${SYSTEM}/nccl-sendrecv --ppn 2 --trend_limit 0_busbw-ip_GB/s:76

# NCCL - Collectives
./plotter.py -s ${SYSTEM} -v nccl-allreduce:${NCCL_INPUTS}:8GiB -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/${SYSTEM}/nccl-allreduce --ppn 4 --trend_limit 0_busbw-ip_GB/s:230
./plotter.py -s ${SYSTEM} -v nccl-alltoall:${NCCL_INPUTS}:8GiB -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/${SYSTEM}/nccl-alltoall --ppn 4 --trend_limit 0_busbw-ip_GB/s:230


##############
#### LUMI ####
##############
SYSTEM="lumi"
# Point-to-point - same blade
# Trend
./plotter.py -s ${SYSTEM} -v ping-pong_b:1B:8B:64B:512B:4KiB:32KiB:256KiB:2MiB:16MiB:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s,0_MainRank-Bandwidth_Gb/s -o plots/${SYSTEM}/pingpong_diff_groups -e same_blade
# Violin - 1B
./plotter.py -s ${SYSTEM} -v ping-pong_b:1B -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s,0_MainRank-Bandwidth_Gb/s -o plots/${SYSTEM}/pingpong_diff_groups_1B -e same_blade
# Violin - 128MiB
./plotter.py -s ${SYSTEM} -v ping-pong_b:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s,0_MainRank-Bandwidth_Gb/s -o plots/${SYSTEM}/pingpong_diff_groups_128MiB -e same_blade