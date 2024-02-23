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
    return (w.get_bench_name(), w.get_bench_input())

# Loads the data of the specified aggressors (only the specified metric).
# This is used for distribution plots (box/violin/etc)
# Returns the DataFrame with the data.
def load_data_distribution(aggressors_info, data_filename, metric):
    global_df = pd.DataFrame()
    # Load the data from data_filename using pandas    
    for a in aggressors_info.split(","):
        data_path, aggressor_fullname = data_filename[a]
        if ":" in a:
            aggressor_input = " (" + a.split(":")[1] + ")"
        else:
            aggressor_input = ""        
        data = pd.read_csv(data_path)[metric]
        if data.empty:
            raise Exception("Error: data file " + data_path + " does not contain data for metric " + metric)
        global_df[a] = data
        # Rename the column to human-readable name
        global_df = global_df.rename(columns={a: aggressor_fullname + aggressor_input})
    return global_df

# Loads the data of the specified victim, for different inputs.
# This is used for trend plots.
# Returns the DataFrame with the data.
def load_data_trend(victim_inputs, data_filename, metric):
    global_df = pd.DataFrame()
    # Load the data from data_filename using pandas    
    for vin in victim_inputs:
        data_path = data_filename[vin]
        data = pd.DataFrame()
        data[metric] = pd.read_csv(data_path)[metric]
        if data.empty:
            raise Exception("Error: data file " + data_path + " does not contain data for metric " + metric)
        data["Input"] = vin
        global_df = pd.concat([global_df, data], ignore_index=True)
    return global_df

def metric_to_human_readable(metric):
    metric_expanded = {}
    metric_expanded["Avg-Duration"] = "Runtime"
    metric_expanded["MainRank-Duration"] = "Runtime"
    metric_expanded["busbw-ip"] = "Bus Bandwidth (In-Place)"
    metric_expanded["algbw-ip"] = "Algo Bandwidth (In-Place)"
    metric_expanded["time-ip"] = "Runtime (In-Place)"
    # See get_title in data_container in runner.py to check how the header of the data.csv files is created
    # It is composed of appid_metric_unit
    # The appid is the id of the application in the mix, starting from 0
    # Since we assume there are always two applications (one of which is the aggressor), we can ignore it 
    # (i.e., the data file only contains data about a single application)
    appid, metric, unit = metric.split("_")
    return metric_expanded[metric] + " (" + unit + ")"

# Plots one violin for each victim+aggressor combination, for the given metric
def plot_violin(df, title, metric, outname):
    # Setup the violin
    ax = sns.violinplot(data=df, cut=0)

    # Set the title and labels
    ax.set_title(title)
    ax.set_ylabel(metric_to_human_readable(metric))

    # Add 99th quantile as a black x
    q99s = df.quantile(0.99).to_list()    
    plt.scatter(x = range(len(q99s)), y = q99s, c = 'black', marker = 'x', s = 50)

    # Save to file
    ax.figure.savefig(outname + "_violins.png", bbox_inches='tight')
    ax.figure.savefig(outname + "_violins.pdf", bbox_inches='tight')
    plt.clf()      

# Plots one box for each victim+aggressor combination, for the given metric
def plot_box(df, title, metric, outname):
    # Setup the violin
    ax = sns.boxplot(data=df)

    # Set the title and labels
    ax.set_title(title)
    ax.set_ylabel(metric_to_human_readable(metric))

    # Add 99th quantile as a black x
    q99s = df.quantile(0.99).to_list()    
    plt.scatter(x = range(len(q99s)), y = q99s, c = 'black', marker = 'x', s = 50)

    # Save to file
    ax.figure.savefig(outname + "_boxes.png", bbox_inches='tight')
    ax.figure.savefig(outname + "_boxes.pdf", bbox_inches='tight')
    plt.clf()

# Plots one line for each victim+aggressor combination, for the given metric
# Time/iteration on the x-axis
def plot_line(df, title, metric, outname):
    # Setup the violin
    ax = sns.lineplot(data=df)

    # Set the title and labels
    ax.set_title(title)
    ax.set_xlabel("Iteration")
    ax.set_ylabel(metric_to_human_readable(metric))

    # Save to file
    ax.figure.savefig(outname + "_lines.png", bbox_inches='tight')
    ax.figure.savefig(outname + "_lines.pdf", bbox_inches='tight')
    plt.clf()

# Plots the trend of victim performance for different inputs
# Time/iteration on the x-axis
def plot_trend_line(df, title, metric, outname, trend_limit):
    # Setup the plot
    ax = sns.lineplot(data=df, x="Input", y=metric, marker="o")

    # Plots the limit if specified
    if trend_limit:
        m, limit = trend_limit.split(":")
        if metric == m:
            ax.axhline(y=float(limit), color='black', linestyle='--')

    # Set the title and labels
    ax.set_title(title)
    ax.set_xlabel("Input")
    ax.set_ylabel(metric_to_human_readable(metric))

    # Save to file
    ax.figure.savefig(outname + "_lines.png", bbox_inches='tight')
    ax.figure.savefig(outname + "_lines.pdf", bbox_inches='tight')
    plt.clf()

def main():
    parser=argparse.ArgumentParser(description='Plots the results for a single app mix (violins, boxes, and timeseries), with and without congestion.')
    parser.add_argument('-d', '--data_folder', help='Main data folder.', default="data")
    parser.add_argument('-s', '--system', help='System name.', required=True)
    parser.add_argument('-v', '--victim_info', help='Victim info in the format name:input_name_0:input_name_1:... (e.g. ardc_b:128B:256B). The name must match the filename of the Python wrapper.', required=True)
    parser.add_argument('-a', '--aggressors_info', help='Comma-separated list of aggressors name:input_name (e.g. a2a:128,inc:1024). Names must match the filename of the Python wrapper.')
    parser.add_argument('-n', '--num_nodes', help='The number of nodes the mix was executed on (total).', required=True)
    parser.add_argument('-am', '--allocation_mode', help='The allocation mode the mix was executed with.', required=True)
    parser.add_argument('-sp', '--allocation_split', help='The allocation split the mix was executed with.', required=True)
    parser.add_argument('-e', '--extra', help='Any extra info about the execution (comma-separated list).')
    parser.add_argument('-m', '--metrics', help='Comma-separated string of metrics to plot.', default="0_Avg-Duration_s")
    parser.add_argument('-p', '--ppn', help='Processes per node.', default=1)
    parser.add_argument('-tl', '--trend_limit', help='For trends plots, it plots the upper limit. (format metric:limit)')
    parser.add_argument('-o', '--outfile', help='Path of output files.')

    args = parser.parse_args()

    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) # Set BLINK_ROOT to the directory of this script
    data_filename = {}
    victim_fullname = ""
    victim_name_h = ""
    if args.outfile:
        out_file_prefix = args.outfile
    else:
        if args.aggressors_info:
            agg_str = "_" + args.aggressors_info
        else:
            agg_str = ""
        out_file_prefix = "./plots" + os.path.sep + args.system + os.path.sep + args.victim_info + agg_str + "_" + args.num_nodes + "_" + args.allocation_mode + "_" + args.allocation_split
        if args.extra:
            out_file_prefix += "_" + args.extra        

    if not os.path.exists(out_file_prefix): 
        os.makedirs(out_file_prefix)

    plot_trend_inputs = False

    # If we specify the same victim with many inputs and one aggressors, then we plot the
    # performance of the victim for different inputs.
    # Otherwise, we plot the combination of the victim and aggressors.
    victim_inputs = []
    if args.victim_info.count(":") > 1 and (not args.aggressors_info or args.aggressors_info.count(",") == 0):
        plot_trend_inputs = True
        # Trend for the different inputs of the victim
        # Check that is always the same victim
        victim_name = args.victim_info.split(":")[0]        
        victim_inputs = args.victim_info.split(":")[1:]
    else:
        plot_trend_inputs = False
        # Parse the victim info
        if ":" in args.victim_info:
            victim_name, victim_input = args.victim_info.split(":")
        else:
            victim_name = args.victim_info
            victim_input = ""
        victim_inputs.append(victim_input)
            
    # Parse the aggressor info
    aggressors_input = {}
    if args.aggressors_info:
        for a in args.aggressors_info.split(","):
            if ":" in a:
                name, input = a.split(":")
            else:
                name = a
                input = ""
            aggressors_input[name] = input
        
    # Read the description file to find the data files
    with open(args.data_folder + "/description.csv", mode='r') as infile:
        reader = csv.DictReader(infile)    
        for line in reader:
            row = {key: value for key, value in line.items()}       
            
            # Check if the fields in the description match the ones in the arguments
            if row["system"] == args.system and row["numnodes"] == args.num_nodes and \
               row["allocation_mode"] == args.allocation_mode and row["allocation_split"] == args.allocation_split and \
               int(row["ppn"]) == int(args.ppn) and \
               ((args.extra and row["extra"] == args.extra) or (not args.extra and row["extra"] == "")):                               
                # Check if the mix matches the victim and aggressor
                info = extract_info(row["app_mix"])                
                # Get the bench name from the Python wrapper filename and load the python wrapper to get the full name and the input name
                victim_shortname = info["victim_wrapper"].split("/")[-1][:-3]                
                (victim_fn, victim_in) = get_input_and_full_name_from_args(info["victim_wrapper"], info["victim_args"])

                aggressors_found = False
                if "aggressor_wrapper" in info:
                    aggressor_shortname = info["aggressor_wrapper"].split("/")[-1][:-3]
                    (aggressor_fn, aggressor_in) = get_input_and_full_name_from_args(info["aggressor_wrapper"], info["aggressor_args"])
                    aggressors_found = (args.aggressors_info) and aggressor_shortname in aggressors_input and aggressor_in == aggressors_input[aggressor_shortname]
                else:
                    aggressors_found = (args.aggressors_info is None)

                # Check if aggressor and victim match the ones in the mix
                if (victim_shortname == victim_name) and (victim_in in victim_inputs) and aggressors_found:
                    if plot_trend_inputs:
                        key = victim_in
                        data_filename[key] = row["path"] + "/data.csv"
                    else:
                        key = aggressor_shortname
                        if aggressor_in != "":
                            key += ":" + aggressor_in
                        data_filename[key] = (row["path"] + "/data.csv", aggressor_fn)
                    victim_name_h = victim_fn
                    victim_fullname = victim_fn # Store the fullname to be used later as label
                    if victim_in != "":
                        victim_fullname += " (" + victim_in + ")"

    # At this point:
    #    - If we plot trend: data_filename is a dictionary where the keys are the victim input and the values are the data filename.
    #    - If we plot violin/box/etc data_filename is a dictionary where the keys are the aggressor name and input.
    #      The values are tuples with the data filename and the aggressor full name.
    if plot_trend_inputs:
        if(len(data_filename) != len(victim_inputs)):
            raise Exception("Error: could not find all the data files (or too much data has been found)")
    else:
        if(len(data_filename) != len(aggressors_input)):
            raise Exception("Error: could not find all the data files (or too much data has been found)")
    
    for metric in args.metrics.split(","):        
        outname = out_file_prefix + os.path.sep + metric.replace("/", "_")

        if plot_trend_inputs:
            # Lines
            df = load_data_trend(victim_inputs, data_filename, metric)
            plot_trend_line(df, victim_name_h, metric, outname, args.trend_limit)  
        else:
            df = load_data_distribution(args.aggressors_info, data_filename, metric)

            # Violins        
            plot_violin(df, victim_fullname, metric, outname)

            # Boxes        
            plot_box(df, victim_fullname, metric, outname)

            # Lines
            plot_line(df, victim_fullname, metric, outname)

if __name__=='__main__':
    main()