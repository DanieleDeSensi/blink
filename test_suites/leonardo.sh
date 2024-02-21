#!/bin/bash

# Point-to-point
./run.sh test_suites/pingpong auto -n 2 -sp 100 --timeout 60 -am l
#./plotter.py -s local -v ping-pong_b:1B,ping-pong_b:8B,ping-pong_b:64B,ping-pong_b:512B,ping-pong_b:4KiB,ping-pong_b:32KiB,ping-pong_b:256KiB,ping-pong_b:2MiB,ping-pong_b:16MiB,ping-pong_b:128MiB -n 2 -am l -sp 100 --metrics 0_MainRank-Duration_s -o plots/local/pingpong
./run.sh test_suites/nccl_sendrecv auto -n 2 -sp 100 --timeout 60 -am l
#