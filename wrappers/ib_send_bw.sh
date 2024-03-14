#!/bin/bash
IB_DEVICES=$1 # e.g. "mlx5_0|mlx5_1"
ARGS=$2
START_PORT=18515
INDEX=0
for DEV in $(echo $IB_DEVICES | sed "s/|/ /g")
do
    CONN_PORT=$(( $START_PORT + $INDEX ))
    OUT_FILE=ib_send_bw_${DEV}.json
    (rm -rf ib_send_bw_*.json; ib_send_bw --perform_warm_up --ib-dev=${DEV} --out_json --out_json_file=${OUT_FILE} --report_gbits -F -p ${CONN_PORT} ${ARGS}) &
    INDEX=$(( $INDEX + 1 ))
done
wait
