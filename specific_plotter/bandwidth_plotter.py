import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import matplotlib.ticker as ticker
import os
import argparse

def byte_decode_format(string):
    for idx,char in enumerate(string):
        if not char.isdigit():
            break
    numeral=int(string[:idx])
    byte_string=string[idx:]
    if byte_string=='B':
        multiplier=1
    elif byte_string=='KiB':
        multiplier=2**10
    elif byte_string=='MiB':
        multiplier=2**20
    return numeral*multiplier
    
def byte_encode_format(num):
    if num >= 2**20:
        string=str(num//(2**20))+'MiB'
    elif num >= 2**10:
        string=str(num//(2**10))+'KiB'
    else:
        string=str(num)+'B'
    return string

def read_data_SSSP():
    base_directory='./data/bandwidth_SSSP'
    list_2d=[]
    for idx, file_name in enumerate(os.listdir(base_directory)):
        print(file_name)
        file_path=base_directory+'/'+file_name
        ppn,ms,perm,app_short=file_name.split('_')
        data=np.nan
        if os.path.getsize(file_path)==0:
            check=False
            if app_short=='a2a':
                app='all-to-all'
            elif app_short=='ebb':
                app='bisection'
            
        else:
            if app_short=='a2a':
                app='all-to-all'
                with open(file_path,'r') as file:
                    lines=file.readlines()
                    last_line=lines[-1].strip()
                    if len(lines)<2:
                        check=False
                    elif set(last_line)!=set('-') and last_line.split(' ')[-1]=='Finished':
                        data_line=lines[-2].strip()
                        data=float([x for x in data_line.split(' ') if x != ''][-2])
                        check=True
                    else:
                        check=False
                    
            elif app_short=='ebb':
                app='bisection'
                with open(file_path,'r') as file:
                    lines=file.readlines()
                    last_line=lines[-1].strip()
                    if len(lines)<2:
                        check=False
                    elif set(last_line)!=set('-'):
                        data_line=last_line
                        data=float(data_line.split(' ')[-2][1:])
                        check=True
                    else:
                        check=False

        list_2d+=[[app,ppn,ms,perm,check,data]]    
    
    cols=['app','ppn','msg_size','perm','check','data']
    df=pd.DataFrame(list_2d,columns=cols)
    df[['ppn','msg_size','perm','data']]=df[['ppn','msg_size','perm','data']].apply(pd.to_numeric)
    df.sort_values(by=cols,ignore_index=True,inplace=True)
    return df
    
def read_data_MH():
    base_directory='./data/bandwidth_MH'
    list_2d=[]
    for idx, file_name in enumerate(os.listdir(base_directory)):
        print(file_name)
        d_file=base_directory+'/'+file_name+'/d_'+file_name+'.csv'
        r_file=base_directory+'/'+file_name+'/r_'+file_name


        app_string,_,_=file_name.split('__')
        app_1,app_2,msg_size=app_string.split('_')
        ms=byte_decode_format(msg_size)
        data=np.nan
        
        with open(r_file,'r') as file:
            node_file=file.readlines()[2].split(',')[8].split('/')[-1][:-1]
        if node_file=='SF_all_nodes_ordered':
            perm=0
        else:
            perm=int(node_file.split('_')[-1])
        
        if os.path.isfile(d_file):
            check=True
        else:
            check=False
        
        if app_1=='a2a':
            app='all-to-all'
            if check:
                read_data=pd.read_csv(d_file)
                data=ms/(read_data.iloc[:,0].mean()*(2**10))
                
        elif app_2=='EBB':
            app='bisection'
            if check:
                read_data=pd.read_csv(d_file)
                data=read_data.iloc[:300,1].mean()
                
        list_2d+=[[app,1,ms,perm,check,data]]
    cols=['app','ppn','msg_size','perm','check','data']
    df=pd.DataFrame(list_2d,columns=cols)
    df[['ppn','msg_size','perm','data']]=df[['ppn','msg_size','perm','data']].apply(pd.to_numeric)
    df.sort_values(by=cols,ignore_index=True,inplace=True)
    #with pd.option_context('display.max_rows',None,'display.max_columns',None):
    #    print(df)
    return df
    
def main():
    parser=argparse.ArgumentParser(description='Plots for bandwidth tests.')
    parser.add_argument('plotname',help='Name of lineplot.')
    args=parser.parse_args()
    
    SSSP_df=read_data_SSSP()
    MH_df=read_data_MH()
    output_path='./plots/'+args.plotname+'.pdf'
    
    SSSP_df=SSSP_df.loc[(SSSP_df['app']=='bisection') &
                        (SSSP_df['ppn']==1) &
                        (SSSP_df['check']==True)]
                        
    MH_df=MH_df.loc[(MH_df['app']=='bisection') &
                    (MH_df['ppn']==1) &
                    (MH_df['check']==True)]

    SSSP_df=SSSP_df[['msg_size','perm','data']]
    MH_df=MH_df[['msg_size','perm','data']]
    
    SSSP_df=SSSP_df.groupby(['msg_size'],as_index=False).mean().drop('perm',axis=1)
    MH_df=MH_df.groupby(['msg_size'],as_index=False).mean().drop('perm',axis=1)
    
    SSSP_df['mode']='SSSP'
    MH_df['mode']='Min-Hop'
    df=pd.concat([SSSP_df,MH_df])
    with pd.option_context('display.max_rows',None,'display.max_columns',None):
        print()
        print(df.pivot(index='msg_size',columns='mode',values='data'))
    
    sns.set_theme(style='darkgrid')
    ax=sns.lineplot(data=df,x='msg_size',y='data',hue='mode',marker='o')
    x_ticks=df['msg_size'].unique().tolist()
    x_ticklabels=[byte_encode_format(x) for x in x_ticks]
    ax.set_xscale('log')
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_ticklabels)
    ax.set_xlabel('message size')
    ax.set_ylabel('bandwidth [MiB/s]')
    ax.set_title('Bandwidth analysis')
    ax.get_legend().set_title(None)
    ax.figure.savefig(output_path,bbox_inches='tight')

if __name__=='__main__':
    main()
