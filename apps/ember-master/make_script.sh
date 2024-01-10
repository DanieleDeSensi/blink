#!/bin/bash
if [ "$1" == "clean" ]; then
    cd ./mpi/halo3d
    make clean
    cd ../..

    cd ./mpi/halo3d-26
    make clean
    cd ../..

    cd ./mpi/incast
    make clean
    cd ../..

    cd ./mpi/pingpong
    make clean
    cd ../..

    cd ./mpi/sweep3d
    make clean
    cd ../..
fi

if [ "$1" == "all" ]; then
    cd ./mpi/halo3d
    make
    cd ../..

    cd ./mpi/halo3d-26
    make
    cd ../..

    cd ./mpi/incast
    make
    cd ../..

    cd ./mpi/pingpong
    make
    cd ../..

    cd ./mpi/sweep3d
    make
    cd ../..
fi
