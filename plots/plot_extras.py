#!/usr/bin/env python3
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
rcParams['figure.figsize'] = 12,4.5

extra_fullname = {}
extra_fullname["same_switch_AR0"] = "Same switch\nStat. Rout."
extra_fullname["same_switch_AR1"] = "Same switch\nAdp. Rout."
extra_fullname["diff_switch_AR0"] = "Diff. switch\nStat. Rout."
extra_fullname["diff_switch_AR1"] = "Diff. switch\nAdp. Rout."
extra_fullname["diff_groups_AR0"] = "Diff. group\nStat. Rout."
extra_fullname["diff_groups_AR1"] = "Diff. group\nAdp. Rout."

extra_fullname["same_switch_SL0"] = "Same\nSwitch\nSL0"
extra_fullname["same_switch_SL1"] = "Same\nSwitch\nSL1"
extra_fullname["diff_switch_SL0"] = "Diff.\nSwitch\nSL0"
extra_fullname["diff_switch_SL1"] = "Diff.\nSwitch\nSL1"
extra_fullname["diff_group_SL0"] = "Diff.\nGroup\nSL0"
extra_fullname["diff_group_SL1"] = "Diff.\nGroup\nSL1"

extra_fullname["same_switch_RC_SL0"] = "Same\nSwitch\nSL0"
extra_fullname["same_switch_RC_SL1"] = "Same\nSwitch\nSL1"
extra_fullname["diff_switch_RC_SL0"] = "Diff.\nSwitch\nSL0"
extra_fullname["diff_switch_RC_SL1"] = "Diff.\nSwitch\nSL1"
extra_fullname["diff_group_RC_SL0"] = "Diff.\nGroup\nSL0"
extra_fullname["diff_group_RC_SL1"] = "Diff.\nGroup\nSL1"

extra_fullname["SL0VAR0"] = "SL0VAR0"
extra_fullname["SL0VAR1"] = "SL0VAR1"
extra_fullname["SL1VAR0"] = "SL1VAR0"
extra_fullname["SL1VAR1"] = "SL1VAR1"

extra_fullname["SL0"] = "SL0"
extra_fullname["SL1"] = "SL1"

extra_fullname["same_switch_SL1_hcoll0"] = "SL1\nHColl0"
extra_fullname["same_switch_SL1_hcoll1"] = "SL1\nHColl1"

extra_fullname["diff_group_RC_SL0"] = "SL0,RC"
extra_fullname["diff_group_RC_SL1"] = "SL1,RC"
extra_fullname["diff_group_UC_SL0"] = "SL0,UC"
extra_fullname["diff_group_UC_SL1"] = "SL1,UC"

extra_fullname["diff_group"] = "Diff.\nGroup"
extra_fullname["diff_switch"] = "Diff.\nSwitch"
extra_fullname["same_switch"] = "Same\nSwitch"

extra_fullname["0-1"] = "0->1"
extra_fullname["0-2"] = "0->2"
extra_fullname["0-3"] = "0->3"
extra_fullname["0-4"] = "0->4"
extra_fullname["0-5"] = "0->5"
extra_fullname["0-6"] = "0->6"
extra_fullname["0-7"] = "0->7"

extra_fullname["diff_group_TC_BE"] = "Best Effort"
extra_fullname["diff_group_TC_LL"] = "Low Latency"


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
    parser.add_argument('-my', '--max_y', help='Max value on the y-axis. Comma separated list (one element for each metric/system combination). E.g.: "leonardo|bandwidth=100,lumi|latency=10"')
    parser.add_argument('-pt', '--plot_types', help='Types of plots to produce. Comma-separated list of "violin", "box", "line", "dist".', default="violin,box,boxnofliers,line,dist")
    parser.add_argument('-o', '--outfile', help='Path of output files.', required=True)
    parser.add_argument('--xticklabels', help='xticklabels.', required=False)
    parser.add_argument('--title', help='title', required=False)

    args = parser.parse_args()   
    
    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) + "/../" # Set BLINK_ROOT
    if not os.path.exists(args.outfile.lower()): 
        os.makedirs(args.outfile.lower())

    if args.aggressor_name and not args.aggressor_input:
        aggressor_input = get_default_aggressor_input(args.aggressor_name)
    else:
        aggressor_input = args.aggressor_input

    data_dict = {}

    for metric_hr in args.metrics.split(","):                        
        for system in args.system.split(","):
            global_df = pd.DataFrame()
            for ea in args.extras.split(","):
                victim_input = get_actual_input_name(args.victim_input, metric_hr)
                victim_name = get_actual_bench_name(args.victim_name, system, victim_input)                
                e = get_actual_extra_name(ea, system, victim_name, args.numnodes)
                allocation_split = args.allocation_split
                ppn = args.ppn            
                if victim_name == "ib_send_lat": # TODO: This is a hack, make it cleaner
                    if ppn != 1:
                        ppn = 1
                    if allocation_split == "100":
                        allocation_split = "50:50"            
                filename = get_data_filename(args.data_folder, system, args.numnodes, args.allocation_mode, allocation_split, ppn, e, victim_name, victim_input, args.aggressor_name, aggressor_input)
                if not filename:
                    print("Data not found for extra " + e + " " + ea + " " + str(args))
                    continue
                data = pd.DataFrame()
                if not os.path.exists(filename):
                    print("Error: data file " + filename + " does not exist")
                    continue
                data[extra_fullname[e]] = get_bench_data(victim_name, victim_input, metric_hr, filename, ppn, args.numnodes, system)
                if data.empty:
                    raise Exception("Error: data file " + filename + " does not contain data for metric " + metric_hr)
                global_df = pd.concat([global_df, data], axis=1)
            key = system + "|" + metric_hr
            data_dict[key] = global_df

    outname = args.outfile + os.path.sep
    outname = outname.lower()

    plot_types = args.plot_types.split(",")
    num_cols = len(args.system.split(","))
    num_rows = len(args.metrics.split(","))

    for plot_type in plot_types:
        fig = plt.figure()
        fig.subplots_adjust(hspace=0.4, wspace=0.4)
        index = 1
        for metric in args.metrics.split(","):
            for system in args.system.split(","):     
                key = system + "|" + metric
                if key not in data_dict:
                    print("Error: no data found for " + key)
                    continue
                global_df = data_dict[key]
                ax = fig.add_subplot(num_rows, num_cols, index)

                # Violins        
                if plot_type == "violin":            
                    plot_violin(global_df, metric, ax)                
                    # Fix aspect
                    patch_violinplot(sns.color_palette(), len(args.extras.split(",")), ax)            
                elif plot_type == "box":
                    plot_box(global_df, metric, ax)
                elif plot_type == "boxnofliers":
                    plot_box(global_df, metric, ax, False)                      
                elif plot_type == "dist":
                    plot_dist(global_df, metric, ax)

                if plot_type != "dist":
                    # No limits for box with fliers
                    if plot_type != "box":
                        if args.max_y and key in args.max_y:
                            max_y = 0
                            for st in args.max_y.split(","):
                                sysmet, max_y = st.split(":")
                                if sysmet == key:
                                    break
                            ax.set_ylim(0, float(max_y))
                    # Remove xticklabels for all but the last row
                    if index <= (num_rows - 1) * num_cols:
                        ax.set_xticklabels("")      
                    elif args.xticklabels:
                        ax.set_xticklabels(ast.literal_eval(args.xticklabels))                  
                
                # Remove y-axis label for all but the first column
                if index % num_cols == 1:
                    ax.set_ylabel(add_unit_to_metric(metric))
                else:
                    ax.set_ylabel("")
                
                # Set title only on the first row
                if index <= num_cols:
                    ax.set_title(system_to_human_readable(system))            
                
                index += 1
        # Save to file
        fig.savefig(outname + plot_type + ".pdf", bbox_inches='tight')
        plt.clf()      

    '''
    # Boxes, with and without outliers        
    if "box" in plot_types:
        plot_box(global_df, metric_hr, outname, args.max_y, args.xticklabels, args.title, True)
        plot_box(global_df, metric_hr, outname, args.max_y, args.xticklabels, args.title, False)

    # Lines
    if "line" in plot_types:
        plot_line(global_df, metric_hr, outname, args.max_y, args.xticklabels, args.title)

    ## Dist
    if "dist" in plot_types:
        plot_dist(global_df, metric_hr, outname, args.max_y, args.xticklabels, args.title)
    '''

if __name__=='__main__':
    main()