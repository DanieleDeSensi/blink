#!/bin/bash
# Point-to-point - Same switch
./plotter.py -s leonardo -v ping-pong_b:1B,ping-pong_b:8B,ping-pong_b:64B,ping-pong_b:512B,ping-pong_b:4KiB,ping-pong_b:32KiB,ping-pong_b:256KiB,ping-pong_b:2MiB,ping-pong_b:16MiB,ping-pong_b:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s -o plots/leonardo/pingpong -e same_switch


# NCCL - Point-to-point 
./plotter.py -s leonardo -v nccl-sendrecv:4B,nccl-sendrecv:8B,nccl-sendrecv:64B,nccl-sendrecv:512B,nccl-sendrecv:4KiB,nccl-sendrecv:32KiB,nccl-sendrecv:256KiB,nccl-sendrecv:2MiB,nccl-sendrecv:16MiB,nccl-sendrecv:128MiB,nccl-sendrecv:1GiB -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/leonardo/nccl-sendrecv --ppn 2

# NCCL - Collectives
./plotter.py -s leonardo -v nccl-allreduce:4B,nccl-allreduce:8B,nccl-allreduce:64B,nccl-allreduce:512B,nccl-allreduce:4KiB,nccl-allreduce:32KiB,nccl-allreduce:256KiB,nccl-allreduce:2MiB,nccl-allreduce:16MiB,nccl-allreduce:128MiB,nccl-allreduce:1GiB,nccl-allreduce:8GiB -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/leonardo/nccl-allreduce --ppn 4
./plotter.py -s leonardo -v nccl-alltoall:4B,nccl-alltoall:8B,nccl-alltoall:64B,nccl-alltoall:512B,nccl-alltoall:4KiB,nccl-alltoall:32KiB,nccl-alltoall:256KiB,nccl-alltoall:2MiB,nccl-alltoall:16MiB,nccl-alltoall:128MiB,nccl-alltoall:1GiB,nccl-allreduce:8GiB -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/leonardo/nccl-alltoall --ppn 4
