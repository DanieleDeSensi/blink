import os

class wl_manager:
    # Generates a script that can be used to run all the benchmarks specified in the schedule.
    def write_script(self, runner_args, schedules, nams, name, splits, node_file, ppn):
        script=open(name,'w+')
        script.write('#!/bin/bash\nfor schedule in '+' '.join(schedules)+'\ndo\n')
        script.write('\tfor nam in '+' '.join(nams)+'\n\tdo\n')
        script.write('\t\tfor split in '+' '.join(splits)+'\n\t\tdo\n')
        script.write('\t\tpython3 runner.py "$schedule" '+node_file+' -am "$nam" -as "$split"'+runner_args+' -p '+str(ppn))
        script.write('\n\t\tdone\n\tdone\ndone')
        script.close()

    # Returns a string that can be used to run command 'cmd'
    # on the nodes in 'node_list' with 'ppn' processes per node.
    def run_job(self, node_list, ppn, cmd):
        num_nodes=len(node_list)
        node_list_string=','.join(node_list)
        job_cmd = os.environ["BLINK_MPIRUN"] + " " + \
                  os.environ["BLINK_MPIRUN_MAP_BY_NODE_FLAG"] + " " + \
                  os.environ["BLINK_MPIRUN_ADDITIONAL_FLAGS"] + " " + \
                  os.environ["BLINK_PINNING_FLAGS"] + " " + \
                  os.environ["BLINK_MPIRUN_HOSTNAMES_FLAG"] + " " + node_list_string + " " + \
                  "-np " + str(ppn*num_nodes) + " " + cmd
        return job_cmd
