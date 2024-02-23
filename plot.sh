#!/bin/bash
# Point-to-point - Same switch
./plotter.py -s leonardo -v ping-pong_b:1B:8B:64B:512B:4KiB:32KiB:256KiB:2MiB:16MiB:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s,bw -o plots/leonardo/pingpong -e same_switch


# NCCL - Point-to-point 
NCCL_INPUTS="4B:8B:64B:512B:4KiB:32KiB:256KiB:2MiB:16MiB:128MiB:1GiB"
./plotter.py -s leonardo -v nccl-sendrecv:${NCCL_INPUTS} -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/leonardo/nccl-sendrecv --ppn 2 --trend_limit 0_busbw-ip_GB/s:76

# NCCL - Collectives
./plotter.py -s leonardo -v nccl-allreduce:${NCCL_INPUTS}:8GiB -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/leonardo/nccl-allreduce --ppn 4 --trend_limit 0_busbw-ip_GB/s:230
./plotter.py -s leonardo -v nccl-alltoall:${NCCL_INPUTS}:8GiB -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/leonardo/nccl-alltoall --ppn 4 --trend_limit 0_busbw-ip_GB/s:230
