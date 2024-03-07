exps=("pp" "a2a" "ar" "hlo" "mpp")
strs=("baseline" "cudaaware" "nccl" "nvlink")
dim=("B" "KiB" "MiB" "GiB")

outfile="gpubench"

echo "" > $outfile
for exp in "${exps[@]}"
do
    echo "EXP: $exp"

    for str in "${strs[@]}"
    do
        echo "EXP: $exp STR: $str"

        if [[ "$str" != "nvlink" || "$exp" == "pp" || "$exp" == "a2a" ]]
        then
            for j in {0..3}
            do
                for i in {0..9}
                do
                    if [[ "$j" != "3" || "$i" -lt "2" ]]
                    then
                        let quantity=2**$i
                        dimension=${dim[$j]}
                        echo "EXP: $exp OTHER_STR: $str Dimensions: $quantity$dimension"

                        string="./apps_mix/gpubench/$exp-$str/$quantity$dimension"
                        echo "$string"
                        echo "$string" >> $outfile
                    fi
                done
            done
        fi
    done
done
