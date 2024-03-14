#!/bin/bash
IB_DEVICES=$1 # e.g. "mlx5_0#mlx5_1"
ARGS=$2
START_PORT=18515
INDEX=0
for DEV in $(echo $IB_DEVICES | sed "s/#/ /g")
do
    CONN_PORT=$(( $START_PORT + $INDEX ))
    rm -rf ib_send_lat${INDEX}
    (ib_send_lat --perform_warm_up --ib-dev=${DEV} --report_gbits -t 1 -F -U -p ${CONN_PORT} ${ARGS} &> ib_send_lat${INDEX}) &
    INDEX=$(( $INDEX + 1 ))
done
wait
