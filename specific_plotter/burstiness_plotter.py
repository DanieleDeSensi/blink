import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import matplotlib.ticker as ticker
import os
import argparse

base_dir='./data/burstiness'
vic=['ring','bsnbr','128B']
agg=['a2a','b']
alloc='i_50:50'
df_l=[]
data_col='0_Max-Duration_s'

for sub_dir in os.listdir(base_dir):
    print(sub_dir)
    d_path=base_dir+'/'+sub_dir+'/d_'+sub_dir+'.csv'
    exp_name,exp_all,exp_date=sub_dir.split('__')
    exp_name_l=exp_name.split('_')
    
    assert vic==exp_name_l[0:3]
    agg_name=exp_name_l[3]+'_'+exp_name_l[4]
    if agg_name=='null_dummy':
        agg_ms='0B'
        blength=0
        bpause=0
    else:
        assert agg==exp_name_l[3:5]
        agg_ms=exp_name_l[5]
        blength=float(exp_name_l[6])
        bpause=float(exp_name_l[7])
        
    read_df=pd.read_csv(d_path)
    read_df['agg_ms']=agg_ms
    read_df['blength']=blength
    read_df['bpause']=bpause
    df_l+=[read_df]
    
r_df=pd.concat(df_l)
r_df=r_df[['agg_ms','blength','bpause',data_col]]
r_df.columns=['agg_ms','blength','bpause','data']
#to us
r_df['data']*=(10**6)
r_df['blength']*=(10**6)
r_df['blength']=r_df['blength'].astype(int)
r_df['bpause']*=(10**6)
r_df['bpause']=r_df['bpause'].astype(int)

#compute congestion impact
df=r_df.groupby(['agg_ms','blength','bpause'],as_index=False).mean()
base_line=df.loc[(df['agg_ms']=='0B')&(df['blength']==0)&(df['bpause']==0)]
df=df.loc[(df['agg_ms']!='0B')&(df['blength']!=0)&(df['bpause']!=0)]
base_line=base_line['data'].to_numpy()[0]
df['data']/=base_line

#congestion impact heatmap
ci_FG=sns.FacetGrid(df,col='agg_ms',col_order=['128B','16KiB','1MiB'],height=10
                    ,aspect=0.3,gridspec_kws={'right':0.875,'wspace':0.05})
v_min=float(df['data'].min())
v_max=float(df['data'].max())
cbar_ax=ci_FG.fig.add_axes([0.9, 0.39, 0.03, 0.2])
cmap='RdYlGn_r'
cmap_fmt=ticker.FormatStrFormatter('%.1f')
annot_fmt='.1f'

def map_heatmap(*args,**kwargs):
    data=kwargs.pop('data')
    data=data.drop(args[3],axis=1)
    print(data)
    data=data.pivot(columns=args[0],index=args[1],values=args[2])
    #sorted_cols=sorted(data.columns,reverse=True,key=lambda x: (x[0],-x[1]))      
    sorted_cols = sorted(data.columns)
    data=data[sorted_cols]
    data=data.sort_index()
    sns.heatmap(data,**kwargs)

ci_FG.map_dataframe(map_heatmap,'blength','bpause','data','agg_ms',
                    xticklabels=True,yticklabels=True,cbar=True,robust=False,
                    cmap=cmap,linewidth=0.1,cbar_ax=cbar_ax,vmin=v_min,vmax=v_max,
                    cbar_kws={'format':cmap_fmt,'ticks':np.linspace(v_min,v_max,5),'label':'Congestion-impact'},
                    square=True,annot=True,annot_kws={'size':10},fmt=annot_fmt)                 

pow10=['1','10²','10⁴','10⁶']
for row_idx,row in enumerate(ci_FG.axes):
            for col_idx,ax in enumerate(row):
                ax.set_title(ax.get_title().split(' ')[-1])
                ax.set_xticklabels(pow10)
                if col_idx==1:
                    ax.set_xlabel('burst length')
                else:
                    ax.set_xlabel('')
                if col_idx==0:
                    ax.set_ylabel('burst pause')
                    ax.set_yticklabels(pow10)
                
ci_FG.savefig('plots/burstiness_heatmap_mean.pdf',bbox_inches='tight')

#example
ex_ms='1MiB'
ex_bl=10**4
ex_bp=10**4
sns.set_theme(style='darkgrid')
plt.clf()
ex_df=r_df.loc[(r_df['agg_ms']==ex_ms)&(r_df['blength']==ex_bl)&(r_df['bpause']==ex_bp)]
ex_df=ex_df.drop(['agg_ms','blength','bpause'],axis=1)

x='time'

if x=='time':
    prefix=0
    xs=[]
    for x in ex_df['data']:
        prefix+=x
        xs+=[prefix]
    ex_df['index']=xs
    x_label='time[us]'
elif x=='sample':
    ex_df.reset_index(level=0,inplace=True)
    x_label='samples'
    
ax=sns.lineplot(data=ex_df,x='index',y='data')
ax.tick_params(axis='both',labelsize=17)
ax.set_ylabel('duration[us]',size=20)
ax.set_xlabel(x_label,size=20)
ax.set_title(ex_ms+', burst length=10⁴us, burst pause=10⁴us',size=24)
ax.figure.savefig('plots/burstiness_zoomin_'+x_label+'.pdf',bbox_inches='tight')

#violinplots
#vio_FG=sns.FacetGrid(r_df,row='blength',col='bpause')
#vio_FG.map_dataframe(sns.violinplot,x='agg_ms',y='data',hue='agg_ms',scale='width',cut=0)
#vio_FG.savefig('plots/burstiness_violins.pdf',bbox_inches='tight')

#trendplots
#r_df=r_df.loc[r_df['agg_ms']=='1MiB']
#r_df.reset_index(level=0,inplace=True)
#r_df['bpms']=r_df['bpause'].astype(str)+'_'+r_df['agg_ms']
#r_df=r_df.drop(['bpause','agg_ms'],axis=1)
#trend_FG=sns.FacetGrid(r_df,row='blength',col='bpause')
#trend_FG.map_dataframe(sns.lineplot,x='index',y='data',hue='agg_ms')
#trend_FG.savefig('plots/burstiness_trend.pdf',bbox_inches='tight')
