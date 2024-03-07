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
    parser.add_argument('-o', '--outfile', help='Path of output files.', required=True)

    args = parser.parse_args()   
    
    os.environ["BLINK_ROOT"] = os.path.dirname(os.path.abspath(__file__)) + "/../" # Set BLINK_ROOT
    if not os.path.exists(args.outfile): 
        os.makedirs(args.outfile)

    for metric in args.metrics.split(","):                
        global_df = pd.DataFrame()
        for e in args.extras.split(","):            
            filename, victim_fn, aggressor_fn = get_data_filename(args.data_folder, args.system, args.numnodes, args.allocation_mode, args.allocation_split, args.ppn, e, args.victim_name, args.victim_input, args.aggressor_name, args.aggressor_input)
            if not filename:
                raise Exception("Data not found for extra " + e)
            data = pd.DataFrame()
            data[e] = pd.read_csv(filename)[metric]
            if data.empty:
                raise Exception("Error: data file " + filename + " does not contain data for metric " + metric)
            global_df = pd.concat([global_df, data], axis=1)
            print(filename)

        
        outname = args.outfile + os.path.sep + metric.replace("/", "_")

        print(global_df)
        # Violins        
        plot_violin(global_df, victim_fn, metric, outname, args.max_y)

        # Boxes        
        plot_box(global_df, victim_fn, metric, outname, args.max_y)

        # Lines
        plot_line(global_df, victim_fn, metric, outname, args.max_y)

if __name__=='__main__':
    main()