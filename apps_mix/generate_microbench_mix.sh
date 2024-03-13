#!/bin/bash
# Generates other microbenchmarks mix starting from those defined in the pingpong folder
# Takes only one argument: the name of the microbenchmark to generate
mkdir -p $1
for f in $(ls pingpong); do
    echo "Generating $1-$f..."
    cat pingpong/$f | sed "s/ping-pong_b/$1/g" > $1/$f
done