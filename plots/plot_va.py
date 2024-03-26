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
rcParams['figure.figsize'] = 8,4.5

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

extra_fullname["SL0"] = "Default SL"
extra_fullname["SL1"] = "Non-default SL"

extra_fullname["same_switch_SL1_hcoll0"] = "SL1\nHColl0"
extra_fullname["same_switch_SL1_hcoll1"] = "SL1\nHColl1"

extra_fullname["diff_group_RC_SL0"] = "SL0,RC"
extra_fullname["diff_group_RC_SL1"] = "SL1,RC"
extra_fullname["diff_group_UC_SL0"] = "SL0,UC"
extra_fullname["diff_group_UC_SL1"] = "SL1,UC"

extra_fullname["diff_group"] = "Diff.\nGroup"
extra_fullname["diff_switch"] = "Diff.\nSwitch"
extra_fullname["same_switch"] = "Same\nSwitch"

def main():
    parser=argparse.ArgumentParser(description='Plots the performance distribution for a specific victim/aggressor combination, for different extras.')
    parser.add_argument('-d', '--data_folder', help='Main data folder.', default="data")
    parser.add_argument('-s', '--system', help='System name.', required=True)
    parser.add_argument('-vn', '--victim_name', help='Victim name. It must match the filename of the Python wrapper.', required=True)
    parser.add_argument('-vi', '--victim_input', help='Victim input.', required=True)
    parser.add_argument('-an', '--aggressor_names', help='Names of the aggressor. It must match the filename of the Python wrapper.', default="")
    #parser.add_argument('-ai', '--aggressor_inputs', help='Aggressor inputs (one per aggressor).', default="")    
    parser.add_argument('-n', '--numnodes', help='The number of nodes the mix was executed on (total).', required=True)
    parser.add_argument('-am', '--allocation_modes', help='The allocation mode the mix was executed with (comma-separated string).', required=True)
    parser.add_argument('-sp', '--allocation_split', help='The allocation split the mix was executed with.', required=True)
    parser.add_argument('-e', '--extras', help='Extra info about the execution (comma-separated list).', required=True)
    parser.add_argument('-m', '--metric', help='Comma-separated string of metrics to plot.', default="0_Avg-Duration_s")
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



    data_dict = {}

    for allocation_mode in args.allocation_modes.split(","):                        
        for ea in args.extras.split(","):        
            global_df = pd.DataFrame()
            for aggressor in args.aggressor_names.split(","):    
                victim_input = get_actual_input_name(args.victim_input, args.metric)
                victim_name = get_actual_bench_name(args.victim_name, args.system, victim_input)                
                e = get_actual_extra_name(ea, args.system, victim_name, args.numnodes)
                allocation_split = args.allocation_split
                ppn = args.ppn            
                if victim_name == "ib_send_lat": # TODO: This is a hack, make it cleaner
                    if ppn != 1:
                        ppn = 1
                    if allocation_split == "100":
                        allocation_split = "50:50"            
                aggressor_input = get_default_aggressor_input(aggressor)
                filename = get_data_filename(args.data_folder, args.system, args.numnodes, allocation_mode, allocation_split, ppn, e, victim_name, victim_input, aggressor, aggressor_input)
                if not filename:
                    print("Data not found for extra " + e + " " + ea + " " + str(args))
                    continue
                data = pd.DataFrame()
                if not os.path.exists(filename):
                    print("Error: data file " + filename + " does not exist")
                    continue
                data[aggressor] = get_bench_data(victim_name, victim_input, args.metric, filename, ppn, args.numnodes, args.system)
                if data.empty:
                    raise Exception("Error: data file " + filename + " does not contain data for metric " + args.metric)
                global_df = pd.concat([global_df, data], axis=1)
            key = ea + "|" + allocation_mode
            data_dict[key] = global_df

    outname = args.outfile + os.path.sep
    outname = outname.lower()

    plot_types = args.plot_types.split(",")
    num_cols = len(args.extras.split(","))
    num_rows = len(args.allocation_modes.split(","))

    for plot_type in plot_types:
        axes = []
        fig = plt.figure()
        fig.subplots_adjust(hspace=0.4, wspace=0.4)
        index = 1
        for allocation_mode in args.allocation_modes.split(","):
            for extra in args.extras.split(","):     
                key = extra + "|" + allocation_mode
                if key not in data_dict:
                    print("Error: no data found for " + key)
                    continue
                global_df = data_dict[key]
                ax = fig.add_subplot(num_rows, num_cols, index)
                axes += [ax]

                # Violins        
                if plot_type == "violin":            
                    plot_violin(global_df, args.metric, ax)                
                    # Fix aspect
                    patch_violinplot(sns.color_palette(), len(args.aggressor_names.split(",")), ax)            
                elif plot_type == "box":
                    plot_box(global_df, args.metric, ax)
                elif plot_type == "boxnofliers":
                    plot_box(global_df, args.metric, ax, False)                      
                elif plot_type == "dist":
                    plot_dist(global_df, args.metric, ax)

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
                    ax.set_ylabel(get_human_readable_allocation_mode(allocation_mode) + "\n" + add_unit_to_metric(args.metric))
                else:
                    ax.set_ylabel("")
                
                # Set title only on the first row
                if index <= num_cols:
                    ax.set_title(extra_fullname[extra])            
                
                index += 1
        # Share y-axis
        for ax in axes[1:]:
            ax.sharey(axes[0])                
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