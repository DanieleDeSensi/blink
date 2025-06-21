import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd



def DrawLinePlot(data, name):
    print(f"Plotting data collective: {name}")

    # Use a dark theme for the plot
    sns.set_style("whitegrid")  # darker background for axes

    # Create the figure and axes
    f, ax1 = plt.subplots(figsize=(20, 10))
    
    # Convert input data to a DataFrame
    df = pd.DataFrame(data)

    df['cluster_collective'] = df['cluster'].astype(str) + '_' + df['collective'].astype(str)

    # Plot with seaborn
    fig = sns.lineplot(
        data=df,
        x='message_size',
        y='bandwidth',
        hue='cluster_collective',
        style='cluster_collective',
        markers=True,
        markersize=10,
        linewidth=3,
        ax=ax1
    )

    ax1.axhline(
        y=100,
        color='red',
        linestyle='--',
        linewidth=2,
        label=f'Theoretical Peak {100} Gb/s'
    )

    # Labeling and formatting
    ax1.tick_params(axis='both', which='major', labelsize=18)
    ax1.set_ylabel('Bandwidth (Gb/s)', fontsize=28, labelpad=20)
    ax1.set_xlabel('Message Size', fontsize=28, labelpad=20)
    ax1.set_title(f'{name}', fontsize=38, pad=30)

    # Show legend and layout
    ax1.legend(fontsize=20)
    plt.tight_layout()

    # Save the figure
    plt.savefig(f'../plots/{name}.png')  # save with dark background


def LoadData(data, path, coll, size):

    print("Loading data for collective: "+coll)

    df_description = pd.read_csv(description_path)

    for i in range(len(df_description['app_mix'])):

        if df_description['app_mix'][i].strip().split('/')[-2] != coll:
            continue

        if int(df_description['numnodes'][i]) != size:
            continue

        message_size = df_description['app_mix'][i].strip().split('/')[-1]
        path = df_description['path'][i]
        global_path = ".."+path[1:len(path)]+"/data.csv"

        try:
            df_data = pd.read_csv(global_path)
        except Exception as e:
            print("Error reading: "+global_path)
            continue

        message_digit = ""
        message_mult = ""
        for char in message_size:
            if char.isdigit():
                message_digit += char
            else:
                message_mult += char

        message_digit = int(message_digit)

        if message_mult == "B":
            message_bytes = message_digit
        elif message_mult == 'KiB':
            message_bytes = message_digit*1024
        elif message_mult == 'MiB':
            message_bytes = message_digit*1024*1024

        message_gb = (message_bytes/1e9) * 8

        if coll == "ardc_b" or coll == "ardc_noop_b":
            message_gb = 2*message_gb*((size-1)/size)
        elif coll == "a2a_b":
            message_gb = message_gb*(size-1)
        else:  #"agtr_b", "agtr_raw", "redscat_b", "red_scat" also noop and raw versions
            message_gb = message_gb*((size-1)/size)

        latencies = [x for x in df_data["0_Max-Duration_s"]]
        bandwidth = [message_gb / x for x in latencies]

        avg_bandwidth = np.mean(bandwidth)
        avg_latency = np.mean(latencies)

        if coll == "agtr_b":
            print("----")
            print(f"AGTR Bandwidth: {avg_bandwidth} Gb/s")
            print(f"AGTR Latency: {avg_latency} s")
            print(f"AGTR Message Size: {message_size} ({message_bytes} B)")
            print(f"AGTR Message Gb: {message_gb} Gb")
            print("----")

        data['message_size'].extend([message_size]*len(latencies))
        data['latency'].extend(latencies)
        data['message_Gb'].extend([message_gb]*len(latencies))
        data['bandwidth'].extend(bandwidth)
        data['cluster'].extend([df_description['system'][i]+"-"+df_description['extra'][i]]*len(latencies))
        data['collective'].extend([coll]*len(latencies))

    return data

def CleanData(data):
    for key in data.keys():
        data[key] = []
    return data

if __name__ == "__main__":

    data = {
        'message_size': [],
        'message_Gb': [],
        'latency': [],
        'bandwidth': [],
        'cluster': [],
        'collective': []
    }

    description_path = "../data/description.csv"

    node_count = 8

    system = "Nanjing"

    data = LoadData(data, description_path, 'a2a_b', node_count)
    DrawLinePlot(data, f'{system} Blink {node_count} Nodes a2a')
    CleanData(data)

    data = LoadData(data, description_path, 'agtr_b', node_count)
    DrawLinePlot(data, f'{system} Blink {node_count} Nodes agtr')
    CleanData(data)

    data = LoadData(data, description_path, 'ardc_b', node_count)
    DrawLinePlot(data, f'{system} Blink {node_count} Nodes ardc')
    CleanData(data)

    data = LoadData(data, description_path, 'redscat_b', node_count)
    DrawLinePlot(data, f'{system} Blink {node_count} Nodes redscat')
    CleanData(data)
