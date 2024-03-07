base_file="gpubench-pp-baseline.py"

exps=("pp" "a2a" "ar" "hlo" "mpp")
Strs=("Baseline" "CudaAware" "Nccl" "Nvlink")
strs=("baseline" "cudaaware" "nccl" "nvlink")

cat "$base_file"
echo "========================="

for exp in "${exps[@]}"
do
    i=0
    for str in "${strs[@]}"
    do
        echo "i = $i --> ${Strs[i]}"
        if [[ "$exp" != "pp" || "$str" != "baseline" ]]
        then
            if [[ "${strs[i]}" != "nvlink" || "$exp" == "pp" || "$exp" == "a2a" ]]
            then
                new_file_name=$(echo "$base_file" | sed "s/-pp/-$exp/g" | sed "s/-baseline/-$str/g")
                echo "new_file_name: $new_file_name"
                echo "-----------------"
                new_contenent=$(cat $base_file | sed "s/pp_/"$exp"_/g" | sed "s/_Baseline/_${Strs[i]}/g" | sed "s/gpubench pp Baseline/gpubench $exp ${Strs[i]}/g")
                echo "$new_contenent"
                echo "$new_contenent" > "$new_file_name"
            fi
        fi
        i=$((i+1))
    done
done
