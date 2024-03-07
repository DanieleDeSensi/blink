#!/usr/bin/python3
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
from common import *

matplotlib.rc('pdf', fonttype=42) # To avoid issues with camera-ready submission


def main():
    parser=argparse.ArgumentParser(description='Plots the performance of benchmarks/applications for different inputs.')
    parser.add_argument('-d', '--data_folder', help='Main data folder.', default="data")
    parser.add_argument('-s', '--system', help='System name.', required=True)
    parser.add_argument('-vn', '--victim_names', help='Comma-separated list of victims. The names must match the filename of the Python wrapper.', required=True)
    parser.add_argument('-vi', '--victim_inputs', help='Comma-separated list of inputs.', required=True)
    parser.add_argument('-an', '--aggressor_name', help='Name of the aggressor. It must match the filename of the Python wrapper.', default="")
    parser.add_argument('-ai', '--aggressor_input', help='Aggressor input.', default="")    
    parser.add_argument('-n', '--numnodes', help='The number of nodes the mix was executed on (total).', required=True)
    parser.add_argument('-am', '--allocation_mode', help='The allocation mode the mix was executed with.', required=True)
    parser.add_argument('-sp', '--allocation_split', help='The allocation split the mix was executed with.', required=True)
    parser.add_argument('-e', '--extra', help='Extra info about the execution.', default="")
    parser.add_argument('-m', '--metrics', help='Comma-separated string of metrics to plot.', default="0_Avg-Duration_s")
    parser.add_argument('-p', '--ppn', help='Processes per node.', default=1)
    parser.add_argument('-tl', '--trend_limit', help='Y-axis upper limit. (format metric:limit)')
    parser.add_argument('-my', '--max_y', help='Max value on the y-axis')
    parser.add_argument('-o', '--outfile', help='Path of output files.', required=True)

    args = parser.parse_args()

    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) + "/../" # Set BLINK_ROOT
    if not os.path.exists(args.outfile): 
        os.makedirs(args.outfile)

    victim_names = args.victim_names.split(",")
    victim_inputs = args.victim_inputs.split(",")

    for metric in args.metrics.split(","):        
        outname = args.outfile + os.path.sep + metric.replace("/", "_")
        global_df = pd.DataFrame()
        for vn in victim_names:
            for vi in victim_inputs:
                filename, victim_fn, aggressor_fn = get_data_filename(args.data_folder, args.system, args.numnodes, args.allocation_mode, args.allocation_split, args.ppn, args.extra, vn, vi, args.aggressor_name, args.aggressor_input)
                data = pd.DataFrame()
                if filename and os.path.exists(filename):                    
                    data[metric] = pd.read_csv(filename)[metric]
                else:
                    print("Data not found for metric " + metric + " victim " + vn + " with input " + vi)
                    data[metric] = [np.nan]

                if data.empty:
                    raise Exception("Error: data file " + filename + " does not contain data for metric " + metric)
                data["Input"] = vi
                data["Application"] = victim_fn
                global_df = pd.concat([global_df, data], ignore_index=True)

        # Setup the plot
        ax = sns.lineplot(data=global_df, x="Input", y=metric, hue="Application", style="Application", marker="o")

        # Plots the limit if specified
        if args.trend_limit:
            m, limit = args.trend_limit.split(":")
            if metric == m:
                ax.axhline(y=float(limit), color='black', linestyle='--')

        # Set the title and labels
        #ax.set_title(title)
        ax.set_xlabel("Input")
        ax.set_ylabel(metric_to_human_readable(metric))
        if args.max_y:
            ax.set_ylim(0, float(args.max_y))

        # Save to file
        #ax.figure.savefig(outname + "_lines.png", bbox_inches='tight')
        ax.figure.savefig(outname + "_lines.pdf", bbox_inches='tight')
        plt.clf()

if __name__=='__main__':
    main()