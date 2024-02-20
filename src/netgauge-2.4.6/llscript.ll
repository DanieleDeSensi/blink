#@ job_type = parallel
#@ environment = COPY_ALL
#@ tasks_per_node = 1
#@ node = 1
#@ wall_clock_limit = 0:05:00
##@ network.MPI_LAPI = sn_all,not_shared,US


#@ queue

