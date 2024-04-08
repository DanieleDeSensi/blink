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
from common import *

matplotlib.rc('pdf', fonttype=42) # To avoid issues with camera-ready submission
sns.set_style("whitegrid")
#sns.set_context("paper")
rcParams['figure.figsize'] = 10,4.5

extra_fullname = {}
extra_fullname["0-1"] = "0 to 1"
extra_fullname["0-2"] = "0 to 2"
extra_fullname["0-3"] = "0 to 3"
extra_fullname["0-4"] = "0 to 4"
extra_fullname["0-5"] = "0 to 5"
extra_fullname["0-6"] = "0 to 6"
extra_fullname["0-7"] = "0 to 7"

def main():
    parser=argparse.ArgumentParser(description='Plots the performance of benchmarks/applications for different inputs.')
    parser.add_argument('-d', '--data_folder', help='Main data folder.', default="data")
    parser.add_argument('-s', '--system', help='System name.', required=True)
    parser.add_argument('-vn', '--victim_names', help='Comma-separated list of victims. The names must match the filename of the Python wrapper.', required=True)
    parser.add_argument('-vi', '--victim_input', help='Input.', required=True)
    parser.add_argument('-an', '--aggressor_name', help='Name of the aggressor. It must match the filename of the Python wrapper.', default="")
    parser.add_argument('-ai', '--aggressor_input', help='Aggressor input.', default="")    
    parser.add_argument('-n', '--numnodes', help='The number of nodes the mix was executed on (total).', required=True)
    parser.add_argument('-am', '--allocation_mode', help='The allocation mode the mix was executed with.', required=True)
    parser.add_argument('-sp', '--allocation_split', help='The allocation split the mix was executed with.', required=True)
    parser.add_argument('-e', '--extras', help='Extra info about the execution (comma-separated list).', default="")
    parser.add_argument('-m', '--metrics', help='Comma-separated string of metrics to plot.', default="0_Avg-Duration_s")
    parser.add_argument('-p', '--ppn', help='Processes per node.')
    parser.add_argument('-tl', '--trend_limit', help='Y-axis upper limit. (format metric:limit:label) More of them comma-separated')
    parser.add_argument('-my', '--max_y', help='Max value on the y-axis')
    parser.add_argument('-pt', '--plot_types', help='Types of plots to produce. Comma-separated list of "line", "box", "bar".', default="line,box,bar")
    parser.add_argument('-ip', '--inner_pos', help='Positioning arguments for the inner plot.', default="[0.23, 0.6, .3, .2]")
    parser.add_argument('-iy', '--inner_ylim', help='Y-axis limits for the inner plot.', default="[0, 100]")
    parser.add_argument('-l', '--labels', help='Comma-separated list of labels.')
    parser.add_argument('--barplot_tops', help='')
    parser.add_argument('-o', '--outfile', help='Path of output files.', required=True)

    args = parser.parse_args()

    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) + "/../" # Set BLINK_ROOT
    if not os.path.exists(args.outfile.lower()): 
        os.makedirs(args.outfile.lower())

    victim_names = args.victim_names.split(",")
    global_df_time = None

    for metric_hr in args.metrics.split(","):      
        global_df = pd.DataFrame()
        for vn_a in victim_names:
            vn = get_actual_bench_name(vn_a, args.system, None)
            if args.ppn == "DEFAULT_MULTINODE":
                ppn = get_default_multinode_ppn(args.system, vn_a)
            else:
                ppn = int(args.ppn)
            allocation_split = args.allocation_split
            # TODO: This is a hack, make it cleaner
            if vn == "ib_send_lat":
                if ppn != 1:
                    ppn = 1
                if allocation_split == "100":
                    allocation_split = "50:50"

            outname = args.outfile + os.path.sep + metric_hr
            outname = outname.lower()
            for extra in args.extras.split(","):
                if metric_hr == "Goodput": # For bandwidth, we also get the data for runtime to plot the inner plot
                    actual_metrics = ["Runtime", "Goodput"]
                else:
                    actual_metrics = [metric_hr]

                for actual_metric in actual_metrics:
                    filename = get_data_filename(args.data_folder, args.system, args.numnodes, args.allocation_mode, allocation_split, ppn, get_actual_extra_name(extra, args.system, vn, args.numnodes), vn, args.victim_input, args.aggressor_name, args.aggressor_input)
                    data = pd.DataFrame()
                    if filename and os.path.exists(filename):                    
                        data[actual_metric] = get_bench_data(vn, args.victim_input, actual_metric, filename, ppn, args.numnodes, args.system)
                    else:
                        print("Data not found for metric " + actual_metric + " victim " + vn + " with input " + args.victim_input)
                        data[actual_metric] = [np.nan]

                    if data.empty:
                        raise Exception("Error: data file " + filename + " does not contain data for metric " + actual_metric)
                    data["Extra"] = extra_fullname[extra]
                    data["Application"] = vn # victim_fn
                    if metric_hr == "Goodput" and actual_metric == "Runtime": # Save the data for the inner plot in the bandwidth plots
                        global_df_time = pd.concat([global_df_time, data], ignore_index=True)
                    else:
                        global_df = pd.concat([global_df, data], ignore_index=True)

        plot_types = args.plot_types.split(",")
        if args.labels:
            index = 0
            labels = args.labels.split(",")
            for v in args.victim_names.split(","):
                global_df.replace(v, labels[index], inplace=True)
                index += 1
        else:
            labels = args.victim_names.split(",")


        errorbar = ("pi", 50) # From 25th to 75th percentile
        #############
        # Line plot #
        #############
        if "line" in plot_types:
            # Setup the plot
            ax = sns.lineplot(data=global_df, x="Extra", y=metric_hr, hue="Application", style="Application", markers=True, linewidth=3, markersize=8, hue_order=labels, errorbar=errorbar)

            # Plots the limit if specified
            if args.trend_limit:
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
                            ax.text(7, float(limit), label, fontsize=8, va='center', ha='center', backgroundcolor='w')

            # Set the title and labels
            #ax.set_title(title)
            ax.set_xlabel("")
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
                ax2 = plt.axes(ast.literal_eval(args.inner_pos), facecolor='w')
                global_df_time["Runtime (us)"] = global_df_time["Runtime"] # Was already scaled before
                sns.lineplot(data=global_df_time, x="Extra", y="Runtime (us)", hue="Application", style="Application", marker="o", ax=ax2, errorbar=errorbar)
                ax2.set_xlim([0, 5])
                ax2.set_ylim(ast.literal_eval(args.inner_ylim))
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
            ax = sns.boxplot(data=global_df, x="Extra", y=metric_hr, hue="Application", hue_order=labels)            
            ax.legend_.set_title(None)
            ax.set_xlabel("")
            ax.set_ylabel(add_unit_to_metric(metric_hr))
            ax.figure.savefig(outname + "_box.pdf", bbox_inches='tight')
            plt.clf()

        ########
        # Bars #
        ########
        if "bar" in plot_types:
            ax = sns.barplot(data=global_df, x="Extra", y=metric_hr, hue="Application", hue_order=labels)
            ax.legend_.set_title(None)
            ax.set_xlabel("")
            ax.set_ylabel(add_unit_to_metric(metric_hr))
            if args.barplot_tops:
                tops = args.barplot_tops.split(",")
                for i in range(len(tops)):
                    ax.hlines(y = int(tops[i]), xmin = i-0.4, xmax = i+0.4, color = 'black', linestyles='dashed')

            ax.figure.savefig(outname + "_bar.pdf", bbox_inches='tight')
            plt.clf()

if __name__=='__main__':
    main()