#!/usr/bin/python3

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

# For a given benchmark and metric, returns the data.
# We always assume that
# - We return bandwidth in Gb/s
# - We return runtime in microseconds
def get_bench_data(bench, input, metric, filename, ppn=1, nodes=0):
    microbenchs = ["ping-pong_b", "pw-ping-pong_b", "a2a_b", "ardc_b"]
    if "gpubench" in bench:
        if metric == "Bandwidth":
            return pd.read_csv(filename)["0_Bandwidth_GB/s"]*8
        elif metric == "Runtime" or metric == "Latency":
            return pd.read_csv(filename)["0_Transfer Time_s"]*1e6
    elif bench == "nccl-sendrecv" or bench == "nccl-allreduce" or bench == "nccl-alltoall":
        if metric == "Bandwidth":
            return pd.read_csv(filename)["0_busbw-ip_GB/s"]*8
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
    return bench


# Plots one violin for each victim+aggressor combination, for the given metric
def plot_violin(df, title, metric, outname, max_y):
    # Setup the violin
    ax = sns.violinplot(data=df, cut=0)

    # Set the title and labels
    ax.set_title(title)
    ax.set_ylabel(add_unit_to_metric(metric))
    if max_y:
        ax.set_ylim(0, float(max_y))

    # Add 99th quantile as a black x
    q99s = df.quantile(0.99).to_list()    
    plt.scatter(x = range(len(q99s)), y = q99s, c = 'black', marker = 'x', s = 50)

    # Save to file
    #ax.figure.savefig(outname + "_violin.png", bbox_inches='tight')
    ax.figure.savefig(outname + "_violin.pdf", bbox_inches='tight')
    plt.clf()      

# Plots one box for each victim+aggressor combination, for the given metric
def plot_box(df, title, metric, outname, max_y, showfliers=True):
    # Setup the violin
    ax = sns.boxplot(data=df, showfliers=showfliers)

    # Set the title and labels
    ax.set_title(title)
    ax.set_ylabel(add_unit_to_metric(metric))
    if max_y:
        ax.set_ylim(0, float(max_y))

    # Add 99th quantile as a black x
    q99s = df.quantile(0.99).to_list()    
    plt.scatter(x = range(len(q99s)), y = q99s, c = 'black', marker = 'x', s = 50)

    # Save to file
    #ax.figure.savefig(outname + "_box.png", bbox_inches='tight')
    ax.figure.savefig(outname + "_box" + ("" if showfliers else "_nofliers") + ".pdf", bbox_inches='tight')
    plt.clf()

def plot_dist(df, title, metric, outname, max_y):
    # Setup the violin
    ax = sns.displot(data=df, kind="kde")

    # Set the title and labels
    #ax.set_title(title)
    #ax.set_ylabel(add_unit_to_metric(metric))
    #if max_y:
    #    ax.set_ylim(0, float(max_y))

    # Save to file
    #ax.figure.savefig(outname + "_dist.png", bbox_inches='tight')
    ax.figure.savefig(outname + "_dist.pdf", bbox_inches='tight')
    plt.clf()

# Plots one line for each victim+aggressor combination, for the given metric
# Time/iteration on the x-axis
def plot_line(df, title, metric, outname, max_y):
    # Setup the violin
    ax = sns.lineplot(data=df)

    # Set the title and labels
    ax.set_title(title)
    ax.set_xlabel("Iteration")
    ax.set_ylabel(add_unit_to_metric(metric))
    if max_y:
        ax.set_ylim(0, float(max_y))

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

'''
def main():
    print(get_data_filename("./data", "leonardo", 2, "l", "100", 1, "diff_groups_SL1", "gpubench-pp-nccl", "1GiB", "",""))

if __name__=='__main__':
    main()
'''