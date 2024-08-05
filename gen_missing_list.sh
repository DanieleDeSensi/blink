#!/bin/bash

expeted_sizes=("1B" "4B" "8B" "64B" "512B" "4KiB" "32KiB" "256KiB" "2MiB" "16MiB" "128MiB" "1GiB" "8GiB")
expectedextras=("cn-eth" "cn-ib")

for f in plots/haicgu/*/*.csv
do
	for extra in ${expectedextras[@]}
	do

		echo "$f:"
		grep -v "#" $f | grep "${extra}" > tmp.txt
		found_flag="0"
	        for e in $( awk -F, '{ print $1 }' tmp.txt )
        	do
			echo "e: ${e}"
	        done

		missing_exps=()
		for s in ${expeted_sizes[@]}
		do 
			found_flag="0"
			for e in $( awk -F, '{ print $1 }' tmp.txt )
			do 
				if [[ "$e" == "${s}" ]]
				then 
					found_flag="1"
				fi
			done
		
			if [[ "${found_flag}" == "0" ]]
			then 
				echo -e "Error: size $s is missing!"
				missing_exps+=("$s")
			fi
		done

		expname=$( basename -- $f | cut -d. -f1 )
		echo "For experiment ${expname} with extra ${extra} are missing experiments: ${missing_exps[*]}"
		if [[ "$#" -gt "0" ]]
		then
			echo "For experiment ${expname} with extra ${extra} are missing experiments: ${missing_exps[*]}" >> $1
		fi
	done
done
