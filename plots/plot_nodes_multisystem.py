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
import ast
import math
from common import *

matplotlib.rc('pdf', fonttype=42) # To avoid issues with camera-ready submission
sns.set_style("whitegrid")
#sns.set_context("paper")
#rcParams['figure.figsize'] = 12,4.5
rcParams['figure.figsize'] = 8,4.5

def main():
    parser=argparse.ArgumentParser(description='Plots the performance of benchmarks/applications for different inputs.')
    parser.add_argument('-d', '--data_folder', help='Main data folder.', default="data")
    parser.add_argument('-s', '--systems', help='System name.', required=True)
    parser.add_argument('-vn', '--victim_names', help='Comma-separated list of victims. The names must match the filename of the Python wrapper.', required=True)
    parser.add_argument('-vi', '--victim_input', help='Comma-separated list of inputs.', required=True)
    parser.add_argument('-an', '--aggressor_name', help='Name of the aggressor. It must match the filename of the Python wrapper.', default="")
    parser.add_argument('-ai', '--aggressor_input', help='Aggressor input.', default="")    
    parser.add_argument('-n', '--numnodes', help='The number of nodes the mix was executed on (total).', required=True)
    parser.add_argument('-am', '--allocation_mode', help='The allocation mode the mix was executed with.', required=True)
    parser.add_argument('-sp', '--allocation_split', help='The allocation split the mix was executed with.', required=True)
    parser.add_argument('-e', '--extra', help='Extra info about the execution.', default="")
    parser.add_argument('-m', '--metrics', help='Comma-separated string of metrics to plot.', default="0_Avg-Duration_s")
    parser.add_argument('-p', '--ppn', help='Processes per node.')
    parser.add_argument('-tl', '--trend_limit', help='Y-axis upper limit. (format metric:limit:label) More of them comma-separated')
    parser.add_argument('-my', '--max_y', help='Max value on the y-axis')
    parser.add_argument('-pt', '--plot_types', help='Types of plots to produce. Comma-separated list of "line", "box", "bar".', default="line,box,bar")
    parser.add_argument('-o', '--outfile', help='Path of output files.', required=True)
    parser.add_argument('--bw_per_node', help='Report bandwidth per-node rather than per-rank.', action='store_true')
    parser.add_argument('--errorbar', help='Error bar.', default="(\"ci\", 90)")

    args = parser.parse_args()

    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) + "/../" # Set BLINK_ROOT
    if not os.path.exists(args.outfile.lower()): 
        os.makedirs(args.outfile.lower())

    global_df_time = None
    xticklabels = []
    xticks = []
    ideal_label_pos = 256
    available_dashes = ["", (2, 2)]
    for metric_hr in args.metrics.split(","):      
        palette = {}
        dashes = {}
        palette_idx = 0
        dashes_idx = 0
        global_df = pd.DataFrame()
        global_df_ideal = pd.DataFrame()
        ideals = {}
        for sys in args.systems.split(","):
            for vic in args.victim_names.split(","):
                vn = get_actual_bench_name(vic, sys, None)
                if args.ppn == "DEFAULT_MULTINODE":
                    ppn = get_default_multinode_ppn(sys, vn)
                else:
                    ppn = int(args.ppn)

                scaling_factor = 1
                if args.bw_per_node and metric_hr == "Bandwidth":
                    scaling_factor = ppn
                
                allocation_split = args.allocation_split
                # TODO: This is a hack, make it cleaner
                if vn == "ib_send_lat":
                    if ppn != 1:
                        ppn = 1
                    if allocation_split == "100":
                        allocation_split = "50:50"

                outname = args.outfile + os.path.sep + metric_hr
                outname = outname.lower()
                
                matches   = [value for key, value in palette.items() if system_to_human_readable(sys) in key]
                matches_d = [value for key, value in dashes.items() if bench_to_human_readable_impl(vn) in key]
                if len(matches):
                    palette[system_to_human_readable(sys) + " (" + bench_to_human_readable_impl(vn) + ")"] = matches[0]                    
                else:
                    palette[system_to_human_readable(sys) + " (" + bench_to_human_readable_impl(vn) + ")"] = sns.color_palette()[palette_idx]                    
                    palette_idx += 1
            
                if len(matches_d):
                    dashes[system_to_human_readable(sys) + " (" + bench_to_human_readable_impl(vn) + ")"] = matches_d[0]
                else:
                    dashes[system_to_human_readable(sys) + " (" + bench_to_human_readable_impl(vn) + ")"] = available_dashes[dashes_idx]
                    dashes_idx += 1

                for nodes in args.numnodes.split(","):
                    if metric_hr == "Bandwidth": # For bandwidth, we also get the data for runtime to plot the inner plot
                        actual_metrics = ["Runtime", "Bandwidth"]
                    else:
                        actual_metrics = [metric_hr]                    

                    # Compute trend limit, assuming alltoall
                    if args.trend_limit == "AUTO" and metric_hr == "Bandwidth" and not args.bw_per_node:
                        data_ideal = pd.DataFrame(["System", "Nodes", "#GPUs", metric_hr])
                        data_ideal["System"] = system_to_human_readable(sys) + " (Expected)"
                        data_ideal["Nodes"] = nodes
                        data_ideal["#GPUs"] = int(nodes)*int(ppn)
                        if sys == "alps":
                            data_ideal[metric_hr] = 200 * ((int(nodes)*int(ppn) - 1) / ((int(nodes) - 1)*int(ppn)))
                        elif sys == "leonardo":
                            data_ideal[metric_hr] = 100 * ((int(nodes)*int(ppn) - 1) / ((int(nodes) - 1)*int(ppn)))
                        elif sys == "lumi":
                            data_ideal[metric_hr] = 100 * ((int(nodes)*int(ppn) - 1) / ((int(nodes) - 1)*int(ppn)))
                        global_df_ideal = pd.concat([global_df_ideal, data_ideal], ignore_index=True)             
                        if nodes == str(ideal_label_pos):
                            ideals[sys] = data_ideal[metric_hr].values[0]              
                    for actual_metric in actual_metrics:
                        filename = get_data_filename(args.data_folder, sys, nodes, args.allocation_mode, allocation_split, ppn, get_actual_extra_name(args.extra, sys, vn, nodes), vn, args.victim_input, args.aggressor_name, args.aggressor_input)
                        data = pd.DataFrame()
                        if filename and os.path.exists(filename):                    
                            data[actual_metric] = get_bench_data(vn, args.victim_input, actual_metric, filename, ppn, nodes, sys)
                        else:
                            #print("Data not found for metric " + actual_metric + " victim " + vn + " with input " + args.victim_input)
                            data[actual_metric] = [np.nan]

                        if data.empty:
                            raise Exception("Error: data file " + filename + " does not contain data for metric " + actual_metric)
                        
                        data[actual_metric] *= scaling_factor
                        data["Nodes"] = nodes
                        data["#GPUs"] = int(nodes)*int(ppn)
                        data["System"] = system_to_human_readable(sys) + " (" + bench_to_human_readable_impl(vn) + ")"

                        if not args.bw_per_node:
                            xticklabels += [(int(nodes)*int(ppn))]
                            xticks += [(int(nodes)*int(ppn))]

                        if not args.bw_per_node and int(nodes)*int(ppn) < 16: # Start from 16 GPUs (at least two nodes for LUMI)
                            continue

                        if metric_hr == "Bandwidth" and actual_metric == "Runtime": # Save the data for the inner plot in the bandwidth plots
                            global_df_time = pd.concat([global_df_time, data], ignore_index=True)
                        else:
                            global_df = pd.concat([global_df, data], ignore_index=True)
                    
        plot_types = args.plot_types.split(",")

        if args.bw_per_node:
            x = "Nodes"
        else:
            x = "#GPUs"
            xticklabels = sorted(set(xticklabels))
            xticks = sorted(set(xticks))
        #############
        # Line plot #
        #############
        errorbar = ast.literal_eval(args.errorbar)
        if "line" in plot_types:
            # Setup the plot
            #print(dashes)
            ax = sns.lineplot(data=global_df, x=x, y=metric_hr, hue="System", style="System", markers=True, linewidth=4, markersize=10, errorbar=errorbar, palette=palette, dashes=dashes)

            # Plots the limit if specified
            if args.trend_limit != "AUTO":
                for limit_str in args.trend_limit.split(","):
                    if limit_str.count(":") == 1:
                        m, limit = limit_str.split(":")
                        label = None
                    else:
                        m, limit, label = limit_str.split(":")
                    if metric_hr == m:
                        ax.axhline(y=float(limit), color='black', linestyle='--')
                        if label:
                            label = label.replace("_", " ") 
                            ax.text(256, float(limit), label, fontsize=8, va='center', ha='center', backgroundcolor='w')
            else:
                ax = sns.lineplot(data=global_df_ideal, x=x, y=metric_hr, hue="System", style="System", markers=False, linewidth=1.5, linestyle='--', color='black')
                for sys in args.systems.split(","):
                    label = system_to_human_readable(sys) + " (Expected)"
                    print(ideals)
                    ax.text(ideal_label_pos, ideals[sys], label, fontsize=8, va='center', ha='center', backgroundcolor='w')

            # Set the title and labels
            #ax.set_title(title)
            #ax.set_xlabel("")
            ax.set_ylabel(add_unit_to_metric(metric_hr))
            if args.max_y:
                ax.set_ylim(0, float(args.max_y))
            else:
                ax.set_ylim(0, None)
            # Remove legend title
            ax.legend_.set_title(None)
        
            if not args.bw_per_node:
                plt.xscale('log')
                ax.set_xticks(xticks)
                ax.set_xticklabels(xticklabels)                

            # Move legend outside
            sns.move_legend(ax, "lower center",
                            bbox_to_anchor=(.5, 1), ncol=3, title=None, frameon=False)

            # Save to file
            #ax.figure.savefig(outname + "_line.png", bbox_inches='tight')
            ax.figure.savefig(outname + "_line.pdf", bbox_inches='tight')
            plt.clf()

        #########
        # Boxes #
        #########
        if "box" in plot_types:
            ax = sns.boxplot(data=global_df, x=x, y=metric_hr, hue="System")
            ax.figure.savefig(outname + "_box.pdf", bbox_inches='tight')
            plt.clf()

        ########
        # Bars #
        ########
        if "bar" in plot_types:
            ax = sns.barplot(data=global_df, x=x, y=metric_hr, hue="System")
            ax.figure.savefig(outname + "_bar.pdf", bbox_inches='tight')
            plt.clf()

if __name__=='__main__':
    main()