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

def read_data(base_directory):
    data_cols=['avg_duration','min_duration','max_duration','median_duration']
    meta_cols=['agg','grty','dir']
    df=pd.DataFrame()
    
    settings=[]
    for idx, directory in enumerate(os.listdir(base_directory)):
        path=base_directory+'/'+directory
        d_path=path+'/d_'+directory+'.csv'
        r_path=path+'/r_'+directory
        
        args_list=directory.split('__')[0].split('_')
        agg=args_list[3]+'_'+args_list[4]
        
        setting=args_list[0]+'_'+args_list[1]+'_'+args_list[2]+'__'+directory.split('__')[1]
        settings+=[setting]
        
        if agg=='null_dummy':
            agg='isolated'
            grty=0
        else:
            with open(r_path,'r') as file:
                agg_args=file.readlines()[6]
            agg_args=agg_args.split('"')[1]
            grty=int(agg_args.split(' ')[-1])
        
        if os.path.isfile(d_path):
            read_df=pd.read_csv(d_path,names=data_cols,header=None,skiprows=1)  
            read_df*=(10**6) #to us
            
        read_df[['agg']]=agg
        read_df[['grty']]=grty
        df=pd.concat([df,read_df],sort=False)
    
    df.sort_values(by=['grty','agg'],ignore_index=True,inplace=True)
        
    assert len(set(settings)) <= 1
    return settings[0],df
    
def main():
    parser=argparse.ArgumentParser(description='Plots for aggressor tests.')
    parser.add_argument('basedir',help='Path to base directory of data.')
    parser.add_argument('plotname',help='Name of the plot.')
    parser.add_argument('grty',help='Measure granularity of aggressor.',type=int)
    parser.add_argument('-c','--column',help='Data column to plot.',
                        choices=['avg','min','max','median'],default='max')
    args=parser.parse_args()
    
    output_path_viols='plots/'+args.plotname+'_viols.pdf'
    output_path_trend='plots/'+args.plotname+'_trend.pdf'
    
    info,plot_df=read_data(args.basedir)
    data_col=args.column+'_duration'
    plot_df=plot_df[['agg','grty',data_col]]
    
    plot_df=plot_df.loc[plot_df['grty'].isin([0,args.grty])]

    def q99(x):
            return x.quantile(0.99)
            
    over_df=plot_df.drop('grty',axis=1).groupby(['agg'],as_index=False).agg(['median','mean',q99,'max','count'])
    with pd.option_context('display.max_rows',None,'display.max_columns',None):
        print(over_df)
    
    victim,allocation=info.split('__')
    if victim.split('_')[:-1]==['pw-ping-pong','b']:
        title_vic='pw-pingpong'
    if victim.split('_')[:-1]==['ring','bsnbr']:
        title_vic='GPCnet-ring'
    title_ms=victim.split('_')[-1]
    all_mode,all_split=allocation.split('_')
    if all_mode=='r':
        title_am='random'
    elif all_mode=='i':
        title_am='interleaved'
    title=title_vic+' ['+title_ms+']\n'+title_am+' '+all_split
    
    sns.set_theme(style='darkgrid')
    ax=sns.violinplot(data=plot_df,x='agg',y=data_col,cut=0,scale='width')
    ax.set_yscale('log')
    ax.yaxis.set_major_formatter(ticker.ScalarFormatter())
    ax.yaxis.get_major_formatter().set_scientific(False)
    ax.set_xticklabels(ax.get_xticklabels(),rotation=90)
    
    ax.set_ylabel('Duration [us]')
    ax.set_xlabel('Aggressor')
    
    ax.set_title(title)
    ax.figure.savefig(output_path_viols,bbox_inches='tight')
    
    #trend
    plt.clf()
    prev_row=''
    val=0
    plot_idx=[]
    for i,row in plot_df.iterrows():
        if row['agg']!=prev_row:
            val=0
        else:
            val+=1
        plot_idx+=[val]    
        prev_row=row['agg']
    
            
    plot_df['plot_idx']=plot_idx
    plot_df=plot_df.drop('grty',axis=1)
    
    FG=sns.FacetGrid(plot_df,row='agg',hue='agg',aspect=3)
    FG.map_dataframe(sns.lineplot,x='plot_idx',y=data_col)
    '''    
    x_lb=0
    x_ub=plot_df['plot_idx'].max()+1
    x_step=100
    ax.set_xticks(range(x_lb,x_ub+1,x_step))
    ax.margins(x=0)
        
    ax.set_xlabel('Samples')
    ax.set_ylabel('Duration [us]')
    ax.set_title(title)
        
    ax.legend(loc='center left',bbox_to_anchor=(1, 0.5))
    ax.get_legend().set_title(None)
    '''  
    for row_idx,row in enumerate(FG.axes):
            for col_idx,ax in enumerate(row):
                ax.set_xlabel('')
                ax.set_ylabel('Duration [us]')
                 
    FG.savefig(output_path_trend,bbox_inches='tight')
    
if __name__=='__main__':
    main()
