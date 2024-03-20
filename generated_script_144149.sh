#!/bin/bash
for schedule in ./apps_mix/gpubench/pp-baseline/1B ./apps_mix/gpubench/pp-baseline/4B ./apps_mix/gpubench/pp-baseline/8B ./apps_mix/gpubench/pp-baseline/64B ./apps_mix/gpubench/pp-baseline/512B ./apps_mix/gpubench/pp-baseline/4KiB ./apps_mix/gpubench/pp-baseline/32KiB ./apps_mix/gpubench/pp-baseline/256KiB ./apps_mix/gpubench/pp-baseline/2MiB ./apps_mix/gpubench/pp-baseline/16MiB ./apps_mix/gpubench/pp-baseline/128MiB ./apps_mix/gpubench/pp-baseline/1GiB ./apps_mix/gpubench/pp-nccl/1B ./apps_mix/gpubench/pp-nccl/4B ./apps_mix/gpubench/pp-nccl/8B ./apps_mix/gpubench/pp-nccl/64B ./apps_mix/gpubench/pp-nccl/512B ./apps_mix/gpubench/pp-nccl/4KiB ./apps_mix/gpubench/pp-nccl/32KiB ./apps_mix/gpubench/pp-nccl/256KiB ./apps_mix/gpubench/pp-nccl/2MiB ./apps_mix/gpubench/pp-nccl/16MiB ./apps_mix/gpubench/pp-nccl/128MiB ./apps_mix/gpubench/pp-nccl/1GiB ./apps_mix/gpubench/pp-nvlink/1B ./apps_mix/gpubench/pp-nvlink/4B ./apps_mix/gpubench/pp-nvlink/8B ./apps_mix/gpubench/pp-nvlink/64B ./apps_mix/gpubench/pp-nvlink/512B ./apps_mix/gpubench/pp-nvlink/4KiB ./apps_mix/gpubench/pp-nvlink/32KiB ./apps_mix/gpubench/pp-nvlink/256KiB ./apps_mix/gpubench/pp-nvlink/2MiB ./apps_mix/gpubench/pp-nvlink/16MiB ./apps_mix/gpubench/pp-nvlink/128MiB ./apps_mix/gpubench/pp-nvlink/1GiB
do
	for nam in l
	do
		for split in 100
		do
		python3 runner.py "$schedule" auto -am "$nam" -as "$split" -n 1 -mn 1 -mx 100 -t 600 -a 0.05 -b 0.05 -of csv -ro +file -p 2
		done
	done
done