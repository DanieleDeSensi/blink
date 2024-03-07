
base_exp="pp"
other_exps=("a2a" "ar" "hlo")

for other_exp in "${other_exps[@]}"
do
    echo "OTHER_EXP: $other_exp"
    for base_exp_dir in "$base_exp"-*
    do
        other_exp_dir=$(echo "$base_exp_dir" | sed "s/$base_exp/$other_exp/g")
        echo "===================="
        echo "$base_exp_dir --> $other_exp_dir"
        mkdir -p "$other_exp_dir"

        for base_exp_file in "$base_exp_dir"/*
        do
            echo "-------------------"
            other_exp_file=$( echo "$base_exp_file" | sed "s/$base_exp/$other_exp/g" )
            echo "$base_exp_file --> $other_exp_file"
            contenent=$(cat "$base_exp_file")
            newcontenent=$(echo "$contenent" | sed "s/-$base_exp-/-$other_exp-/g")
            echo "~~~~~~~~~~~~"
            echo "$base_exp_file:"
            echo "$contenent"
            echo "~~~~~~~~~~~~"
            echo "$other_exp_file:"
            echo "$newcontenent"
            echo "$newcontenent" > "$other_exp_file"
        done
        echo "-------------------"
    done
    echo "===================="
done
