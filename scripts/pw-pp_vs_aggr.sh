#!/bin/bash
for schedule in ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_null_dummy ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_inc_b_128KiB ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_inc_nb_128KiB ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_inc_get_128KiB ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_inc_put_128KiB ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_inc_bsnbr_128KiB ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_a2a_b_128KiB ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_a2a_nb_128KiB ./schedule_files/pw-pp_vs_aggr/pw-ping-pong_b_128B_a2a_man_128KiB
do
	for nam in r
	do
		for split in 50:50
		do
		python3 runner.py wl_manager/MPI_wlm.py "$schedule" node_files/SF_all_nodes_ordered -am "$nam" -as "$split" -mn 3 -mx 10 -t 180 -a 0.05 -b 0.05 -of csv -ro +file -p 1
		done
	done
done
