#!/usr/bin/python3
import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
from matplotlib import rcParams
import matplotlib.ticker as ticker
import os
import argparse
import csv
import importlib.util
from common import *

matplotlib.rc('pdf', fonttype=42) # To avoid issues with camera-ready submission
sns.set_style("whitegrid")
#sns.set_context("paper")
rcParams['figure.figsize'] = 8,4.5

extra_fullname = {}
extra_fullname["same_switch_AR0"] = "Same switch\nStat. Rout."
extra_fullname["same_switch_AR1"] = "Same switch\nAdp. Rout."
extra_fullname["diff_switch_AR0"] = "Diff. switch\nStat. Rout."
extra_fullname["diff_switch_AR1"] = "Diff. switch\nAdp. Rout."
extra_fullname["diff_groups_AR0"] = "Diff. group\nStat. Rout."
extra_fullname["diff_groups_AR1"] = "Diff. group\nAdp. Rout."

extra_fullname["same_switch_SL0"] = "Same switch\nSL0"
extra_fullname["same_switch_SL1"] = "Same switch\nSL1"
extra_fullname["diff_switch_SL0"] = "Diff. switch\nSL0"
extra_fullname["diff_switch_SL1"] = "Diff. switch\nSL1"
extra_fullname["diff_group_SL0"] = "Diff. group\nSL0"
extra_fullname["diff_group_SL1"] = "Diff. group\nSL1"

extra_fullname["SL0VAR0"] = "SL0VAR0"
extra_fullname["SL0VAR1"] = "SL0VAR1"
extra_fullname["SL1VAR0"] = "SL1VAR0"
extra_fullname["SL1VAR1"] = "SL1VAR1"

extra_fullname["SL0"] = "SL0"
extra_fullname["SL1"] = "SL1"

def main():
    parser=argparse.ArgumentParser(description='Plots the performance distribution for a specific victim/aggressor combination, for different extras.')
    parser.add_argument('-d', '--data_folder', help='Main data folder.', default="data")
    parser.add_argument('-s', '--system', help='System name.', required=True)
    parser.add_argument('-vn', '--victim_name', help='Victim name. It must match the filename of the Python wrapper.', required=True)
    parser.add_argument('-vi', '--victim_input', help='Victim input.', required=True)
    parser.add_argument('-an', '--aggressor_name', help='Name of the aggressor. It must match the filename of the Python wrapper.', default="")
    parser.add_argument('-ai', '--aggressor_input', help='Aggressor input.', default="")    
    parser.add_argument('-n', '--numnodes', help='The number of nodes the mix was executed on (total).', required=True)
    parser.add_argument('-am', '--allocation_mode', help='The allocation mode the mix was executed with.', required=True)
    parser.add_argument('-sp', '--allocation_split', help='The allocation split the mix was executed with.', required=True)
    parser.add_argument('-e', '--extras', help='Extra info about the execution (comma-separated list).', required=True)
    parser.add_argument('-m', '--metrics', help='Comma-separated string of metrics to plot.', default="0_Avg-Duration_s")
    parser.add_argument('-p', '--ppn', help='Processes per node.', default=1)
    parser.add_argument('-my', '--max_y', help='Max value on the y-axis')
    parser.add_argument('-pt', '--plot_types', help='Types of plots to produce. Comma-separated list of "violin", "box", "line", "dist".', default="violin,box,line,dist")
    parser.add_argument('-o', '--outfile', help='Path of output files.', required=True)

    args = parser.parse_args()   
    
    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) + "/../" # Set BLINK_ROOT
    if not os.path.exists(args.outfile): 
        os.makedirs(args.outfile.lower())

    for metric_hr in args.metrics.split(","):                
        global_df = pd.DataFrame()
        for e in args.extras.split(","):            
            filename, victim_fn, aggressor_fn = get_data_filename(args.data_folder, args.system, args.numnodes, args.allocation_mode, args.allocation_split, args.ppn, e, args.victim_name, args.victim_input, args.aggressor_name, args.aggressor_input)
            if not filename:
                print("Data not found for extra " + e + " " + str(args))
                continue
            data = pd.DataFrame()
            if not os.path.exists(filename):
                print("Error: data file " + filename + " does not exist")
                continue
            data[extra_fullname[e]] = get_bench_data(args.victim_name, args.victim_input, metric_hr, filename, args.ppn)
            if data.empty:
                raise Exception("Error: data file " + filename + " does not contain data for metric " + metric_hr)
            global_df = pd.concat([global_df, data], axis=1)

        
        outname = args.outfile + os.path.sep + metric_hr
        outname = outname.lower()

        plot_types = args.plot_types.split(",")
        # Violins        
        if "violin" in plot_types:            
            plot_violin(global_df, victim_fn, metric_hr, outname, args.max_y)

        # Boxes, with and without outliers        
        if "box" in plot_types:
            plot_box(global_df, victim_fn, metric_hr, outname, args.max_y, True)
            plot_box(global_df, victim_fn, metric_hr, outname, args.max_y, False)

        # Lines
        if "line" in plot_types:
            plot_line(global_df, victim_fn, metric_hr, outname, args.max_y)

        ## Dist
        if "dist" in plot_types:
            plot_dist(global_df, victim_fn, metric_hr, outname, args.max_y)

if __name__=='__main__':
    main()