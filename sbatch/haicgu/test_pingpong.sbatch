#!/bin/bash

#SBATCH --output=ping-pong_b_iter5_msgsize10000.out
#SBATCH --error=ping-pong_b_iter5_msgsize10000.err
#SBATCH --partition=cn-eth 

#SBATCH --nodes=2 
#SBATCH --ntasks=2 
#SBATCH --cpus-per-task=1

srun ../../src/microbench/bin/ping-pong_b -iter 5 -msgsize 10000
