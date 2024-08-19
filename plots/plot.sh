#!/bin/bash

filter_fields() {
	path=$( echo "$1" | awk -F, '{ print $1 }' )
	sys=$(  echo "$1" | awk -F, '{ print $2 }' )
	nnod=$( echo "$1" | awk -F, '{ print $3 }' )
	ppn=$(  echo "$1" | awk -F, '{ print $6 }' )
	extra=$( echo "$1" | awk -F, '{ print $8 }' )
	fold=$( echo "$1" | awk -F, '{ print $9 }' )
	datapath="${fold}/data.csv"

	prefix="./apps_mix/"	
	tmp=${path#"$prefix"}
	exp=$(  echo "$tmp" | cut -d/ -f1 )
	size=$( echo "$tmp" | cut -d/ -f2 )
	
	echo -e "\tpath:  ${path}"
	echo -e "\t\texp:  ${exp}"
	echo -e "\t\tsize: ${size}"
	echo -e "\tsys:   ${sys}"
	echo -e "\tnnod:  ${nnod}"
	echo -e "\tppn:   ${ppn}"
	echo -e "\textra: ${extra}"
	echo -e "\tfold:  ${fold}"
	echo -e "\t\tdatapath: ${datapath}"
}

addToExpfile () {
	echo "$2, $3, $4" >> $1
}

mkdir -p "./plots"
[ "$#" -ge "1" ] && descriptionfile="$1" || descriptionfile="./data/description.csv"

i="0"
while IFS= read -r line; do
    if [[ "$i" != "0" ]]
    then
	echo "Text read from file: $line"
	filter_fields $line

	mkdir -p "./plots/${sys}"
	mkdir -p "./plots/${sys}/${exp}"
	plotpath="./plots/${sys}/${exp}"
	plottitle="${sys}-${exp}-${nnod}-${ppn}"
	filename="${plottitle}.csv"

	if [[ ! -f "${plotpath}/${filename}" ]]
	then
		echo "# ststem: ${sys}, experiment: ${exp}, nnodes: ${nnod}, process-per-node: ${ppn}" > "${plotpath}/${filename}"
		echo "# msg_size, extra, datafile" >> "${plotpath}/${filename}"
	fi

	echo -e "\t\tplotpath: ${plotpath}"
	echo -e "\t\tfilename: ${filename}"
	
	if [[ -f "${datapath}" ]]
	then
		addToExpfile "${plotpath}/${filename}" "${size}" "${extra}" "${datapath}"
	else
		echo "ERROR: ${datapath} does not exist"
	fi
    fi
    i=$(( i + 1))
done < "${descriptionfile}"

for f in plots/nanjin/*/*.csv
do
       grep "#" $f > tmp.txt ; grep -v "#" $f | sed "s/nanjin,/intra-switch,/g" | sed "s/nanjin2,/inter-switch,/g" >> tmp.txt
       mv tmp.txt ${f}
done
