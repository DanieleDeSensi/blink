import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import matplotlib.ticker as ticker
import os
import argparse

'miniFE_180_inc_b_128KiB__i_90:10__2021-07-31_14:49:57'

def read_data(base_dir):
    #as realapps collect different types of data use dict
    apps={}
    for sub_dir in os.listdir(base_dir):
        print(sub_dir)
        d_path=base_dir+'/'+sub_dir+'/d_'+sub_dir+'.csv'
        exp_name,exp_all,exp_date=sub_dir.split('__')
        exp_name_l=exp_name.split('_')
        
        vic_name=exp_name_l[0]
        agg_name=exp_name_l[2]+'_'+exp_name_l[3]
        if agg_name=='null_dummy':
            agg_ms='0B'            
        else:
            agg_ms=exp_name_l[4]
            
        all_mode,all_split=exp_all.split('_')
    
        if all_mode=='l':
            all_mode='linear'
        elif all_mode=='r':
            all_mode='random'
        elif all_mode=='i':
            all_mode='interleaved'
            
        if agg_name=='a2a_b':
            agg_name='all-to-all'
        elif agg_name=='inc_b':
            agg_name='incast'
        elif agg_name=='null_dummy':
            agg_name='isolated'
    
        read_df=pd.read_csv(d_path)
        read_df.columns=['_'.join(x.split('_')[1:-1]) for x in read_df.columns]
        
        read_df['vic']=vic_name
        read_df['agg']=agg_name
        read_df['all_mode']=all_mode
        read_df['all_split']=all_split
        
        if vic_name in apps:
            apps[vic_name].append(read_df)
        else:
            apps[vic_name]=[read_df]
    
    for app in apps:
        apps[app]=pd.concat(apps[app])
    return apps    
        
def print_overview(data):
    overview={}
    for vic,df in data.items():
        print('\n\n'+vic)
        
        def q95(x):
            return x.quantile(0.95)
            
        df=df.groupby(['all_mode','all_split','agg','vic'],as_index=False).agg(['mean',q95])
        
        #with pd.option_context('display.max_rows',None,'display.max_columns',None):
        print(df)
        overview[vic]=df
        
    return overview

def plot_allmetrics_oneallocation(data,all_mode,all_split,output_path,ow_dict):
    #compute congestion impact
    CIs={}
    for vic,ow in ow_dict.items():
        #ow=ow.loc[(ow['all_mode']==all_mode)&(ow['all_split']==all_split)] 
        num_df=ow
        CIs[vic]=num_df.loc[all_mode,all_split,'all-to-all',vic]/num_df.loc[all_mode,all_split,'isolated',vic]

    for vic,df in data.items():
        df=df.loc[(df['all_mode']==all_mode)&(df['all_split']==all_split)]
        df=df.drop(['all_mode','all_split','vic'],axis=1)
        #make selction
        if vic=='amg':
            pass
        elif vic=='g500':
            df=df.drop(['min_time','q1_time','q3_time','stddev_time','max_time','median_time'
                       ,'min_valid','q1_valid','q3_valid','stddev_valid','max_valid','median_valid']
                       ,axis=1)
        elif vic=='miniFE':
            df=df.drop(['CG_per_iteration'],axis=1)   
        df=pd.melt(df, id_vars=['agg'])
        #df.set_index('agg',inplace=True)
        plt.clf()
        FG=sns.FacetGrid(df,col='variable',col_wrap=3,sharex=False,sharey=False,hue='agg'
                        ,hue_order=['isolated','all-to-all'])
        FG.map_dataframe(sns.violinplot,x='agg',y='value',cut=0,scale='width',order=['isolated','all-to-all'])
        #ytick_fmt=ticker.StrMethodFormatter('{x:.3e}')
        plt.subplots_adjust(hspace=0.5,wspace=0.3)
        
        def format_metric(string):
            #fix typo of parser
            if string=='FE_assambly':
                string='FE_assembly'
            elif string=='spatial_operator':
                string='matrix_generation'
            elif string=='AMG_solve':
                string='AMG-PCG_solve'
            string=string.replace('_',' ')
            return string
        
        for ax in FG.axes:
            metric=ax.get_title().split(' = ')[-1]
            CI_mean=CIs[vic][metric,'mean']
            CI_q95=CIs[vic][metric,'q95']
                
            ax.set_title(format_metric(metric)+
                '\n$CI_{mean}$='+str(round(CI_mean,2))+' | $CI_{q95}$='+str(round(CI_q95,2)),
                y=1.1,fontdict={'fontsize':14.5})
            #ax.set_yscale('log')
            ax.yaxis.set_major_locator(ticker.MaxNLocator(nbins=4,min_n_ticks=3))
            ax.yaxis.set_major_formatter(ticker.ScalarFormatter(useMathText=True))
            ax.ticklabel_format(axis='y',scilimits=(-3,3))
        #plt.ticklabel_format(axis='y',style='sci')
        FG.savefig('../plots/b_real_apps_'+vic+'_'+all_mode+'_'+all_split+'_mean.pdf',bbox_inches='tight')

def main():
    parser=argparse.ArgumentParser(description='Plots for real application testing.')
    parser.add_argument('basedir',help='Path to base directory of data.')
    args=parser.parse_args()
    
    data=read_data(args.basedir)
    ow_df=print_overview(data)
    
    all_mode='interleaved'
    all_split='50:50'
    out_path='plots/real_apps_mean_duration.pdf'
    plot_allmetrics_oneallocation(data,all_mode,all_split,out_path,ow_df)   

if __name__=='__main__':
    main()
