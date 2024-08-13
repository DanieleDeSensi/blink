import os
import itertools
import subprocess
import statistics
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

def format_bytes_binary(b):
    GiB = 2**30  # 1 GiB = 2^30 bytes
    MiB = 2**20  # 1 MiB = 2^20 bytes
    KiB = 2**10  # 1 KiB = 2^10 bytes

    if b >= GiB:
        return f"{b / GiB:.2f} GiB"
    elif b >= MiB:
        return f"{b / MiB:.2f} MiB"
    elif b >= KiB:
        return f"{b / KiB:.2f} KiB"
    else:
        return f"{b} B"

def parse_bytes_binary(formatted_str):
    # Strip any leading or trailing whitespace from the input
    formatted_str = formatted_str.strip().upper()
    
    # Define conversion factors
    GiB = 2**30
    MiB = 2**20
    KiB = 2**10
    
    # Check for GiB
    if "GIB" in formatted_str:
        value = float(formatted_str.replace("GIB", "").strip())
        return int(value * GiB)
    
    # Check for MiB
    elif "MIB" in formatted_str:
        value = float(formatted_str.replace("MIB", "").strip())
        return int(value * MiB)
    
    # Check for KiB
    elif "KIB" in formatted_str:
        value = float(formatted_str.replace("KIB", "").strip())
        return int(value * KiB)
    
    # Check for B (bytes)
    elif "B" in formatted_str:
        value = float(formatted_str.replace("B", "").strip())
        return int(value)
    
    # If none of the above, raise an error
    else:
        print("ERROR!!!")
        raise ValueError("Invalid format. The input should end with GiB, MiB, KiB, or B.")


def read_data(file_path):
    data = []
    lables = []
    with open(file_path, 'r') as file:
        # Skip the header line
        #next(file)
        i = 0
        for line in file:
            # Skip lines starting with #
            if line.startswith('#'):
                continue
            
            # Split each line by comma and convert to floats
            if i == 0:
                lables   = list(map(str, line.strip().split(',')))
            else:
                elements = list(map(float, line.strip().split(',')))
               
                tmp=[]
                for j in range(0,len(elements)):
                    tmp.append(elements[j])
                data.append(tmp)
                #print("tmp has type ", type(tmp))
            i = i + 1
    return lables, data


def parse_infoline(infoline):
    # Remove the leading "# " and split by commas
    parts = infoline[2:].split(", ")

    parsed = {}
    for part in parts:
        key, value = part.split(": ")
        parsed[key] = value

    # Extract the variables
    system = parsed["ststem"].strip()
    experiment = parsed["experiment"].strip()
    nnodes = int(parsed["nnodes"].strip())
    process_per_node = int(parsed["process-per-node"].strip())

    print(f"System: {system}")
    print(f"Experiment: {experiment}")
    print(f"Number of nodes: {nnodes}")
    print(f"Processes per node: {process_per_node}")

    return system, experiment, nnodes, process_per_node

def process_file(file_path, current_exp):
    print(f"\tfile: {file_path}")
   
    data=[]
    true_lables=""
    first_line = True
    with open(file_path, 'r') as file:
        for line in file:
            #print(f"\t\tline: {line.strip()}")
            if first_line:
                system, experiment, nnodes, process_per_node = parse_infoline(line)
                first_line = False
            if "#" not in line:
                size, extra, current_datafile = [item.strip() for item in line.strip().split(',')]
                print(f"\t\tcurrent_datafile: {current_datafile}, extra: {extra}, size: {size}")
                    
                lables, data_array = read_data(current_datafile)

                #print("\t\t", lables)
                #print("\t\tlen of transposed_array: ", len(transposed_array) )
                #print("\t\tsize ", size, " ---> ", parse_bytes_binary(size))
                
                lables.insert(0, 'hrsize')
                lables.insert(0, 'size')
                lables.insert(0, 'process_per_node')
                lables.insert(0, 'nnodes')
                lables.insert(0, 'experiment')
                lables.insert(0, 'extra')
                lables.insert(0, 'system')

                for j in range(0, len(data_array)):
                    data_array[j].insert(0, size)
                    data_array[j].insert(0, parse_bytes_binary(size))
                    data_array[j].insert(0, process_per_node)
                    data_array[j].insert(0, nnodes)
                    data_array[j].insert(0, experiment)
                    data_array[j].insert(0, extra)
                    data_array[j].insert(0, system)

                #print(lables)
                #print(data_array)
                
                if true_lables == "":
                    true_lables=lables
                    print("\t", true_lables)
                else:
                    if lables != true_lables:
                        print("ERROR!!!")
                
                for e in data_array:
                    data.append(e)
    return true_lables, data


def mediancol(df, collectorname, valuename, extraname=None):
    sizes=df[collectorname].unique()
    print("sizes: ", sizes)
    if extraname == None:
        distinguisher = sizes
    else:
        extras=df[extraname].unique()
        distinguisher = list(itertools.product(sizes, extras))
        print("extras: ", extras)
    print("distinguisher: ", distinguisher)

    medians={}
    for s in distinguisher:
        if extraname == None:
            sub_df=df[df[collectorname]==s]
            search_for = s
        else:
            sub_df=df[(df[collectorname]==s[0]) & (df[extraname]==s[1])]
            search_for = str(s[0]) + str(s[1])
        print("sub_df for ", search_for, " has len: ", len(sub_df))
        if len(sub_df) > 0:
            medians[search_for] = statistics.median(sub_df[valuename])
        else:
            medians[search_for] = 0
    print("medians: ", medians)

    newvalname= valuename + '-median'
    
    if extraname == None:
        df['MeshedNames'] = df[collectorname].astype(str)
    else:
        df['MeshedNames'] = df[collectorname].astype(str) + df[extraname].astype(str)
    print("---- MeshedNames ----")
    print(df['MeshedNames'])
    df['Median'] = df['MeshedNames'].map(medians)
    print("---- Median ----")
    print(df['Median'])
    
    print("---- DF ----")
    print(df)
    #exit()

    df[newvalname] = df[valuename] / df['Median']
    df.drop(columns=['MeshedNames'], inplace=True)
    df.drop(columns=['Median'], inplace=True)
    print(df)

    return newvalname

def boxPlot(df, outname, title, collectorname, valuename, xlable='Message Size', ylable='Time', extraname=None):
    newvaluename = mediancol(df, collectorname, valuename, extraname)

    sns.set(style="whitegrid")
    plt.figure(figsize=(12, 8))
    sns.boxplot(data=df, x=collectorname, y=newvaluename, hue="extra", showfliers=False)
    plt.title(title)
    plt.xlabel(xlable)
    plt.ylabel(ylable)
    plt.xticks(rotation=30)
    plotname=outname
    plt.savefig(plotname)
    plt.close()

def linePlot(df, outname, title, collectorname, valuename, xlable='Message Size', ylable='Time', peak=None, 
        mnp_coords=None, mnp_valuename=None, mnp_collectorname=None, mnp_xlable=None, mnp_ylable=None):
  
    # Set global font size
    my_fontsize = 20

    sns.set(style="whitegrid")
    plt.figure(figsize=(12, 8))
    sns.lineplot(data=df,
        x=collectorname, y=valuename, hue="extra", style="extra",
        markers=True, dashes=False, errorbar=('pi', 50),
        linewidth=3, markersize=8
    )

    if peak != None:
        plt.axhline(y=peak, color='r', linestyle='--')
        peak_lable = f"Theoretical peak: {peak:.2f}"
        x_min, x_max = plt.gca().get_xlim()
        x_center = (x_min + x_max) / 3
        plt.text(x=x_center, y=peak, s=peak_lable, color='r', va='center', ha='center', backgroundcolor='w', fontsize=my_fontsize)

    max_acheved = df[valuename].max() 
    #plt.axhline(y=max_acheved, color='g', linestyle='--')
    max_lable = f"Max acheved value: {max_acheved:.2f}"
    x_min, x_max = plt.gca().get_xlim()
    x_center = (x_min + x_max) / 2
    if peak != None:
        y_centre = peak / 2
    else:
        y_centre = max_acheved / 2
    plt.text(x=x_max+0.5, y=y_centre, s=max_lable, color='g', va='center', ha='left', backgroundcolor='w', rotation = 90, fontsize=my_fontsize)

    plt.title(title, fontsize=my_fontsize)
    plt.xlabel(xlable, fontsize=my_fontsize)
    plt.ylabel(ylable, fontsize=my_fontsize)
    plt.xticks(rotation=30, fontsize=my_fontsize)
    plt.yticks(fontsize=my_fontsize)
    plt.legend(loc='lower right', bbox_to_anchor=(1, 0), ncol=1, fontsize=my_fontsize)
    plt.tight_layout(pad=1.5)

    if mnp_coords != None:
        mnp_ax = plt.axes(mnp_coords, facecolor='w')
        sns.lineplot(data=df[~df["hrsize"].str.contains('iB')],
            x=mnp_collectorname, y=mnp_valuename, hue="extra", style="extra",
            markers=True, dashes=False, ax=mnp_ax, legend=False, errorbar=('pi', 50)
        )

        plt.xticks(rotation=30)
        plt.xlabel(mnp_xlable)
        plt.ylabel(mnp_ylable)

    plotname=outname
    plt.savefig(plotname)
    plt.close()

def addBandwidth(df, exptype, durationlable, newlable, ty='complessive'):
    if ty == 'complessive':
        #df['nproc'] = df['nnodes'] * df['process_per_node']

        if 'inc' in exptype:
            df['totaldatatrasfer_B'] = (df['nnodes'] - 1 ) * df['size']
        elif 'a2a' in exptype:
            df['totaldatatrasfer_B'] = df['nnodes'] * (df['nnodes'] - 1) * df['size']
        elif 'ar' in exptype:
            df['totaldatatrasfer_B'] = df['nnodes'] * (df['nnodes'] - 1) * df['size']
        else:
            print("Error: unsupported exptype %s" % exptype)
            exit(1)

        df[newlable] = ( ( df['totaldatatrasfer_B'] / 1e+9 ) / df[durationlable] ) * 8 # ( ( B --> GB ) --> GB/s ) --> Gb/s
        #print(df[['experiment', 'nnodes', 'process_per_node', 'size', 'nproc', 'totaldatatrasfer_B', durationlable, newlable]])
        #exit(1)

        #df.drop(columns=['totaldatatrasfer_B'], inplace=True)
        #df.drop(columns=['nproc'], inplace=True)
    elif ty == 'single':
        df[newlable] = ( ( ( df['size'] * df['process_per_node'] ) / 1e+9 ) / df[durationlable] ) * 8
    elif ty == 'alltoall':
        df[newlable] = ( ( ( df['size'] * ( df['nnodes'] - 1 ) * df['process_per_node'] ) / 1e+9 ) / df[durationlable] ) * 8
    else:
        print("Error: unsupported type %s" % ty)
        exit(1)


def main():
    base_dir = './plots'
    for root, dirs, files in os.walk(base_dir):
        for folder in dirs:
            folder_path = os.path.join(root, folder)
            current_exp = os.path.basename(folder_path)
            print(f"folder: {folder_path}")
            print(f"current_exp: {current_exp}")
            for file in os.listdir(folder_path):
                file_path = os.path.join(folder_path, file)
                
                if current_exp != "nanjin" and current_exp != 'haicgu': # ---> BUG to fix
                    lables, data = process_file(file_path, current_exp)
                    
                    print(lables)
                    #for j in range(0, len(lables)):
                    #    print("\t\t", lables[j], ": ", type(data[0][j]))
                    #    if isinstance(data[0][j], list):
                    #        print("\t\t\t", type(data[0][j][0]))

                    df = pd.DataFrame(data, columns=lables)
                    tmp = df['nnodes'].unique()
                    if len(tmp) == 1:
                        nnodes = tmp[0]
                    else:
                        print("Error: the experiment include multiple node sizes (%s)" % str(tmp))
                        exit(1)
                    tmp = df['system'].unique()
                    if len(tmp) == 1:
                        system = tmp[0]
                    else:
                        print("Error: the experiment include multiple node sizes (%s)" % str(tmp))
                        exit(1)
                    tmp = df['process_per_node'].unique()
                    if len(tmp) == 1:
                        ppn = tmp[0]
                    else:
                        print("Error: the experiment include multiple process_per_node values (%s)" % str(tmp))
                        exit(1)

                    multiswitch_falg = 0
                    if system == 'nanjin':
                        multiswitch_falg = 1
                        totalspinepeak = 400

                    df = df.drop(df[df['hrsize'] == '8GiB'].index) #debug
                    if system == 'nanjin' and nnodes == 8 and ppn == 1:
                        df = df.drop(df[df['extra'] != 'nanjin2'].index)
                    print('nnodes: ', nnodes)
                    print('system: ', system)
                    print('ppn:    ', ppn)
                    print('msf:    ', multiswitch_falg)
                    print(df)
                    #exit()

                    if system == 'nanjin':
                        cnpeak = 200
                    elif system == 'haicgu':
                        cnpeak = 100
                    else:
                        cnpeak = None

                    # Plotting
                    if current_exp == 'pingpong':
                        print("-----------------------------")

                        basename=file_path.removesuffix('.csv')
                        outname=basename + '_boxplot' + '.png'
                        boxPlot(df, outname, file_path, 'hrsize', '0_MainRank-Duration_s', extraname='extra')

                        outname=basename + '_lineplot_combined' + '.png'
                        linePlot(df, outname, file_path, 'hrsize', '0_MainRank-Bandwidth_Gb/s', ylable='Bandwidth (Gb/s)', peak=cnpeak,
                                mnp_coords=[0.17, 0.65, 0.35, 0.2], mnp_valuename='0_MainRank-Duration_s', mnp_collectorname='hrsize', mnp_ylable='Runtime (s)')

                    if current_exp == 'inc_b' or current_exp == 'a2a_b' or current_exp == 'ardc_b':

                        if current_exp == 'inc_b':
                            ty = 'single' 
                            if cnpeak != None:
                                pk = cnpeak / (nnodes - 1)
                            else:
                                pk = None
                        else:
                            #ty = 'complessive'
                            ty = 'alltoall'
                            if cnpeak != None:
                                if not multiswitch_falg:
                                    pk = cnpeak
                                else:
                                    # Here we suppose that processes are equally distributed on two switch
                                    singlespinepeak = ( 2 * totalspinepeak ) / nnodes
                                    pk = ( ( nnodes/2 - 1 ) * cnpeak + (nnodes/2) * singlespinepeak ) / ( nnodes - 1 )
                                    if pk > cnpeak:
                                        pk = cnpeak
                            else:
                                pk = None

                        addBandwidth(df, current_exp, '0_Max-Duration_s', '0_Min-Bandwidth_Gb/s', ty)
                        #addBandwidth(df, current_exp, '0_Median-Duration_s', '0_Bandwidth_Gb/s')
                        #print(df)
                        #exit(1)

                        basename=file_path.removesuffix('.csv')
                        outname=basename + '_boxplot' + '.png'
                        boxPlot(df, outname, file_path, 'hrsize', '0_Avg-Duration_s', ylable='Avg Duration (s)', extraname='extra')

                        #print(df[['hrsize', '0_Max-Duration_s', 'totaldatatrasfer_B', '0_Min-Bandwidth_Gb/s']])
                        #exit(1)

                        outname=basename + '_lineplot_combined' + '.png'
                        linePlot(df, outname, file_path, 'hrsize', '0_Min-Bandwidth_Gb/s', ylable='Bandwidth (Gb/s)', peak=pk,
                                mnp_coords=[0.17, 0.65, 0.4, 0.2], mnp_valuename='0_Max-Duration_s', mnp_collectorname='hrsize', mnp_ylable='Runtime (s)')


if __name__ == "__main__":
    main()

