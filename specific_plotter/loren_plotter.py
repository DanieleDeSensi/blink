import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd


def DrawLinePlot(data, coll, yax):

    print("Plotting data collective: "+coll)

    f, ax1 = plt.subplots(figsize=(25, 10))
    df = pd.DataFrame(data)
    #sns.set_context("paper", font_scale = 3)
    #sns.set_palette("pastel")

    #fig = sns.barplot(df, x='name', y='time', hue='name', edgecolor=".3", linewidth=.5, errorbar="sd", ax=ax1)
    fig = sns.lineplot(data=df, x='message_size', y=yax)
    #ax1.axhline(y=lat, color='red', linestyle='--', linewidth=2, label=f'Average Total Latency: {1.243}')

    plt.tick_params(axis='both', which='major', labelsize=18)
    measure = '(GiB)'
    if yax == 'bandwidth':
        measure = '(Gib/s)'
    elif yax == 'latency':
        measure = '(s)'
    plt.ylabel(f'{yax} {measure}', fontsize=28)
    plt.xlabel('Message Size', fontsize=28)
    plt.title(f'{coll}', fontsize=28)
    plt.tight_layout(h_pad=2)

    plt.savefig(f'plot_{coll}_{yax}.png')

def LoadData(data, path, coll):

    print("Loading data for collective: "+coll)

    df_description = pd.read_csv(description_path)

    for i in range(len(df_description['app_mix'])):

        if df_description['app_mix'][i].strip().split('/')[-2] != coll:
            continue

        message_size = df_description['app_mix'][i].strip().split('/')[-1]
        path = df_description['path'][i]
        global_path = "./blink"+path[1:len(path)]+"/data.csv"

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

        message_bytes = int(message_digit)

        if message_mult == "B":
            message_bytes /= 1024**3
        elif message_mult == 'KiB':
            message_bytes /= 1024**2
        elif message_mult == 'MiB':
            message_bytes /= 1024


        if coll == "ardc_b":
            message_bytes = 2*(message_bytes/4)*(3)
        elif coll == "a2a_b":
            message_bytes = message_bytes*(3)
        elif coll == "agtr_b" or coll == "agtr_raw":
            message_bytes = (message_bytes/4)*(3)

        bandwidth = [(message_bytes/x)*8 for x in df_data["0_Avg-Duration_s"]]

        data['message_size'].extend([message_size]*len(df_data["0_Avg-Duration_s"]))
        data['latency'].extend(df_data["0_Avg-Duration_s"])
        data['message_GiB'].extend([message_bytes]*len(df_data["0_Avg-Duration_s"]))
        data['bandwidth'].extend(bandwidth)

    with open(f'debug_{coll}.txt', 'a') as debug:
        for i in range(len(data['latency'])):
            debug.write(f"{data['message_size'][i]} - {data['latency'][i]} : {data['bandwidth'][i]}\n")

    return data



if __name__ == "__main__":

    sns.set_theme(style="darkgrid")

    data = {
        'message_size': [],
        'message_GiB': [],
        'latency': [],
        'bandwidth': []
    }


    description_path = "./blink/data/description.csv"

    
    data = LoadData(data, description_path, 'ardc_b')
    DrawLinePlot(data, 'ardc_b', 'latency')
    DrawLinePlot(data, 'ardc_b', 'bandwidth')

    data['message_size'] = []
    data['message_GiB'] = []
    data['latency'] = []
    data['bandwidth'] = []

    data = LoadData(data, description_path, 'a2a_b')
    DrawLinePlot(data, 'a2a_b', 'latency')
    DrawLinePlot(data, 'a2a_b', 'bandwidth')

    data['message_size'] = []
    data['message_GiB'] = []
    data['latency'] = []
    data['bandwidth'] = []

    data = LoadData(data, description_path, 'agtr_b')
    DrawLinePlot(data, 'agtr_b', 'latency')
    DrawLinePlot(data, 'agtr_b', 'bandwidth')
    '''
    data = LoadData(data, description_path, 'agtr_raw')
    DrawLinePlot(data, 'agtr_raw', 'latency')
    DrawLinePlot(data, 'agtr_raw', 'bandwidth')
    '''