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
rcParams['figure.figsize'] = 8,4.5

def main():
    parser=argparse.ArgumentParser(description='Plots the performance of benchmarks/applications for different inputs.')
    parser.add_argument('-d', '--data_folder', help='Main data folder.', default="data")
    parser.add_argument('-s', '--system', help='System name.', required=True)
    parser.add_argument('-vn', '--victim_name', help='Victim name. The name must match the filename of the Python wrapper.', required=True)
    parser.add_argument('--victim_types', help="Victim types", required=True)
    parser.add_argument('-vi', '--victim_inputs', help='Comma-separated list of inputs.', required=True)
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
    parser.add_argument('--cbarlabel', help='Cbar label.', default="")

    args = parser.parse_args()

    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) + "/../" # Set BLINK_ROOT
    if not os.path.exists(args.outfile.lower()): 
        os.makedirs(args.outfile.lower())
    if args.victim_types.count(",") != 1:
        raise Exception("Error: The script only supports two types of victims for now.")
    
    xticklabels = []
    xticks = []
    sys = args.system
    index = 0
    ref_ppn = 0
    for metric_hr in args.metrics.split(","):      
        global_df = pd.DataFrame(columns=['Input', 'Nodes', '#GPUs', 'Metric'])
        for vin in args.victim_inputs.split(","):
            for nodes in args.numnodes.split(","):
                mean = 0
                for type in args.victim_types.split(","):   
                    vic = args.victim_name + "-" + type
                    vn = get_actual_bench_name(vic, sys, None)
                    if args.ppn == "DEFAULT_MULTINODE":
                        ppn = get_default_multinode_ppn(sys, vn)
                    else:
                        ppn = int(args.ppn)
                    
                    if ref_ppn == 0:
                        ref_ppn = ppn
                    if ref_ppn != ppn:
                        raise Exception("Error: Different ppn values are not supported for now.")
                    ranks = int(nodes)*int(ppn)                        

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

                    filename = get_data_filename(args.data_folder, sys, nodes, args.allocation_mode, allocation_split, ppn, get_actual_extra_name(args.extra, sys, vn, nodes), vn, vin, args.aggressor_name, args.aggressor_input)
                    data = pd.DataFrame()
                    if filename and os.path.exists(filename):                    
                        data[metric_hr] = get_bench_data(vn, vin, metric_hr, filename, ppn, nodes, sys)
                    else:
                        print("Data not found for metric " + metric_hr + " victim " + vn + " with input " + vin)
                        data[metric_hr] = [np.nan]

                    if data.empty:
                        raise Exception("Error: data file " + filename + " does not contain data for metric " + metric_hr)
                    
                    data[metric_hr] *= scaling_factor
                    if type == args.victim_types.split(",")[0]:
                        mean = data[metric_hr].mean()
                    else:
                        #print(f"Input {vin} Nodes {nodes} Type {type} Metric {metric_hr} Mean {mean} Data {data[metric_hr].mean()}")
                        mean = mean / data[metric_hr].mean() # Always first type divided by the second

                global_df.loc[index] = [vin, nodes, ranks, mean]
                index += 1
    

        outname = args.outfile + os.path.sep + metric_hr
        outname = outname.lower()

        x_labels = None
        if args.bw_per_node:
            x = "Nodes"
            x_labels = args.numnodes.split(",")
        else:
            x = "#GPUs"
            xticklabels = sorted(set(xticklabels))
            xticks = sorted(set(xticks))
            x_labels = []
            for n in args.numnodes.split(","):
                x_labels.append((int(n)*int(ref_ppn)))


        ###########
        # Heatmap #
        ###########
        df_pivot = global_df.pivot(index="Input", columns=x, values="Metric")
        df_pivot = df_pivot.reindex(labels=args.victim_inputs.split(","), level=0) # Sort "Input"
        df_pivot = df_pivot[x_labels] # Sort "Nodes"/"#GPUs"

        #ax = sns.heatmap(df_pivot, annot=True, fmt=".2f", vmin=0, center=1, cmap="RdYlGn_r", cbar_kws={'label': args.victim_types.split(",")[0] + "/" + args.victim_types.split(",")[1]})
        ax = sns.heatmap(df_pivot, annot=True, fmt=".2f", cmap='viridis', cbar_kws={'label': args.cbarlabel})
        ax.set_ylabel("")       
        ax.figure.savefig(outname + "_hm.pdf", bbox_inches='tight')
        plt.clf()

if __name__=='__main__':
    main()