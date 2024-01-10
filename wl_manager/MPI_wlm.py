class wl_manager:
    def write_script(self,wlm_path,runner_args,schedules,nams,name,splits,node_file,ppn):
        script=open(name+'.sh','w+')
        script.write('#!/bin/bash\nfor schedule in '+' '.join(schedules)+'\ndo\n')
        script.write('\tfor nam in '+' '.join(nams)+'\n\tdo\n')
        script.write('\t\tfor split in '+' '.join(splits)+'\n\t\tdo\n')
        script.write('\t\tpython3 runner.py '+wlm_path+' "$schedule" '+node_file+' -am "$nam" -as "$split"'+runner_args+' -p '+str(ppn))
        script.write('\n\t\tdone\n\tdone\ndone')
        script.close()

    def schedule_job(self,node_list,ppn,cmd):
        num_nodes=len(node_list)
        node_list_string=','.join(node_list)
        SF_string=('/scratch/2/t2hx/dep/openmpi/bin/mpirun -mca plm_rsh_no_tree_spawn 1'+
            ' --map-by node -mca btl openib,self,sm -mca btl_openib_if_include mlx4_0 -H '+node_list_string+
            ' -mca orte_base_help_aggregate 0  -np '+str(ppn*num_nodes)+' '+cmd)
        local_string='mpirun -host '+node_list_string+' -np '+str(ppn*num_nodes)+' --map-by node --oversubscribe '+cmd
        return SF_string
