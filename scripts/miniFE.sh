#!/bin/bash

for nam in l r i
do
	# baseline
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_20_null_dummy node_files/SF_all_nodes_ordered -am "$nam" -as 10:90 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_100_null_dummy node_files/SF_all_nodes_ordered -am "$nam" -as 50:50 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_180_null_dummy node_files/SF_all_nodes_ordered -am "$nam" -as 90:10 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
	# inc
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_20_inc_b_128KiB node_files/SF_all_nodes_ordered -am "$nam" -as 10:90 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_100_inc_b_128KiB node_files/SF_all_nodes_ordered -am "$nam" -as 50:50 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_180_inc_b_128KiB node_files/SF_all_nodes_ordered -am "$nam" -as 90:10 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
	# a2a
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_20_a2a_b_128KiB node_files/SF_all_nodes_ordered -am "$nam" -as 10:90 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_100_a2a_b_128KiB node_files/SF_all_nodes_ordered -am "$nam" -as 50:50 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
	python3 runner.py wl_manager/MPI_wlm.py ./schedule_files/real_apps/miniFE_180_a2a_b_128KiB node_files/SF_all_nodes_ordered -am "$nam" -as 90:10 -mn 10 -mx 100 -t 400 -a 0.05 -b 0.05 -of csv -ro +file -p 1
done
