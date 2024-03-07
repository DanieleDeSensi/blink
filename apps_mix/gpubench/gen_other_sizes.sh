
base_size="pp-baseline/1B"
dim=("B" "KiB" "MiB" "GiB")

for j in {0..3}
do
    for i in {0..9}
    do
        if [[ "$j" != "0" || "$i" != "0" ]]
        then
            if [[ "$j" != "3" || "$i" -lt "2" ]]
            then
                let quantity=2**$i
                dimension=${dim[$j]}
                let x=$j*10+$i


                echo "Dimensions: $quantity$dimension --> -x $x [(i,j) = ($i, $j)]"
                echo "-------------------"
                other_size_file=$( echo "$base_size" | sed "s/1B/$quantity$dimension/g" )
                echo "$base_size --> $other_size_file"
                contenent=$(cat "$base_size")
                newcontenent=$(echo "$contenent" | sed "s/-x 0/-x $x/g")
                echo "~~~~~~~~~~~~"
                echo "$base_size"
                echo "$contenent"
                echo "~~~~~~~~~~~~"
                echo "$other_size_file:"
                echo "$newcontenent"
                echo "$newcontenent" > "$other_size_file"
                echo "-------------------"
            fi
        fi
    done
done
