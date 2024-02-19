import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import matplotlib.ticker as ticker
import os
import argparse
  
def get_data(path):
    d_df=pd.read_pickle(path+'/dura_df.pkl')
    m_df=pd.read_pickle(path+'/meta_df.pkl')
    return pd.merge(m_df,d_df,on='idx')
 
def transform(df,data_col,agg_name,cols,aggregate):    
    df=df[cols+[data_col]]
    if aggregate:
        df=df.groupby(by=cols,as_index=False).mean()
    df=df.loc[df['agg_name']==agg_name]
    df=df.drop('agg_name',axis=1)
    df=df.dropna()
    return df
    
def overview(df_S,df_M):
    data_col='max_duration'
    agg_name='isolated'
    cols=['all_mode', 'all_split','vic_name','vic_msg_size','agg_name']

    df_S=transform(df_S,data_col,agg_name,cols,True)
    df_M=transform(df_M,data_col,agg_name,cols,True)
    
    df=pd.merge(left=df_S,right=df_M
            ,on=cols[:-1]
            ,suffixes=('_left', '_right'))
    df['ratio']=df[data_col+'_left']/df[data_col+'_right']
    df=df.drop([data_col+'_left',data_col+'_right'],axis=1)

    with pd.option_context('display.max_rows',None,'display.max_columns',None):
        print(df)

def violins(df):
    data_col='max_duration'
    agg_name='isolated'
    cols=['all_mode', 'all_split','vic_name','vic_msg_size','agg_name','routing']
    df=transform(df,data_col,agg_name,cols,False)
    
    df=df.loc[df['all_split']=='50:50']
    df=df.drop('all_split',axis=1)
    #df['all_mode_and_split']=df['all_mode'].astype(str)+'_'+df['all_split'].astype(str)
    #df=df.drop(['all_mode','all_split'],axis=1)
    
    df[data_col]*=(10**6)
    
    output_path='./plots/routing_violin.pdf'
    
    def map_violins(*args,**kwargs):
        data=kwargs.pop('data')
        data['vic_msg_size']=data['vic_msg_size'].cat.remove_unused_categories()
        sns.violinplot(data=data,**kwargs)
    
    #sns.set_theme(style='darkgrid')
    FG=sns.FacetGrid(df,row='all_mode',col='vic_name',sharex=False,sharey='col',margin_titles=True
                    ,gridspec_kws={'wspace':0.5,'hspace':0.05,'width_ratios':[3,3,1,3,3,3]})
    FG.map_dataframe(map_violins,x='vic_msg_size',y=data_col,hue='routing',cut=0,scale='width')
    FG.add_legend()
    FG.set_titles(col_template='{col_name}', row_template='{row_name}',size=15)
    for row_idx,row in enumerate(FG.axes):
            for col_idx,ax in enumerate(row):
                ax.set_yscale('log')
                ax.yaxis.set_major_formatter(ticker.ScalarFormatter())
                #if row_idx==0:
                #    print(ax.get_title())
                #    ax.set_title(ax.get_title().split('=')[-1])
                #else:
                #    ax.set_title('')
                if row_idx==1 and col_idx==0:
                    ax.set_ylabel('duration [us]')
                if col_idx==2:
                    ax.set_xticklabels([])    
                if row_idx!=2:
                    ax.set_xticklabels([])
    #FG.set_title('isolated max-duration, 50:50 split')
    FG.savefig(output_path,bbox_inches='tight')

def main():    
    #assume data has already been backed up by plotter.py once
    backup_directory='./plots/postprocessed_files/'
    df_S=get_data(backup_directory+'/pairings_SSSP')
    df_M=get_data(backup_directory+'/pairings_MH')
    
    #overview(df_S,df_M)
    
    df_S['routing']='SSSP'
    df_M['routing']='Min-Hop'
    violins(pd.concat([df_S,df_M]))

if __name__=='__main__':
    main()
