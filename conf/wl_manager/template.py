class wl_manager:
    # Generates a script that can be used to run all the benchmarks specified in the schedule.
    def write_script(self, wlm_path, runner_args, schedules, nams, name, splits, node_file, ppn):
        pass

    # Returns a string that can be used to run command 'cmd'
    # on the nodes in 'node_list' with 'ppn' processes per node.
    def run_job(self, node_list, ppn, cmd):
        pass
        

