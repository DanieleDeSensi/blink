#!/usr/bin/env python3

# - If one victim and multiple aggressors are specified, it plots the performance of the victim for different aggressors.
# - If one victim and multiple extras are specified, it plots the performance of the victim for the different extras (e.g., same switch, different groups, etc...).
# - If one victim with multiple inputs is specified (and no aggressors are specified) it plots the performance of the victim for different inputs.

import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import matplotlib.ticker as ticker
import os
import argparse
import csv
import importlib.util
import ast

matplotlib.rc('pdf', fonttype=42) # To avoid issues with camera-ready submission
matplotlib.rc('font', size=12)

# Extracts victim and aggressor from an app mix file
# Assumes only two applications are specified in the mix, 
# one of which is the victim and the other the aggressor
def extract_info(app_mix):
    victim_line = ""
    aggressor_line = ""
    with open(app_mix) as mix_file:
        lines = mix_file.readlines()
        if len(lines) > 3:
            raise Exception("Error: app mix file " + app_mix + " has more (or less) than 3 lines")
            
        sep = lines[0].strip()
        if lines[0] == "":
            sep = ","
        app_lines_idxs = np.arange(1, len(lines))
        for appi in app_lines_idxs:
            l = lines[appi].strip()
            end = l.split(sep)[-1]
            if end == "f": # Aggressor
                aggressor_line = l
            else:
                victim_line = l

    d = {}
    d["victim_wrapper"] = victim_line.split(sep)[0]
    d["victim_args"] = victim_line.split(sep)[1]
    d["victim_collection"] = victim_line.split(sep)[2]
    d["victim_start"] = victim_line.split(sep)[3]
    d["victim_end"] = victim_line.split(sep)[4]

    if aggressor_line != "":
        d["aggressor_wrapper"] = aggressor_line.split(sep)[0]
        d["aggressor_args"] = aggressor_line.split(sep)[1]
        d["aggressor_collection"] = aggressor_line.split(sep)[2]
        d["aggressor_start"] = aggressor_line.split(sep)[3]
        d["aggressor_end"] = aggressor_line.split(sep)[4]
    return d

def get_input_and_full_name_from_args(wrapper_path, args):
    class_name = (wrapper_path.split(os.sep)[-1])[:-3]
    spec_app = importlib.util.spec_from_file_location(class_name, wrapper_path)
    mod_app = importlib.util.module_from_spec(spec_app)
    spec_app.loader.exec_module(mod_app)
    w = mod_app.app(0, 0, args)
    return (bench_to_human_readable(w.get_bench_name()), w.get_bench_input())

# Loads the data of the specified aggressors (only the specified metric).
# This is used for distribution plots (box/violin/etc)
# Returns the DataFrame with the data.
def load_data_distribution(categories, data_filename, metric):
    global_df = pd.DataFrame()
    # Load the data from data_filename using pandas    
    for a in categories:
        data_path = data_filename[a] 
        data = pd.read_csv(data_path)[metric]
        if data.empty:
            raise Exception("Error: data file " + data_path + " does not contain data for metric " + metric)
        global_df[a] = data
    return global_df

def input_size_to_bytes(size_str):
    size_str = size_str.strip().lower()

    if size_str.endswith('gib'):
        return int(float(size_str[:-3].strip()) * 2**30)
    elif size_str.endswith('mib'):
        return int(float(size_str[:-3].strip()) * 2**20)
    elif size_str.endswith('kib'):
        return int(float(size_str[:-3].strip()) * 2**10)
    elif size_str.endswith('b'):
        return int(size_str[:-1].strip())

    raise ValueError(f"Invalid file size string: {size_str}")

def get_num_ib_devices(system):
    # Find the number of IB_DEVICES in conf file
    conffile = os.environ["BLINK_ROOT"] + "/conf/" + system + ".sh"
    with open(conffile, "r") as f:
        lines = f.readlines()
        for l in lines:
            if "BLINK_IB_DEVICES" in l:
                num_devices = l.split("=")[1].strip().count("#") + 1
                break
    return num_devices


# For a given benchmark and metric, returns the data.
# We always assume that
# - We return bandwidth in Gb/s
# - We return runtime in microseconds
def get_bench_data(bench, input, metric, filename, ppn, nodes, system):
    microbenchs = ["ping-pong_b", "pw-ping-pong_b", "a2a_b", "ardc_b"]
    if "gpubench" in bench:
        if metric == "Bandwidth":
            return pd.read_csv(filename)["0_Bandwidth_GB/s"]*8
        elif metric == "Runtime" or metric == "Latency":
            return pd.read_csv(filename)["0_Transfer Time_s"]*1e6
    elif bench == "nccl-sendrecv" or bench == "nccl-allreduce" or bench == "nccl-alltoall":
        if metric == "Bandwidth":
            return pd.read_csv(filename)["0_busbw-ip_GB/s"]*8
        elif metric == "Runtime" or metric == "Latency":
            return pd.read_csv(filename)["0_time-ip_us"]
    elif bench == "ib_send_lat":
        if metric == "Runtime" or metric == "Latency":
            return pd.read_csv(filename)["1_time_us"] # 0_ is the server
        elif metric == "Bandwidth":
            num_devices = get_num_ib_devices(system)
            input_bits = input_size_to_bytes(input)*8*num_devices
            input_gbits = input_bits / 1e9
            return input_gbits / (pd.read_csv(filename)["1_time_us"] / 1e6)
    elif bench in microbenchs:
        if bench == "ping-pong_b":
            time_str = "0_MainRank-Duration_s"            
        else:
            time_str = "0_Max-Duration_s"
        if metric == "Runtime" or metric == "Latency":
            return pd.read_csv(filename)[time_str]*1e6
        elif metric == "Bandwidth":
            input_bits = input_size_to_bytes(input)*8
            input_gbits = input_bits / 1e9
            gbit_s = input_gbits / (pd.read_csv(filename)[time_str]) # Time is already in seconds
            if bench == "pw-ping-pong_b":
                gbit_s *= int(ppn)
            elif bench == "a2a_b":
                ranks = int(nodes) * int(ppn)
                gbit_s *= (ranks - 1) # I send that count to each of the other nodes
            elif bench == "ardc_b":
                gbit_s *= 2 # I actually send twice the data (e.g., Rabenseifner's algorithm)
            return gbit_s
    raise Exception("Error: metric " + metric + " not supported for bench " + bench)

def add_unit_to_metric(metric_hr):
    if metric_hr == "Bandwidth":
        return "Bandwidth (Gb/s)"
    elif metric_hr == "Runtime":
        return "Runtime (us)"
    elif metric_hr == "Latency":
        return "Latency (us)"
    return metric_hr

def system_to_human_readable(system):
    if system == "lumi":
        return "LUMI"
    elif system == "leonardo":
        return "Leonardo"
    else:
        return system

def bench_to_human_readable(bench):
    microbenchs = ["Ping-Pong", "Pairwise Ping-Pong", "Alltoall", "Allreduce"]
    if "gpubench" in bench:
        gpubench,bench,version = bench.split(" ") 
        if version == "Baseline":
            return "Host Mem. Staging"
        elif version == "CudaAware":
            return "CUDA-Aware MPI"
        elif version == "Nccl":
            return "NCCL"
        elif version == "Nvlink":
            return "CUDA IPC"
    elif bench in microbenchs:
        return "MPI"
    elif bench == "ib_send_lat":
        return "IB Verbs"
    return bench

# Returns the actual name of the benchmark, given the name of the benchmark and the system
def get_actual_bench_name(bench, system, input):
    if bench.startswith("#"):
        if bench == "#distance-cpu":
            if system == "lumi":
                return "pw-ping-pong_b"
            elif system == "leonardo":
                if input == "1B":
                    return "pw-ping-pong_b"
                else:
                    return "ib_send_lat"
        elif bench == "#distance-gpu":
            if system == "lumi":
                return "pw-ping-pong_b" # TODO: Fix
            elif system == "leonardo":
                return "gpubench-mpp-nccl"
    else:
        return bench
    raise Exception("Error: bench " + bench + " not supported for system " + system)

# Returns the actual name of the extra, given the name of the extra and the system
def get_actual_extra_name(extra, system, victim_name):
    if extra.startswith("#"):
        if "#diff_group" in extra or "#diff_switch" in extra or "#same_switch" in extra:
            if system == "lumi":
                return extra.replace("#", "")
            elif system == "leonardo":
                if "#SL0" in extra: #diff_group_SL0
                    if victim_name == "ib_send_lat":
                        return extra.replace("#", "", 1).replace("#SL0", "_SL0")
                    else:
                        return extra.replace("#", "", 1).replace("#SL0", "_SL0")
                elif "#SL1" in extra: #diff_group_SL1
                    if victim_name == "ib_send_lat":
                        return extra.replace("#", "", 1).replace("#SL1", "_SL1")
                    else:                        
                        return extra.replace("#", "", 1).replace("#SL1", "_SL1")
                else: #diff_group
                    return extra.replace("#", "", 1) + "_SL0"
    else:
        return extra
    raise Exception("Error: extra " + extra + " not supported for system " + system)


def get_actual_input_name(input, metric):
    if input == "#":
        if metric == "Latency":
            return "1B"
        elif metric == "Bandwidth":
            return "1GiB"
    else:
        return input
    raise Exception("Error: input " + input + " not supported for metric " + metric)

# Plots one violin for each victim+aggressor combination, for the given metric
def plot_violin(df, metric, ax):
    # Setup the violin
    sns.violinplot(data=df, cut=0, ax=ax)

    # Set the labels
    ax.set_ylabel(add_unit_to_metric(metric))
    # Add quantile as a black x
    #if metric == "Bandwidth":
    #    q = 0.01
    #else:
    #    q = 0.99    
    #x = df.quantile(q).to_list()    
    x = df.mean().to_list()
    sns.scatterplot(x = range(len(x)), y = x, c = 'black', marker = 'x', s = 50, ax=ax)

# Plots one box for each victim+aggressor combination, for the given metric
def plot_box(df, metric, ax, showfliers=True):
    # Setup the violin
    sns.boxplot(data=df, showfliers=showfliers, ax=ax, whis=[5, 95], notch=True)

    # Set the title and labels
    ax.set_ylabel(add_unit_to_metric(metric))
    
    # Add quantile as a black x
    #if metric == "Bandwidth":
    #    q = 0.01
    #else:
    #    q = 0.99    
    #x = df.quantile(q).to_list()    
    x = df.mean().to_list()

    plt.scatter(x = range(len(x)), y = x, c = 'black', marker = 'x', s = 50)

def plot_dist(df, metric, ax):
    # Setup the violin
    sns.kdeplot(data=df, ax=ax)
    # Set the title and labels
    ax.set_xlabel(add_unit_to_metric(metric))

# Plots one line for each victim+aggressor combination, for the given metric
# Time/iteration on the x-axis
def plot_line(df, metric, outname, max_y, xticklabels, title):
    # Setup the violin
    ax = sns.lineplot(data=df)

    # Set the title and labels
    ax.set_title(title)
    ax.set_xlabel("Iteration")
    ax.set_ylabel(add_unit_to_metric(metric))
    if max_y:
        ax.set_ylim(0, float(max_y))
    if xticklabels:
        ax.set_xticklabels(ast.literal_eval(xticklabels))
    # Save to file
    #ax.figure.savefig(outname + "_line.png", bbox_inches='tight')
    ax.figure.savefig(outname + "_line.pdf", bbox_inches='tight')
    plt.clf()

# Returns a tuple (filename, victim_fullname, aggressor_fullname)
def get_data_filename(data_folder, system, numnodes, allocation_mode, allocation_split, ppn, extra, victim_name, victim_input, aggressor_name, aggressor_input):
    # Read the description file to find the data files
    to_return = (None, None, None)
    with open(data_folder + "/description.csv", mode='r') as infile:
        reader = csv.DictReader(infile)    
        for line in reader:
            row = {key: value for key, value in line.items()}       
            # Check if the fields in the description match the ones in the arguments
            if row["system"] == system and int(row["numnodes"]) == int(numnodes) and \
               row["allocation_mode"] == allocation_mode and row["allocation_split"] == allocation_split and \
               int(row["ppn"]) == int(ppn) and row["extra"] == extra:   
                # Check if the mix matches the victim and aggressor
                info = extract_info(row["app_mix"])                
                
                # Get the bench name from the Python wrapper filename and load the python wrapper to get the full name and the input name
                victim_shortname = info["victim_wrapper"].split("/")[-1][:-3]                
                (victim_fn, victim_in) = get_input_and_full_name_from_args(os.environ["BLINK_ROOT"] + os.path.sep + info["victim_wrapper"], info["victim_args"])
                if "aggressor_wrapper" in info:
                    aggressor_shortname = info["aggressor_wrapper"].split("/")[-1][:-3]
                    (aggressor_fn, aggressor_in) = get_input_and_full_name_from_args(os.environ["BLINK_ROOT"] + os.path.sep + info["aggressor_wrapper"], info["aggressor_args"])
                else:
                    aggressor_shortname = ""
                    aggressor_fn = ""
                    aggressor_in = ""
                if victim_name == victim_shortname and victim_input == victim_in and \
                   aggressor_name == aggressor_shortname and aggressor_input == aggressor_in:
                    to_return = (row["path"] + "/data.csv", victim_fn, aggressor_fn)
                    # We do not return immediately because we might have multiple entries for the same test,
                    # and we want to consider the most recent one among those
    return to_return

def patch_violinplot(palette, n, ax):
    from matplotlib.collections import PolyCollection
    violins = [art for art in ax.get_children() if isinstance(art, PolyCollection)]
    colors = sns.color_palette(palette, n_colors=n) * (len(violins)//n)
    for i in range(len(violins)):
        violins[i].set_edgecolor(colors[i])

def get_default_aggressor_input(aggressor):
    if aggressor == "gpubench-a2a-nccl":
        return "128KiB"
    elif aggressor == "inc_b":
        return "128KiB"
    elif aggressor == "a2a_b":
        return "128KiB"
    else:
        return ""

def get_human_readable_allocation_mode(am):
    if am == "i":
        return "Interleaved Alloc."
    elif am == "r":
        return "Random Alloc."
    elif am == "l":
        return "Linear Alloc."
    else:
        return am

'''
def main():
    print(get_data_filename("./data", "leonardo", 2, "l", "100", 1, "diff_groups_SL1", "gpubench-pp-nccl", "1GiB", "",""))

if __name__=='__main__':
    main()
'''