#!/bin/bash
for schedule in ./apps_mix/a2a_b/1B ./apps_mix/a2a_b/4B ./apps_mix/a2a_b/8B ./apps_mix/a2a_b/64B ./apps_mix/a2a_b/512B ./apps_mix/a2a_b/4KiB ./apps_mix/a2a_b/32KiB ./apps_mix/a2a_b/256KiB ./apps_mix/a2a_b/2MiB ./apps_mix/a2a_b/16MiB ./apps_mix/a2a_b/128MiB ./apps_mix/ardc_b/4B ./apps_mix/ardc_b/8B ./apps_mix/ardc_b/64B ./apps_mix/ardc_b/512B ./apps_mix/ardc_b/4KiB ./apps_mix/ardc_b/32KiB ./apps_mix/ardc_b/256KiB ./apps_mix/ardc_b/2MiB ./apps_mix/ardc_b/16MiB ./apps_mix/ardc_b/128MiB ./apps_mix/agtr_b/4B ./apps_mix/agtr_b/8B ./apps_mix/agtr_b/64B ./apps_mix/agtr_b/512B ./apps_mix/agtr_b/4KiB ./apps_mix/agtr_b/32KiB ./apps_mix/agtr_b/256KiB ./apps_mix/agtr_b/2MiB ./apps_mix/agtr_b/16MiB ./apps_mix/agtr_b/128MiBa
do
	for nam in l
	do
		for split in 100
		do
		python3 runner.py "$schedule" auto -am "$nam" -as "$split" -n 4 -e boost_usr_prod -mn 1 -mx 10 -t 600 -a 0.05 -b 0.05 -of csv -ro +file -p 1
		done
	done
done