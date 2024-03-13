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
    parser.add_argument('-pt', '--plot_types', help='Types of plots to produce. Comma-separated list of "line", "box", "bar".', default="line,box,bar")
    parser.add_argument('-o', '--outfile', help='Path of output files.', required=True)

    args = parser.parse_args()

    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) + "/../" # Set BLINK_ROOT
    if not os.path.exists(args.outfile.lower()): 
        os.makedirs(args.outfile.lower())

    victim_names = args.victim_names.split(",")
    victim_inputs = args.victim_inputs.split(",")

    global_df_time = None

    for metric_hr in args.metrics.split(","):      
        global_df = pd.DataFrame()
        for vn in victim_names:
            outname = args.outfile + os.path.sep + metric_hr
            outname = outname.lower()
            for vi in victim_inputs:
                filename, victim_fn, aggressor_fn = get_data_filename(args.data_folder, args.system, args.numnodes, args.allocation_mode, args.allocation_split, args.ppn, args.extra, vn, vi, args.aggressor_name, args.aggressor_input)
                data = pd.DataFrame()
                if filename and os.path.exists(filename):                    
                    data[metric_hr] = get_bench_data(vn, vi, metric_hr, filename, args.ppn)
                else:
                    print("Data not found for metric " + metric_hr + " victim " + vn + " with input " + vi)
                    data[metric_hr] = [np.nan]

                if data.empty:
                    raise Exception("Error: data file " + filename + " does not contain data for metric " + metric_hr)
                data["Input"] = vi
                data["Application"] = victim_fn
                global_df = pd.concat([global_df, data], ignore_index=True)

        if metric_hr == "Runtime" and "gpubench" in args.victim_names:
            global_df_time = global_df


        plot_types = args.plot_types.split(",")
        #############
        # Line plot #
        #############
        if "line" in plot_types:
            # Setup the plot
            ax = sns.lineplot(data=global_df, x="Input", y=metric_hr, hue="Application", style="Application", markers=True, linewidth=3, markersize=8)

            # Plots the limit if specified
            if args.trend_limit:
                m, limit = args.trend_limit.split(":")
                if metric_hr == m:
                    ax.axhline(y=float(limit), color='black', linestyle='--')

            # Set the title and labels
            #ax.set_title(title)
            ax.set_xlabel("Input")
            ax.set_ylabel(add_unit_to_metric(metric_hr))
            if args.max_y:
                ax.set_ylim(0, float(args.max_y))
            else:
                ax.set_ylim(0, None)
            # Remove legend title
            ax.legend_.set_title(None)
            # Move legend outside
            sns.move_legend(ax, "lower center",
                            bbox_to_anchor=(.5, 1), ncol=2, title=None, frameon=False)

            # Inner latency plot
            if global_df_time is not None:
                ax2 = plt.axes([0.23, 0.6, .3, .2], facecolor='w')
                global_df_time["Runtime (us)"] = global_df_time["Runtime"] # Was already scaled before
                sns.lineplot(data=global_df_time, x="Input", y="Runtime (us)", hue="Application", style="Application", marker="o", ax=ax2)
                ax2.set_xlim([0, 5])
                ax2.set_ylim([0, 100])
                inner_fontsize = 9
                ax2.tick_params(labelsize=inner_fontsize)
                ax2.set_xlabel("")
                ax2.set_ylabel("Runtime (us)", fontdict={'fontsize': inner_fontsize})
                ax2.get_legend().remove()

            # Save to file
            #ax.figure.savefig(outname + "_line.png", bbox_inches='tight')
            ax.figure.savefig(outname + "_line.pdf", bbox_inches='tight')
            plt.clf()

        #########
        # Boxes #
        #########
        if "box" in plot_types:
            ax = sns.boxplot(data=global_df, x="Input", y=metric_hr, hue="Application")
            ax.figure.savefig(outname + "_box.pdf", bbox_inches='tight')
            plt.clf()

        ########
        # Bars #
        ########
        if "bar" in plot_types:
            ax = sns.barplot(data=global_df, x="Input", y=metric_hr, hue="Application")
            ax.figure.savefig(outname + "_bar.pdf", bbox_inches='tight')
            plt.clf()

if __name__=='__main__':
    main()