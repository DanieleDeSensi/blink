#!/usr/bin/python3

import subprocess
import datetime
import argparse
import os
import importlib.util
    
def main():

    parser=argparse.ArgumentParser(description='Driver to create script that runs runner.')
    parser.add_argument('wl_manager',help='Path to workload manager class.')
    parser.add_argument('schedule_file',help='Path to file that holds different schedules.')
    parser.add_argument('node_file',help='Path to node list file. If \'auto\' is specified, nodes are allocated automatically (it assumes Slurm is available).')
    parser.add_argument('-sn','--scriptname',help='Name of script',
                            default='script_'+datetime.datetime.now().strftime('%Y-%m-%d_%H:%M:%S'))
    parser.add_argument('-sp','--splits',help='List of splits, format: "AA:BB,CC:DD,EE:FF"',default='20:80,50:50,80:20')
    parser.add_argument('-am','--allocation_modes',help='List of allocation modes, format: "x,y,z"',default='l,r,i,+r')
    #runner arguments
    parser.add_argument('-mn','--minruns',help='Minimum number of runs.',default=1,type=int)
    parser.add_argument('-mx','--maxruns',help='Maximum number of runs.',default=100,type=int)
    parser.add_argument('-t','--timeout',help='Maximum duration of testing.',default=600,type=int)
    parser.add_argument('-a','--alpha',help='Confidence interval with 1-alpha.',default=0.05,type=float)
    parser.add_argument('-b','--beta',help='Congervence if mean reached beta of confidence interval.',default=0.05,type=float)
    parser.add_argument('-p','--ppn',help='Processes per node.',default=1,type=int)
    parser.add_argument('-ca','--convergeall',
                           help='Test until all metrics converged, if false until first does.',
                           action='store_true',default=False)
    parser.add_argument('-of','--outformat',help='Data output format (default: csv)',default='csv',choices=['csv','hdf'])
    parser.add_argument('-ro','--runtimeout',
                            help='Place where runtime feedback is printed (default: stdout, +file means to file and stdout),',
                            default='stdout',choices=['stdout','none','file','+file'])
    args=parser.parse_args()

    #driver args
    wlm_path=args.wl_manager
    schedule_file_path=args.schedule_file
    node_file=args.node_file
    name='./scripts/'+args.scriptname
    ppn=args.ppn

    if node_file == "auto":
        node_file = "node_files/auto_node_file.txt"
        subprocess.call(["scontrol", "show", "hostnames"], stdout=open(node_file, "w"))

    #runner args, non modified
    runner_args=(' -mn '+str(args.minruns)+' -mx '+str(args.maxruns)+' -t '+str(args.timeout)
            +' -a '+str(args.alpha)+' -b '+str(args.beta)+' -of '+args.outformat+' -ro '+args.runtimeout)
    if args.convergeall:
        runner_args=runner_args+' -ca'
    
    #list of args that scripts iterate over
    nams=args.allocation_modes.split(',')
    splits=args.splits.split(',')
    
    #read driver schedule file to get list of schedules
    with open(schedule_file_path,'r') as file:
        schedules=[]
        for line in file:
            schedules+=[line.strip()]
    if schedules==[]:
        raise Exception('Must at least pass one schedule to run.')

    #load wlm-class
    wlm_class_name=(wlm_path.split(os.sep)[-1])[:-3] #assumes name is last element of path without .py suffix
    spec_wlm=importlib.util.spec_from_file_location(wlm_class_name,wlm_path)
    mod_wlm=importlib.util.module_from_spec(spec_wlm)
    spec_wlm.loader.exec_module(mod_wlm)
    wlmanager=mod_wlm.wl_manager()
    
    #call write script function in wlm-class
    wlmanager.write_script(wlm_path,runner_args,schedules,nams,name,splits,node_file,ppn)
    
if __name__=='__main__':
    main()
