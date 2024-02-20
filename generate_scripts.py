#!/usr/bin/python3
import subprocess
import datetime
import argparse
import os
import importlib.util

def main():
    parser = argparse.ArgumentParser(
        description='Generates the script to be used to run the experiments (called \'generated_script.sh\').')
    parser.add_argument(
        'test_bench', help='The file specifying which apps mix to run.')
    parser.add_argument(
        'node_file', help='Path to node list file. If \'auto\' is specified, nodes are allocated automatically (it assumes Slurm is available).')
    #parser.add_argument('-sn', '--scriptname', help='Name of script',
    #                    default='script_'+datetime.datetime.now().strftime('%Y-%m-%d_%H:%M:%S'))
    parser.add_argument(
        '-sp', '--splits', help='List of splits, format: "AA:BB,CC:DD,EE:FF"', default='20:80,50:50,80:20')
    parser.add_argument('-am', '--allocation_modes',
                        help='List of allocation modes, format: "x,y,z"', default='l,r,i')
    # runner arguments
    parser.add_argument('-mn', '--minruns',
                        help='Minimum number of runs.', default=1, type=int)
    parser.add_argument('-mx', '--maxruns',
                        help='Maximum number of runs.', default=100, type=int)
    parser.add_argument(
        '-t', '--timeout', help='Maximum duration of testing.', default=600, type=int)
    parser.add_argument(
        '-a', '--alpha', help='Confidence interval with 1-alpha.', default=0.05, type=float)
    parser.add_argument(
        '-b', '--beta', help='Congervence if mean reached beta of confidence interval.', default=0.05, type=float)
    parser.add_argument(
        '-p', '--ppn', help='Processes per node.', default=1, type=int)
    parser.add_argument('-ca', '--convergeall',
                        help='Test until all metrics converged, if false until first does.',
                        action='store_true', default=False)
    parser.add_argument('-of', '--outformat', help='Data output format (default: csv)',
                        default='csv', choices=['csv', 'hdf'])
    parser.add_argument('-ro', '--runtimeout',
                        help='Place where runtime feedback is printed (default: stdout, +file means to file and stdout),',
                        default='stdout', choices=['stdout', 'none', 'file', '+file'])
    parser.add_argument('-n', '--numnodes', help='Number of nodes on which to run the applications. It must be smaller or equal than the number of nodes specified in node_file',
                         type=int, required=True)
    parser.add_argument('-e', '--extrainfo', help='Extra info specifying details of this specific execution (will be stored in the description.csv file)', type=str)
    args = parser.parse_args()

    wlm_path="./conf/wl_manager/" + os.environ["BLINK_WL_MANAGER"] + ".py"
    test_bench_path = args.test_bench
    node_file = args.node_file
    #name = './scripts/' + args.scriptname + '.sh'
    name = "generated_script.sh"
    ppn = args.ppn

    if node_file == "auto":
        node_file = "node_files/auto_node_file.txt"
        subprocess.call(["scontrol", "show", "hostnames"],
                        stdout=open(node_file, "w"))

    # runner args, non modified
    extra = ""
    if args.extrainfo:
        extra = " -e " + args.extrainfo
    runner_args = (' -n ' + str(args.numnodes) + extra + ' -mn '+str(args.minruns)+' -mx '+str(args.maxruns)+' -t '+str(args.timeout)
                   + ' -a '+str(args.alpha)+' -b '+str(args.beta)+' -of '+args.outformat+' -ro '+args.runtimeout)
    if args.convergeall:
        runner_args = runner_args+' -ca'

    # list of args that scripts iterate over
    nams = args.allocation_modes.split(',')
    splits = args.splits.split(',')

    # read schedule file to get list of schedules
    with open(test_bench_path, 'r') as file:
        schedules = []
        for line in file:
            schedules += [line.strip()]
    if schedules == []:
        raise Exception('Must at least pass one schedule to run.')

    # load wlm-class
    # assumes name is last element of path without .py suffix
    wlm_class_name = (wlm_path.split(os.sep)[-1])[:-3]
    spec_wlm = importlib.util.spec_from_file_location(wlm_class_name, wlm_path)
    mod_wlm = importlib.util.module_from_spec(spec_wlm)
    spec_wlm.loader.exec_module(mod_wlm)
    wlmanager = mod_wlm.wl_manager()

    # call write script function in wlm-class
    wlmanager.write_script(runner_args, schedules,
                           nams, name, splits, node_file, ppn)

if __name__ == '__main__':
    main()
