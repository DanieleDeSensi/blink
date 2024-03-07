
base_str="pp-baseline"
other_strs=("pp-cudaaware" "pp-nccl" "pp-nvlink")

for other_str in "${other_strs[@]}"
do
    echo "OTHER_STR: $other_str"
    mkdir -p $other_str

    for base_str_file in "$base_str"/*
    do
        echo "-------------------"
        other_str_file=$( echo "$base_str_file" | sed "s/$base_str/$other_str/g" )
        echo "$base_str_file --> $other_str_file"
        contenent=$(cat "$base_str_file")
        newcontenent=$(echo "$contenent" | sed "s/-$base_str/-$other_str/g")
        echo "~~~~~~~~~~~~"
        echo "$base_str_file:"
        echo "$contenent"
        echo "~~~~~~~~~~~~"
        echo "$other_str_file:"
        echo "$newcontenent"
        echo "$newcontenent" > "$other_str_file"
    done
    echo "-------------------"
done
