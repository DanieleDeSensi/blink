import subprocess
import numpy as np
import scipy.stats as st
import math
import time
import importlib.util
import pathlib
import sys
import os
import random
import argparse
import datetime
import pandas
import signal
import shlex


class data_container:
    def __init__(self, app_id, conv_goal, label, unit):
        self.app_id = app_id
        self.conv_run = 0
        self.label = label
        self.unit = unit
        self.conv_goal = conv_goal
        self.converged = False
        self.num_samples = []
        self.data = []

    def get_title(self):
        return str(self.app_id)+'_'+self.label+'_'+self.unit

    def md_to_list(self):
        return [self.app_id, self.label, self.unit, self.conv_goal, self.converged, self.conv_run]+self.num_samples


def check_CI(container_list, alpha, beta, converge_all, run):
    # confidence interval parameters: length of CI (two-tailed with 1-alpha confidence) is within beta of reported mean
    for container in container_list:
        # not converged yet and has to converge
        if (not container.converged) and (converge_all or container.conv_goal):
            n = len(container.data)
            if n <= 1:
                continue  # not converged
            mean = np.mean(container.data)
            sem = st.sem(container.data)
            if sem == 0:  # no variance in data
                container.converged = True  # converged
                container.conv_run = run
                continue
            CI_lb, CI_ub = st.t.interval(1-alpha, n-1, loc=mean, scale=sem)
            CI_length = CI_ub-CI_lb
            if CI_length < beta*mean:
                container.converged = True  # converged
                container.conv_run = run

    # check if all convergence flags
    check = True
    for container in container_list:
        if (converge_all or container.conv_goal):
            check = (check and container.converged)
    return check


def run_job(job, wlmanager, ppn):
    cmd_string = wlmanager.run_job(job.node_list, ppn, job.run_app())
    if not cmd_string or cmd_string == "":
        cmd_string = "echo a > /dev/null"
    cmd = shlex.split(cmd_string)
    process = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False)
    job.set_process(process)


def wait_timed(job, time):
    try:
        out, err = job.process.communicate(timeout=time)
        # if didn't time out
        job.set_output(out, err)
        return False
    except subprocess.TimeoutExpired:
        # if timed out
        end_job(job)
        return True


def end_job(job):
    # signal SIGUSR1 if ouput of endless is needed
    if job.collect_flag:
        job.process.terminate()
    else:
        job.process.kill()
    out, err = job.process.communicate()
    job.set_output(out, err)


def c_allocation(apps, nodes_frame):
    for app in apps:
        nodes_list = list(filter(lambda ele: ele != '',
                          nodes_frame[:][app.id_num].tolist()))
        app.set_nodes(nodes_list)


def l_allocation(apps, node_list, split_absolute):  # linear allocation
    i = 0
    for app, split in zip(apps, split_absolute):
        app.set_nodes(node_list[i:i+split])
        i += split


def i_allocation(apps, node_list, split_absolute):  # interleaved allocation
    num_apps = len(apps)
    ending_condition = [0]*num_apps
    alloc_lists = [[] for _ in range(num_apps)]
    s_index = 0
    n_index = 0
    while split_absolute != ending_condition:
        if split_absolute[s_index] != 0:
            alloc_lists[s_index] += [node_list[n_index]]
            split_absolute[s_index] -= 1
            n_index += 1
        s_index = (s_index+1) % num_apps
    for app, alloc_list in zip(apps, alloc_lists):
        app.set_nodes(alloc_list)


def get_abs_split(allocation_split, num_apps, num_nodes):
    if allocation_split == 'e':  # equal split among all apps
        split_list = [100/num_apps]*num_apps
    else:
        split_list = [float(x) for x in allocation_split.split(':')]

    if sum(split_list) > 100:
        raise Exception("Splits percentages mustn't add up to more than 100.")
    if len(split_list) != num_apps:
        raise Exception('Number of applications ('+str(num_apps)+') is not equal to number of splits ('
                        + str(len(split_list))+')')
    split_absolute = []
    for split in split_list[:-1]:
        split_absolute += [int(num_nodes*split/100)]
    # allocate all remaining nodes to the last application
    split_absolute += [num_nodes-sum(split_absolute)]
    return split_absolute


def log_meta_data(out_format, path, data_container_list, num_runs):
    num_samples_index = list(
        map(lambda x: 'Run '+str(x), list(range(1, num_runs+1))))
    dataframe = pandas.DataFrame()
    for i, container in enumerate(data_container_list):
        dataframe = pandas.concat([dataframe, pandas.DataFrame(
            container.md_to_list())], axis=1, ignore_index=True)
    dataframe.index = ['App_id', 'Label', 'Unit', 'Convergence_goal',
                       'Converged', 'Converged_run']+num_samples_index
    if out_format == 'csv':
        file_name = path+'.csv'
        dataframe.to_csv(file_name, index=True)
    elif out_format == 'hdf':
        file_name = path+'.h5'
        dataframe.to_hdf(file_name, key='df',
                         data_columns=data_header_cols, index=False)


def log_data(out_format, path, data_container_list):
    dataframe = pandas.DataFrame()
    columns = []
    for i, container in enumerate(data_container_list):
        dataframe = pandas.concat([dataframe, pandas.DataFrame(
            container.data)], axis=1, ignore_index=True)
        columns += [container.get_title()]
    dataframe.columns = columns
    if out_format == 'csv':
        file_name = path+'.csv'
        dataframe.to_csv(file_name, index=False)
    elif out_format == 'hdf':
        file_name = path+'.h5'
        dataframe.to_hdf(file_name, key='df',
                         data_columns=data_header_cols, index=False)


def print_runtime(obj, mode, ro_file):
    if mode == 'stdout':
        print(obj)
    elif mode == 'none':
        return
    elif mode == '+file':
        print(obj)
        print(obj, file=ro_file)
    elif mode == 'file':
        print(obj, file=ro_file)


def main():
    pre_start_time = time.time()
    parser = argparse.ArgumentParser(description='Runner of framework.')
    parser.add_argument('app_mix', help='The file specifying which apps mix to run.')
    parser.add_argument('node_file', help='Path to node list file. If \'auto\' is specified, nodes are allocated automatically (it assumes Slurm is available).')
    parser.add_argument('-n', '--numnodes', help='Number of nodes on which to run the applications. It must be smaller or equal than the number of nodes specified in node_file',
                         type=int, required=True)
    parser.add_argument('-am', '--allocationmode', help='Way of allocating nodes (default: linear)',
                        default='l', choices=['l', 'c', 'r', 'i', '+r'])
    parser.add_argument('-as', '--allocationsplit',
                        help='Percent allocated nodes per application, format: percent_0:percent_1:...:percent_k-1.', default='e')
    parser.add_argument('-mn', '--minruns',
                        help='Minimum number of runs.', default=10, type=int)
    parser.add_argument('-mx', '--maxruns',
                        help='Maximum number of runs.', default=1000, type=int)
    parser.add_argument(
        '-t', '--timeout', help='Maximum duration of testing.', default=100.0, type=float)
    parser.add_argument(
        '-a', '--alpha', help='Confidence interval with 1-alpha.', default=0.05, type=float)
    parser.add_argument(
        '-b', '--beta', help='Congervence if mean reached beta of confidence interval.', default=0.05, type=float)
    parser.add_argument(
        '-p', '--ppn', help='Processes per node.', default=1, type=int)
    parser.add_argument('-ca', '--convergeall',
                        help='Test until all metrics converged.', action='store_true', default=False)
    parser.add_argument('-of', '--outformat', help='Data output format (default: csv)',
                        default='csv', choices=['csv', 'hdf'])
    parser.add_argument('-ro', '--runtimeout', help='Place where runtime feedback is printed',
                        default='stdout', choices=['stdout', 'none', 'file', '+file'])
    parser.add_argument('-s', '--seed', help='Seed for randomness', default=1, type=int)
    parser.add_argument('-d', '--datapath', help='Path where data is written', default='./data')
    parser.add_argument('-e', '--extrainfo', help='Extra info specifying details of this specific execution (will be stored in the description.csv file)', type=str)
    args = parser.parse_args()

    # argument namespace to variables
    import_path_wlm = "./conf/wl_manager/" + \
        os.environ["BLINK_WL_MANAGER"] + ".py"
    app_mix_path = args.app_mix
    node_file = args.node_file
    allocation_mode = args.allocationmode
    allocation_split = args.allocationsplit
    out_format = args.outformat
    min_runs = args.minruns
    max_runs = args.maxruns
    time_out = args.timeout
    ppn = args.ppn
    alpha, beta = args.alpha, args.beta
    converge_all = args.convergeall
    ro_mode = args.runtimeout
    data_path = args.datapath
    num_nodes = args.numnodes

    random.seed(args.seed)

    if node_file == "auto":
        if not "BLINK_WL_MANAGER" in os.environ or os.environ["BLINK_WL_MANAGER"] != "slurm":
            raise Exception("auto node file can only be used if slurm is used as workload manager.")
        node_file = "node_files/auto_node_file.txt"
        subprocess.call(["scontrol", "show", "hostnames"], stdout=open(node_file, "w"))

    
    # Create header in description.csv
    if not os.path.isfile(data_path + "/description.csv"):
        with open(data_path + '/description.csv', 'w') as desc_file:
            desc_file.write('app_mix,system,numnodes,allocation_mode,allocation_split,ppn,out_format,extra,path\n')        
    
    # runner_id is current time
    while True:
        runner_id = (os.environ["BLINK_SYSTEM"] + "/" + datetime.datetime.now().strftime('%Y-%m-%d_%H:%M:%S'))
        data_directory = data_path + '/' + runner_id
        if not os.path.exists(data_directory):
            os.makedirs(data_directory)
            break
    

    # Append info to description.csv file
    with open(data_path + '/description.csv', 'a+') as desc_file:
        extra = ""
        if args.extrainfo:
            extra = args.extrainfo
        desc_file.write(app_mix_path + ',' + os.environ["BLINK_SYSTEM"] + ',' + str(args.numnodes) + ',' + allocation_mode + ',' +
                        allocation_split + ',' + str(ppn) + ',' + out_format + ',' + extra + ',' + data_directory + '\n')
           

    # prepare runtime feedback output
    ro_file_path = data_directory+'/runtime'
    if ro_mode == 'file' or ro_mode == '+file':
        ro_file = open(ro_file_path, 'w')
    else:
        ro_file = None
    if ro_mode == 'file':
        print('Runtime feedback printed to: '+ro_file_path)

    print_runtime('\nPassed arguments:', ro_mode, ro_file)
    print_runtime(args, ro_mode, ro_file)

    # requiered testing parameters
    num_apps = 0
    apps = []  # list for all apps
    apps_awaited = []  # list of index of apps for which gets waited at end of iteration
    apps_waiting = []  # list of index of apps which wait at end of iteration
    schedule = []  # execution order, holds triples (app_id,action,time_stamp)

    print_runtime('\nApps:', ro_mode, ro_file)

    # read schedule file and import app classes
    with open(app_mix_path, 'r') as test_bench:
        # get delimiter from first line
        first_line = test_bench.readline()
        file_delimiter = first_line[:-1]
        if file_delimiter == '':
            file_delimiter = ','  # default for delimiter

        for line in test_bench:
            line_list = [x.strip() for x in line.split(file_delimiter)]

            # path to class
            import_path_app = line_list[0]

            # arguments for app
            args = line_list[1]

            # collection flag
            if line_list[2] == '0':
                collect_flag = False
            else:
                collect_flag = True

            # start of app
            if line_list[3] == '':
                start_time = 0
            else:
                start_time = float(line_list[3])
            schedule += [(num_apps, 's', start_time)]

            # termination of app
            if line_list[4] == '':
                apps_awaited += [num_apps]
                end_cond = 'run until finished'
            elif line_list[4] == 'f':
                apps_waiting += [num_apps]
                end_cond = 'wait until others finished'
            else:
                end_time = float(line_list[4])
                if end_time < start_time:
                    raise Exception('Application '+str(num_apps) +
                                    ' must be started before it can be killed.')
                if collect_flag:  # if data should be collected send sigterm
                    schedule += [(num_apps, 't', end_time)]
                    end_cond = 'terminate at: '+str(end_time)+'s'
                else:  # if no data needs to be collected can just kill
                    schedule += [(num_apps, 'k', end_time)]
                    end_cond = 'kill at: '+str(end_time)+'s'

            # import app class
            # assumes name is last element of path without .py suffix
            app_class_name = (import_path_app.split(os.sep)[-1])[:-3]
            spec_app = importlib.util.spec_from_file_location(
                app_class_name, import_path_app)
            mod_app = importlib.util.module_from_spec(spec_app)
            spec_app.loader.exec_module(mod_app)
            apps += [mod_app.app(num_apps, collect_flag, args)]

            print_runtime((str(num_apps)+': with arguments:"'+args+'", collection flag: '+str(collect_flag)
                           + ', ending condition: '+end_cond+'.'), ro_mode, ro_file)

            num_apps += 1

    if num_apps == 0:
        raise Exception('Must specify at least one application to run.')

    # import wlm class
    # assumes name is last element of path without .py suffix
    wlm_class_name = (import_path_wlm.split(os.sep)[-1])[:-3]
    spec_wlm = importlib.util.spec_from_file_location(
        wlm_class_name, import_path_wlm)
    mod_wlm = importlib.util.module_from_spec(spec_wlm)
    spec_wlm.loader.exec_module(mod_wlm)
    wlmanager = mod_wlm.wl_manager()

    # assign nodes to apps
    nodes_frame = pandas.read_csv(node_file, header=None, keep_default_na=False, dtype=str)
    #num_elems = nodes_frame.size
    num_cols = nodes_frame.shape[1]
    if num_nodes % num_cols:
        raise Exception('Number of nodes must be a multiple of columns in '+node_file+'.')
    else:
        nodes_frame.head(num_nodes // num_cols)

    if allocation_mode == 'c':  # custom allocation, doesn't need splits
        if num_cols != num_apps:
            raise Exception(
                'Must specify exactly one column of nodes for every application in '+node_file+'.')
        c_allocation(apps, nodes_frame)
    else:
        split_absolute = get_abs_split(allocation_split, num_apps, num_nodes)
        node_list = nodes_frame.iloc[:, 0].tolist()
        if allocation_mode == 'i':
            i_allocation(apps, node_list, split_absolute)
        else:  # mode 'l' or 'r' or '+r'
            if allocation_mode == 'r' or allocation_mode == '+r':
                random.shuffle(node_list)
            l_allocation(apps, node_list, split_absolute)

    print_runtime('\nNode allocation:', ro_mode, ro_file)
    print_runtime('Processes per node (ppn): '+str(ppn), ro_mode, ro_file)
    for app in apps:
        print_runtime(str(app.id_num)+' on '+str(app.num_nodes) +
                      ' nodes:', ro_mode, ro_file)
        print_runtime(np.array(app.node_list), ro_mode, ro_file)
    # sort schedule chronologically
    print_runtime('\nSchedule:', ro_mode, ro_file)
    schedule.sort(key=lambda t: t[2])
    for app_id, action_type, action_time in schedule:
        if action_type == 's':
            print_runtime(str(action_time)+'s: start ' +
                          str(app_id), ro_mode, ro_file)
        elif action_type == 'k':
            print_runtime(str(action_time)+'s: kill ' +
                          str(app_id), ro_mode, ro_file)
        elif action_type == 't':
            print_runtime(str(action_time)+'s: terminate ' +
                          str(app_id), ro_mode, ro_file)

    # prepare data storing
    tot_num_metrics = 0
    data_container_list = []
    for app in apps:
        if app.collect_flag:
            for i in range(len(app.metadata)):
                data_container_list += [data_container(app.id_num, app.metadata[i]["conv"], app.metadata[i]["name"], app.metadata[i]["unit"])]

    # end of preps
    print_runtime('\nPreparing took '+str(round(time.time() -
                  pre_start_time, 5))+'s.', ro_mode, ro_file)
    print_runtime('\nRunning...', ro_mode, ro_file)

    # start running
    runs = 0
    converged = False
    start_time = time.time()
    while True:
        exec_time = time.time()-start_time
        # termination conditions
        if runs >= max_runs:  # termination condition reached max runs
            print_runtime('Completed maximum number of runs, terminated after '
                          + str(runs)+' runs taking '+str(round(exec_time, 2))+' seconds.', ro_mode, ro_file)
            break
        elif runs >= min_runs and converged:  # termination condition data converged
            print_runtime('Completed after reaching confidence interval, terminated after '
                          + str(runs)+' runs taking '+str(round(exec_time, 2))+' seconds.', ro_mode, ro_file)
            break
        if exec_time >= time_out:  # termination condition timeout inbetween schedules
            print_runtime('Completed after timeout, terminated after '+str(runs)+' runs taking '
                          + str(round(exec_time, 2))+' seconds.', ro_mode, ro_file)
            break
        # not enough time to finish another run of the schedule
        if (time_out-(time.time()-start_time) < (schedule[-1])[2]):
            print_runtime('Completed after timeout (remaining time not enough for schedule), terminated after '
                          + str(runs)+' runs taking '+str(round(exec_time, 2))+' seconds.', ro_mode, ro_file)
            break

        # clean tmp directory
        try:
            for file in os.listdir('./tmp'):
                os.unlink('./tmp/'+file)
        except FileNotFoundError:
            pass

        # execute schedule
        print_runtime(' Run '+str(runs+1)+':', ro_mode, ro_file)
        run_start_time = time.time()

        # reshuffle if needed
        if allocation_mode == '+r':
            random.shuffle(node_list)
            l_allocation(apps, node_list, split_absolute)
            print_runtime('  Reshuffled node allocation.', ro_mode, ro_file)

        current_time = 0
        for app_id, action_type, action_time in schedule:
            time.sleep(action_time-current_time)
            if action_type == 's':
                run_job(apps[app_id], wlmanager, ppn)
                print_runtime('  '+str(round(time.time()-run_start_time, 5)
                                       )+'s: started '+str(app_id), ro_mode, ro_file)
            elif action_type == 'k':
                end_job(apps[app_id])
                print_runtime('  '+str(round(time.time()-run_start_time, 5)
                                       )+'s: killed '+str(app_id), ro_mode, ro_file)
            elif action_type == 't':
                end_job(apps[app_id])
                print_runtime('  '+str(round(time.time()-run_start_time, 5)) +
                              's: terminated '+str(app_id), ro_mode, ro_file)
            else:
                raise Exception('Unkown schedule action: '+action_type)
            current_time = action_time

        # wait for all apps that don't get killed
        timeout_occured = False
        for app_ind in apps_awaited:
            remaining_time = time_out-(time.time()-start_time)
            timeout_occured = wait_timed(apps[app_ind], remaining_time)
            if timeout_occured:
                print_runtime('  '+str(round(time.time()-run_start_time, 5)) +
                              's: timed out '+str(app_ind), ro_mode, ro_file)
            else:
                print_runtime('  '+str(round(time.time()-run_start_time, 5)) +
                              's: awaited '+str(app_ind), ro_mode, ro_file)

        # kill all apps that were marked with 'f' and are still waiting
        for app_ind in apps_waiting:
            end_job(apps[app_ind])
            if apps[app_ind].collect_flag:
                print_runtime('  '+str(round(time.time()-run_start_time, 5))+'s: terminated waiting '
                              + str(app_ind), ro_mode, ro_file)
            else:
                print_runtime('  '+str(round(time.time()-run_start_time, 5))+'s: killed waiting '
                              + str(app_ind), ro_mode, ro_file)

        if timeout_occured:
            # timeout occured terminate, don't read data anymore
            print_runtime('  No data collection due to timeout.',
                          ro_mode, ro_file)
            print_runtime('Completed after timeout, terminated after '+str(runs)+' runs taking '
                          + str(round(time.time()-start_time, 2))+' seconds.', ro_mode, ro_file)
            break

        runs += 1
        # check if any application threw exception during execution
        for app in apps:
            # print_runtime(app.stdout,ro_mode,ro_file)
            if app.stderr != '':
                print_runtime('  '+str(round(time.time()-run_start_time, 5))+'s: encountered an exception in '
                              + str(app.id_num), ro_mode, ro_file)
                print_runtime(app.stderr, ro_mode, ro_file)

        # get data
        data_collection_start_time = time.time()
        j = 0
        for app in apps:
            if (len(app.metadata) > 0) and app.collect_flag:
                data_list_of_list = app.read_data()
                for data_list in data_list_of_list:
                    data_container_list[j].data += data_list
                    data_container_list[j].num_samples += [len(data_list)]
                    j += 1
        print_runtime('  Data collection took '+str(round(time.time() -
                      data_collection_start_time, 5))+'s.', ro_mode, ro_file)

        # check convergence
        if runs >= min_runs:
            conv_check_start_time = time.time()
            converged = check_CI(data_container_list, alpha,
                                 beta, converge_all, runs)
            print_runtime('  Convergence check took '+str(round(time.time() -
                          conv_check_start_time, 5))+'s.', ro_mode, ro_file)

    # create logs
    log_start_time = time.time()
    log_data(out_format, data_directory+'/data', data_container_list)
    log_meta_data(out_format, data_directory+'/metadata', data_container_list, runs)
    print_runtime('Writing data & meta-data took ' +
                  str(round(time.time()-log_start_time, 5))+'s.', ro_mode, ro_file)

    print_runtime('Overall took '+str(round(time.time() -
                  pre_start_time, 5))+'s.', ro_mode, ro_file)

    if ro_mode == 'file' or ro_mode == '+file':
        ro_file.close()


if __name__ == '__main__':
    main()
