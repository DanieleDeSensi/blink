#!/bin/bash
IB_DEVICES=$1 # e.g. "mlx5_0|mlx5_1"
ARGS=$2
for DEV in $(echo $IB_DEVICES | sed "s/|/ /g")
do  
    OUT_FILE=ib_send_bw_${DEV}.json
    (rm -rf ib_send_bw_*.json; ib_send_bw --perform_warm_up --ib-dev=${DEV} --out_json --out_json_file=${OUT_FILE} --report_gbits -F ${ARGS}) &
done
wait