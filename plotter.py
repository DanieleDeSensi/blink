#!/usr/bin/python3
import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import matplotlib.ticker as ticker
import os
import argparse
import csv

# Extracts victim and aggressor from an app mix file
# Assumes only two applications are specified, one of which is the victim and the other the aggressor
def extract_vic_agg(app_mix):
    vic_name = ""
    agg_name = ""
    with open(app_mix) as tb_file:
        lines = tb_file.readlines()
        sep = lines[0].strip()
        if lines[0] == "":
            sep = ","
        for appi in [1, 2]:
            end = lines[appi].strip().split(sep)[-1]
            benchname = lines[appi].strip().split(sep)[0].split("/")[-1][:-3]
            cmdline_fields = lines[appi].strip().split(sep)[1].split(" ")
            if "-msgsize" in cmdline_fields:
                msgsize = cmdline_fields[cmdline_fields.index('-msgsize') + 1]
            else:
                msgsize = "0"
            msgsize += "B"
            if end == "f": # Aggressor
                agg_name = benchname
                agg_msg_size = msgsize
            else:
                vic_name = benchname
                vic_msg_size = msgsize



def main():
    parser=argparse.ArgumentParser(description='Plots the results for a single app mix (violins, boxes, and timeseries), with and without congestion.')
    parser.add_argument('-d', '--data_folder', help='Main data folder.', default="data")
    parser.add_argument('-s', '--system', help='System name.', required=True)
    parser.add_argument('-v', '--victim_name', help='Victim name.', required=True)
    parser.add_argument('-vs', '--victim_size', help='Victim data size.', required=True)
    parser.add_argument('-a', '--aggressor_name', help='Aggressor name.', required=True)
    parser.add_argument('-as', '--aggressor_size', help='Aggressor size.', required=True)
    parser.add_argument('-n', '--num_nodes', help='The number of nodes the mix was executed on (total).', required=True)
    parser.add_argument('-am', '--allocation_mode', help='The allocation mode the mix was executed with.', required=True)
    parser.add_argument('-sp', '--allocation_split', help='The allocation split the mix was executed with.', required=True)
    parser.add_argument('-e', '--extra', help='Any extra info about the execution.', default="")

    args = parser.parse_args()

    out_folder = "./plots" + os.path.sep + args.system + os.path.sep
    out_file_prefix = args.victim_name + "_" + args.victim_size + "_" + args.aggressor_name + "_" + args.aggressor_size + "_" + args.num_nodes + "_" + args.allocation_mode + "_" + args.allocation_split
    if args.extra:
        out_file_prefix += "_" + args.extra
    
    isolated_mix = args.victim_name + "_" + args.victim_size + "_null_dummy"
    congested_mix = args.victim_name + "_" + args.victim_size + "_" + args.aggressor_name + "_" + args.aggressor_size

    isolated_dict = {}
    congested_dict = {}
    
    with open(args.data_folder + "/description.csv", mode='r') as infile:
        reader = csv.DictReader(infile)    
        for line in reader:
            row = {key: value for key, value in line.items()}
            print(row)
            if (row["app_mix"] == isolated_mix or row["app_mix"] == congested_mix) and \
                row["path"].split("/")[0] == args.system and row["numnodes"] == args.num_nodes and \
                row["allocation_mode"] == args.allocation_mode and row["allocation_split"] == args.allocation_split and \
                row["extra"] == args.extra:
                
                if(row["app_mix"] == isolated_mix):
                    isolated_dict = row
                else:
                    congested_dict = row
            #description += [row]

    print(isolated_dict)
    print(congested_dict)

if __name__=='__main__':
    main()