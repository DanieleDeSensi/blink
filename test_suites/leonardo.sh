#!/bin/bash

# Point-to-point
./run.sh test_suites/pingpong auto -n 2 -sp 100 --timeout 60 -am l
#./plotter.py -s local -v ping-pong_b:1B,ping-pong_b:8B,ping-pong_b:64B,ping-pong_b:512B,ping-pong_b:4KiB,ping-pong_b:32KiB,ping-pong_b:256KiB,ping-pong_b:2MiB,ping-pong_b:16MiB,ping-pong_b:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s -o plots/leonardo/pingpong

# Intra-node nccl-tests
## Point to point
./run.sh test_suites/nccl_sendrecv auto -n 1 -sp 100 --timeout 60 -am l --ppn 2
#./plotter.py -s local -v nccl-sendrecv:4B,nccl-sendrecv:8B,nccl-sendrecv:64B,nccl-sendrecv:512B,nccl-sendrecv:4KiB,nccl-sendrecv:32KiB,nccl-sendrecv:256KiB,nccl-sendrecv:2MiB,nccl-sendrecv:16MiB,nccl-sendrecv:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s -o plots/leonardo/nccl-sendrecv
## Collectives
# salloc -p boost_usr_prod -N 1 -n 4 --gres=gpu:4 --time=00:30:00
./run.sh test_suites/nccl_coll auto -n 1 -sp 100 --timeout 60 -am l --ppn 4