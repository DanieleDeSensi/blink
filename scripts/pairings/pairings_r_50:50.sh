#!/bin/bash
for schedule in ./schedule_files/pairings/a2a_b_128B_null_dummy ./schedule_files/pairings/a2a_b_128B_inc_b_128KiB ./schedule_files/pairings/a2a_b_128B_a2a_b_128KiB ./schedule_files/pairings/a2a_b_16KiB_null_dummy ./schedule_files/pairings/a2a_b_16KiB_inc_b_128KiB ./schedule_files/pairings/a2a_b_16KiB_a2a_b_128KiB ./schedule_files/pairings/a2a_b_1MiB_null_dummy ./schedule_files/pairings/a2a_b_1MiB_inc_b_128KiB ./schedule_files/pairings/a2a_b_1MiB_a2a_b_128KiB ./schedule_files/pairings/barrier_none_null_dummy ./schedule_files/pairings/barrier_none_inc_b_128KiB ./schedule_files/pairings/barrier_none_a2a_b_128KiB ./schedule_files/pairings/pw-ping-pong_b_128B_null_dummy ./schedule_files/pairings/pw-ping-pong_b_128B_inc_b_128KiB ./schedule_files/pairings/pw-ping-pong_b_128B_a2a_b_128KiB ./schedule_files/pairings/pw-ping-pong_b_16KiB_null_dummy ./schedule_files/pairings/pw-ping-pong_b_16KiB_inc_b_128KiB ./schedule_files/pairings/pw-ping-pong_b_16KiB_a2a_b_128KiB ./schedule_files/pairings/pw-ping-pong_b_1MiB_null_dummy ./schedule_files/pairings/pw-ping-pong_b_1MiB_inc_b_128KiB ./schedule_files/pairings/pw-ping-pong_b_1MiB_a2a_b_128KiB ./schedule_files/pairings/ring_bsnbr_128B_null_dummy ./schedule_files/pairings/ring_bsnbr_128B_inc_b_128KiB ./schedule_files/pairings/ring_bsnbr_128B_a2a_b_128KiB ./schedule_files/pairings/ring_bsnbr_16KiB_null_dummy ./schedule_files/pairings/ring_bsnbr_16KiB_inc_b_128KiB ./schedule_files/pairings/ring_bsnbr_16KiB_a2a_b_128KiB ./schedule_files/pairings/ring_bsnbr_1MiB_null_dummy ./schedule_files/pairings/ring_bsnbr_1MiB_inc_b_128KiB ./schedule_files/pairings/ring_bsnbr_1MiB_a2a_b_128KiB ./schedule_files/pairings/bdc_b_128B_null_dummy ./schedule_files/pairings/bdc_b_128B_inc_b_128KiB ./schedule_files/pairings/bdc_b_128B_a2a_b_128KiB ./schedule_files/pairings/bdc_b_16KiB_null_dummy ./schedule_files/pairings/bdc_b_16KiB_inc_b_128KiB ./schedule_files/pairings/bdc_b_16KiB_a2a_b_128KiB ./schedule_files/pairings/bdc_b_1MiB_null_dummy ./schedule_files/pairings/bdc_b_1MiB_inc_b_128KiB ./schedule_files/pairings/bdc_b_1MiB_a2a_b_128KiB ./schedule_files/pairings/ardc_b_128B_null_dummy ./schedule_files/pairings/ardc_b_128B_inc_b_128KiB ./schedule_files/pairings/ardc_b_128B_a2a_b_128KiB ./schedule_files/pairings/ardc_b_16KiB_null_dummy ./schedule_files/pairings/ardc_b_16KiB_inc_b_128KiB ./schedule_files/pairings/ardc_b_16KiB_a2a_b_128KiB ./schedule_files/pairings/ardc_b_1MiB_null_dummy ./schedule_files/pairings/ardc_b_1MiB_inc_b_128KiB ./schedule_files/pairings/ardc_b_1MiB_a2a_b_128KiB
do
	for nam in r
	do
		for split in 50:50
		do
		python3 runner.py wl_manager/MPI_wlm.py "$schedule" node_files/SF_all_nodes_ordered -am "$nam" -as "$split" -mn 3 -mx 5 -t 180 -a 0.05 -b 0.05 -of csv -ro +file -p 1
		done
	done
done
