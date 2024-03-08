#!/bin/bash

##################
#### Leonardo ####
##################
SYSTEM="leonardo"
# Compare nccl-allreduce and gpubench-ar-nccl
NCCL_INPUTS="8B,64B,512B,4KiB,32KiB,256KiB,2MiB,16MiB,128MiB,1GiB"
#for TESTNAME in nccl-allreduce nccl-alltoall nccl-sendrecv
#do
#    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME} -vi ${NCCL_INPUTS} -n 1 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_algbw-ip_GB/s,0_time-ip_us -o plots/out/${SYSTEM}/${TESTNAME} --ppn 4 --trend_limit 0_busbw-ip_GB/s:230
#done

for TESTNAME in gpubench-pp
do
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware,${TESTNAME}-nvlink -vi ${NCCL_INPUTS} -n 1 -am l -sp 100 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o plots/out/${SYSTEM}/${TESTNAME} --ppn 2 --trend_limit 0_busbw-ip_GB/s:230
done

for TESTNAME in gpubench-ar gpubench-a2a gpubench-hlo
do
    ./plots/plot_inputs.py -s ${SYSTEM} -vn ${TESTNAME}-nccl,${TESTNAME}-baseline,${TESTNAME}-cudaaware,${TESTNAME}-nvlink -vi ${NCCL_INPUTS} -n 1 -am l -sp 100 --metrics "0_Transfer Time_s,0_Bandwidth_GB/s" -o plots/out/${SYSTEM}/${TESTNAME} --ppn 4 --trend_limit 0_busbw-ip_GB/s:230
done

'''
# Routing tests
for COLL in nccl-allreduce nccl-alltoall
do
    for SIZE in 8B 1GiB
    do
        TESTNAME=${COLL}-routing-${SIZE}
        ./plots/plot_extras.py -s ${SYSTEM} -e diff_groups_AR1,diff_groups_AR0 -vn ${COLL} -vi ${SIZE} -n 2 -am l -sp 100 --metrics 0_busbw-ip_GB/s,0_time-ip_us -o plots/out/${SYSTEM}/${TESTNAME} --ppn 4
    done
done

for COLL in gpubench-ar-nccl
do
    for SIZE in 1GiB
    do
        TESTNAME=${COLL}-routing-${SIZE}
        ./plots/plot_extras.py -s ${SYSTEM} -e diff_groups_AR1,diff_groups_AR0 -vn ${COLL} -vi ${SIZE} -n 2 -am l -sp 100 --metrics "0_Bandwidth (GB/s)_GB/s" -o plots/out/${SYSTEM}/${TESTNAME} --ppn 1
    done
done
'''