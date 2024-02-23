#!/bin/bash

# Point-to-point
./run.sh test_suites/pingpong auto -n 2 -sp 100 --timeout 60 -am l

# Intra-node nccl-tests
# salloc -p boost_usr_prod -N 1 -n 4 --gres=gpu:4 --time=00:30:00 --exclusive
## Point to point
./run.sh test_suites/nccl_sendrecv auto -n 1 -sp 100 --timeout 60 -am l --ppn 2
## Collectives
./run.sh test_suites/nccl_coll auto -n 1 -sp 100 --timeout 60 -am l --ppn 4