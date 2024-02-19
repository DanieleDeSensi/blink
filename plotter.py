import seaborn as sns
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LogNorm
import matplotlib.ticker as ticker
import os
import argparse
        
def get_abs_split(allocation_split,num_apps,num_nodes):
    if allocation_split=='e': #equal split among all apps
        split_list=[100/num_apps]*num_apps
    else:
        split_list=[float(x) for x in allocation_split.split(':')]

    if sum(split_list)>100:
        raise Exception("Splits percentages mustn't add up to more than 100.")
    if len(split_list) != num_apps:
        raise Exception('Number of applications ('+str(num_apps)+') is not equal to number of splits (' 
                            +str(len(split_list))+')')
    split_absolute=[]
    for split in split_list[:-1]:
        split_absolute+=[int(num_nodes*split/100)]
    split_absolute+=[num_nodes-sum(split_absolute)] #allocate all remaining nodes to the last application
    return split_absolute        
        
def throughput_per_rank(duration,msg_size,num_ranks,mode):
    msg_size=byte_decode_format(msg_size)
    if np.isnan(duration) or mode=='barrier':
        return np.NaN
    elif mode=='all-to-all':
        return msg_size*num_ranks/duration
    elif mode=='incast' or mode=='broadcast' or mode=='allreduce':
        return msg_size*(num_ranks-1)/(duration*num_ranks)
    elif mode=='GPCnet-ring' or mode=='pw-pingpong':
        return msg_size*2/duration
    else:
        raise Exception('Unknown throughput mode: '+mode)
        
def throughput_per_rank_list(durations,msg_size,num_ranks,mode):
    return [throughput_per_rank(x,msg_size,num_ranks,mode) for x in durations]

def ci_l(l_isolated,l_congested):
    if np.isnan(l_congested) or np.isnan(l_isolated):
        return np.NaN
    else:
        return l_congested/l_isolated
        
def ci_l_list(con_list,iso_list):
    return [ci_l(iso,con)for iso,con in zip(iso_list,con_list)]
    
def ci_b(b_isolated,b_congested):
    if np.isnan(b_congested) or np.isnan(b_isolated):
        return np.NaN
    else:
        return b_isolated/b_congested

def ci_b_list(iso_list,con_list):
    return [ci_b(iso,con)for iso,con in zip(iso_list,con_list)]
        
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


def read_data(base_directory,num_nodes):
    #define a global frame:
    meta_cols=['idx','full_name','data_path','converged','num_samples',
          'vic_name','vic_msg_size','vic_num_ranks',
          'agg_name','agg_msg_size','agg_num_ranks',
          'all_mode','all_split']
          
    meta_df=pd.DataFrame(columns=meta_cols)
    
    meta_df.idx=meta_df.idx.astype(int)
    meta_df.full_name=meta_df.full_name.astype(str)
    meta_df.data_path=meta_df.data_path.astype(str)
    meta_df.converged=meta_df.converged.astype(bool)   
    meta_df.num_samples=meta_df.num_samples.astype(int)
    meta_df.vic_msg_size=meta_df.vic_msg_size.astype('category')
    meta_df.agg_msg_size=meta_df.agg_msg_size.astype('category')
    meta_df.vic_num_ranks=meta_df.vic_num_ranks.astype(int)
    meta_df.agg_num_ranks=meta_df.agg_num_ranks.astype(int)    
    meta_df.vic_name=meta_df.vic_name.astype('category')
    meta_df.agg_name=meta_df.agg_name.astype('category')
    meta_df.all_mode=meta_df.all_mode.astype('category')    
    meta_df.all_split=meta_df.all_split.astype('category')
    
    dura_cols=['idx','avg_duration','min_duration','max_duration','median_duration','rs_duration']
    
    dura_df=pd.DataFrame(columns=dura_cols)
    
    dura_df.idx=dura_df.idx.astype(int)
    dura_df.avg_duration=dura_df.avg_duration.astype(float)
    dura_df.min_duration=dura_df.min_duration.astype(float)
    dura_df.max_duration=dura_df.max_duration.astype(float)
    dura_df.median_duration=dura_df.median_duration.astype(float)
    dura_df.rs_duration=dura_df.rs_duration.astype(float)
      
    for idx, directory in enumerate(os.listdir(base_directory)):       
        #check if data from directory converged
        converged=True
        print(directory)
        with open(base_directory+'/'+directory+'/md_'+directory+'.csv') as meta_file:
            lines=meta_file.readlines()
            conv_goal=list(map((lambda x: x.strip()=='True'),lines[4].split(',')[1:]))
            conv_actu=list(map((lambda x: x.strip()=='True'),lines[5].split(',')[1:]))
            for goal,actu in zip(conv_goal,conv_actu):
                if goal and not actu:
                    converged=False
                    break
                    
            num_samples=0
            for line in lines[7:]:
                num_samples+=int(line.split(',')[1])
                    
        full_name=directory
        data_path=base_directory+'/'+directory+'/d_'+directory+'.csv'

        name_list=directory.split('__')
        data_title=name_list[0]
        data_title_list=data_title.split('_')
        
        vic_app=data_title_list[0]
        
        if vic_app=='barrier':
            vic_name=vic_app
            vic_msg_size='0B'
            
            agg_app=data_title_list[2]
            agg_com_mode=data_title_list[3]
            agg_name=agg_app+'_'+agg_com_mode
        
            if  agg_app=='null' and agg_com_mode=='dummy':
                agg_msg_size='0B'
                
            else:
                agg_msg_size=data_title_list[4]
        else:
            vic_com_mode=data_title_list[1]
            vic_msg_size=data_title_list[2]
            vic_name=vic_app+'_'+vic_com_mode
        
            agg_app=data_title_list[3]
            agg_com_mode=data_title_list[4]
            agg_name=agg_app+'_'+agg_com_mode
        
            if  agg_app=='null' and agg_com_mode=='dummy':
                agg_msg_size='0B'
                
            else:
                agg_msg_size=data_title_list[5]
            
        all_mode=(name_list[1]).split('_')[0]
        
        if all_mode=='l':
            all_mode='linear'
        elif all_mode=='r':
            all_mode='random'
        elif all_mode=='i':
            all_mode='interleaved'
            
        all_split=(name_list[1]).split('_')[1]
        
        vic_num_ranks,agg_num_ranks=get_abs_split(all_split,2,num_nodes)
        
        if vic_name=='a2a_b':
            vic_name='all-to-all'
        elif vic_name=='ardc_b':
            vic_name='allreduce'
        elif vic_name=='bdc_b':
            vic_name='broadcast'
        elif vic_name=='pw-ping-pong_b':
            vic_name='pw-pingpong'
        elif vic_name=='ring_bsnbr':
            vic_name='GPCnet-ring'
            
        if agg_name=='a2a_b':
            agg_name='all-to-all'
        elif agg_name=='inc_b':
            agg_name='incast'
        elif agg_name=='null_dummy':
            agg_name='isolated'
        
        samples_list=[idx,full_name,data_path,converged,num_samples,
                vic_name,vic_msg_size,vic_num_ranks,
                agg_name,agg_msg_size,agg_num_ranks,
                all_mode,all_split]
                
        local_df=pd.DataFrame(columns=meta_cols)
        local_df.loc[idx]=samples_list
        meta_df = pd.concat([meta_df, local_df], ignore_index=True)
        
        if vic_app!='inc' and vic_app!='bdc':
            read_df=pd.read_csv(data_path,names=dura_cols[1:-1],header=None,skiprows=1)
            read_df[dura_cols[-1]]=np.NaN
            read_df[dura_cols[0]]=idx
        else:
            read_df=pd.read_csv(data_path,names=dura_cols[1:],header=None,skiprows=1)
            read_df[dura_cols[0]]=idx

        dura_df = pd.concat([dura_df, read_df], ignore_index=True)             
    
    #add order to msg_size cols
    vic_msg_sizes_list=meta_df['vic_msg_size'].unique()
    vic_ms_cats=sorted(vic_msg_sizes_list,key=byte_decode_format)
    meta_df.vic_msg_size=pd.Categorical(meta_df.vic_msg_size,categories=vic_ms_cats,
                                        ordered=True)
    agg_msg_sizes_list=meta_df['agg_msg_size'].unique()
    agg_msg_cats=sorted(agg_msg_sizes_list,key=byte_decode_format)
    meta_df.agg_msg_size=pd.Categorical(meta_df.agg_msg_size,categories=agg_msg_cats,
                                        ordered=True)                                     
    meta_df.agg_name=pd.Categorical(meta_df.agg_name,categories=['isolated','incast','all-to-all'],
                                        ordered=True)
    vic_names_list=meta_df['vic_name'].unique()
    vic_names_cats=sorted(vic_names_list,key=str.casefold)
    meta_df.vic_name=pd.Categorical(meta_df.vic_name,categories=vic_names_cats,
                                        ordered=True)
    #add order to allocation splits
    #all_split_list=meta_df['all_split'].unique()
    #all_split_order=sorted(all_split_list,key=lambda x:int(x.split(':')[0]))
    #meta_df.all_split=pd.Categorical(meta_df.all_split,categories=all_split_order)
           
    return meta_df,dura_df
    
def store_backup(df,backup_directory,name):
    df.to_pickle(backup_directory+'/'+name+'.pkl')
   
def fetch_backup(backup_directory,name):
    df=pd.read_pickle(backup_directory+'/'+name)
    return df
    
def clear_backup(backup_directory):
    for file in os.listdir(backup_directory):
        os.remove(backup_directory+'/'+file)
    os.rmdir(backup_directory)
    
def throughput_data(meta_df,dura_df):
    df=pd.merge(meta_df,dura_df,on='idx')
    tput_list=[[row[0]]+throughput_per_rank_list([row[1],row[2],row[3],row[4],row[5]],row[6],row[7],row[8])
                for row in zip(df['idx'],df['avg_duration'],df['min_duration'],df['max_duration'],
                               df['median_duration'],df['rs_duration'],df['vic_msg_size'],df['vic_num_ranks'],
                               df['vic_name'])]
                               
    tput_cols=['idx','avg_throughput','max_throughput','min_throughput','median_throughput','rs_throughput']
    tput_df=pd.DataFrame(tput_list,columns=tput_cols)
       
    tput_df.idx=tput_df.idx.astype(int)
    tput_df.avg_throughput=tput_df.avg_throughput.astype(float)
    tput_df.min_throughput=tput_df.min_throughput.astype(float)
    tput_df.max_throughput=tput_df.max_throughput.astype(float)
    tput_df.median_throughput=tput_df.median_throughput.astype(float)
    tput_df.rs_throughput=tput_df.rs_throughput.astype(float)
    return tput_df
 
def lci_data(meta_df,dura_df,agg_mode):
    if agg_mode=='avg':
        dura_df=dura_df.groupby(['idx'],as_index=False).mean()
    elif agg_mode=='min':
        dura_df=dura_df.groupby(['idx'],as_index=False).min()
    elif agg_mode=='max':
        dura_df=dura_df.groupby(['idx'],as_index=False).max()
    elif agg_mode=='median':
        dura_df=dura_df.groupby(['idx'],as_index=False).quantile(0.5)
    elif agg_mode=='q99':
        dura_df=dura_df.groupby(['idx'],as_index=False).quantile(0.99)
        
    df=pd.merge(meta_df,dura_df,on='idx')
    iso_df=df.loc[df['agg_name']=='isolated']
    lci_list=[[row[0]]+ci_l_list([row[1],row[2],row[3],row[4],row[5]],
                (iso_df.loc[(iso_df['vic_name']==row[6])&(iso_df['all_split']==row[7])&
                                (iso_df['all_mode']==row[8])&(iso_df['vic_msg_size']==row[9])
                                ][['avg_duration','min_duration','max_duration','median_duration','rs_duration']]
                                ).iloc[0,:].tolist())
                for row in zip(df['idx'],df['avg_duration'],df['min_duration'],df['max_duration'],
                               df['median_duration'],df['rs_duration'],df['vic_name'],df['all_split'],
                               df['all_mode'],df['vic_msg_size'])]
    lci_cols=['idx','avg_cong_imp','min_cong_imp','max_cong_imp','median_cong_imp','rs_cong_imp']
    lci_df=pd.DataFrame(lci_list,columns=lci_cols)
    
    lci_df.idx=lci_df.idx.astype(int)
    lci_df.avg_cong_imp=lci_df.avg_cong_imp.astype(float)
    lci_df.min_cong_imp=lci_df.min_cong_imp.astype(float)
    lci_df.max_cong_imp=lci_df.max_cong_imp.astype(float)
    lci_df.median_cong_imp=lci_df.median_cong_imp.astype(float)
    lci_df.rs_cong_imp=lci_df.rs_cong_imp.astype(float)
    return lci_df

    
def plot_trend(m_df,d_df,data_col):
    m1_df=m_df.loc[(m_df['agg_name']=='a2a_b') & 
                  (m_df['all_split']=='50:50') &
                  (m_df['all_mode']=='i')]
                  
    m2_df=m_df.loc[(m_df['agg_name']=='isolated') & 
                  (m_df['all_split']=='50:50') &
                  (m_df['all_mode']=='i')]
                  
    plot1_df=pd.merge(m1_df,d_df,on='idx')
    plot1_df=plot1_df[[data_col]]
    plot1_df['time']=plot1_df.index
    print('Congested: ','{:.5f}'.format(plot1_df[data_col].mean()))
    
    plot2_df=pd.merge(m2_df,d_df,on='idx')
    plot2_df=plot2_df[[data_col]]
    plot2_df['time']=plot2_df.index
    print('Isolated: ','{:.5f}'.format(plot2_df[data_col].mean()))
    
    return sns.lineplot(x=plot1_df['time'],y=plot1_df[data_col]),sns.lineplot(x=plot2_df['time'],y=plot2_df[data_col])
    
def main():

    ###############
    #sns.jointplot#
    ###############

    parser=argparse.ArgumentParser(description='Plots for pairing testing.')
    parser.add_argument('plot',help='Type of plot.',
                        choices=['violin','heatmap','trend','overview','box'])
    parser.add_argument('basedir',help='Path to base directory of data.')
    parser.add_argument('-x','--x_list',help='Data to be used on x axis of plot (comma seperated list).')
    parser.add_argument('-y','--y_list',help='Data to be used on y axis of plot (comma seperated list).')
    parser.add_argument('-r','--refetch',help='Clear backup and refetch data.',action='store_true',default=False)
    parser.add_argument('-mt','--metric',help='Metric of interest.',choices=['tput','dur','lci','bci'],default='dur')
    parser.add_argument('-ag','--aggregation',help='Way of aggregating data.',
                        choices=['avg','min','max','median','q99'],default='avg')
    parser.add_argument('-c','--column',help='Data column to plot.',
                        choices=['avg','min','max','median','rs'],default='max')
    parser.add_argument('-pn','--plotname',help='Name of plot for storing.')
    parser.add_argument('-n','--num_nodes',help='Number of nodes.',default=200,type=int)
    parser.add_argument('-sp','--split',help='Node split to plot.',default='50:50')
    parser.add_argument('-v','--victim',help='Victim application to plot.',default='allreduce')
    parser.add_argument('-a','--aggressors',help='Aggressor applications (comma seperated list).',default='isolated,all-to-all')
    parser.add_argument('-ms','--msgsize',help='Message size to plot.',default='16KiB')
    parser.add_argument('-am','--allmode',help='Allocation mode to plot.',default='random')
    parser.add_argument('--ylim',help='Upper y limit.',default=None)
    
    args=parser.parse_args()
    
    split=args.split
    all_mode=args.allmode
    msg_size=args.msgsize
    vic_name=args.victim
    agg_names=args.aggressors.split(',')
    
    plot=args.plot
    refetch=args.refetch
    aggregation=args.aggregation
    metric=args.metric
    num_nodes=args.num_nodes
    if args.x_list is not None:
        x_list=args.x_list.split(',')
    if args.y_list is not None:
        y_list=args.y_list.split(',')
    
    if metric=='tput':
        data_col=args.column+'_throughput'
    elif metric=='dur':
        data_col=args.column+'_duration'
    else:
        data_col=args.column+'_cong_imp'

    base_directory=args.basedir
    backup_name=base_directory.split('/')[-1]
    backup_directory='./plots/postprocessed_files/'+backup_name
    
    if args.plotname is None:
        form_split=':'.join([str(int(float(x))) for x in split.split(':')])
        output_path='./plots/'+plot+'_'+metric+'_'+form_split+'.pdf'
    else:
        output_path='./plots/'+args.plotname+'.pdf'
    
    if refetch:
        clear_backup(backup_directory)
    
    if os.path.isdir(backup_directory):
        print('Data has already been read. Fetch backup...')
        meta_df=fetch_backup(backup_directory,'meta_df.pkl')
        dura_df=fetch_backup(backup_directory,'dura_df.pkl')
        print('Done.')
    else:
        print('Read data...')
        meta_df,dura_df=read_data(base_directory,num_nodes)
        print('Read all data in '+base_directory)
        
        print('Store backup...')
        os.mkdir(backup_directory)
        store_backup(meta_df,backup_directory,'meta_df')
        store_backup(dura_df,backup_directory,'dura_df')
        print('Done.')
    
    if metric=='tput':
        if os.path.isfile(backup_directory+'/tput_df.pkl'):
            tput_df=fetch_backup(backup_directory,'tput_df.pkl')
        else:
            print('Build throughput frame...')
            tput_df=throughput_data(meta_df,dura_df)
            print('Done.')
            print('Store backup...')
            store_backup(tput_df,backup_directory,'tput_df')
            print('Done.')
            
    if metric=='lci':
        if os.path.isfile(backup_directory+'/lci_'+aggregation+'_df.pkl'):
            lci_df=fetch_backup(backup_directory,'lci_'+aggregation+'_df.pkl')
        else:
            print('Build latency congestion impact frame...')
            lci_df=lci_data(meta_df,dura_df,aggregation)
            print('Done.')
            print('Store backup...')
            store_backup(lci_df,backup_directory,'lci_'+aggregation+'_df')
            print('Done.') 
    
    if metric=='tput':
        d_df=tput_df
    elif metric=='dur':
        d_df=dura_df
    elif metric=='lci':
        d_df=lci_df
    m_df=meta_df
    
    sns.set_theme(style='darkgrid')
    
    if plot=='overview':
        #print overview:
        def q99(x):
            return x.quantile(0.99)
        
        d_df=d_df[['idx',data_col]]
        d_df=d_df.groupby(['idx']).agg(['median','mean',q99,'max','count'])
        over_df=pd.merge(m_df,d_df,on='idx')
        over_df=over_df.drop(['idx','full_name','data_path','num_samples','agg_msg_size',
                'vic_num_ranks','agg_num_ranks'],axis=1)
        over_df.sort_values(by=['all_mode', 'all_split','vic_name','vic_msg_size','agg_name'],inplace=True)
        remaining_cols=[x for x in over_df.columns if x not in ['all_mode', 'all_split','vic_name','vic_msg_size','agg_name']]
        over_df=over_df[['all_mode', 'all_split','vic_name','vic_msg_size','agg_name']+remaining_cols]
        over_df['max/q99']=over_df[('max_duration', 'max')]/over_df[('max_duration', 'q99')]
        over_df['max/median']=over_df[('max_duration', 'max')]/over_df[('max_duration', 'median')]
        with pd.option_context('display.max_rows',None,'display.max_columns',None):
            print(over_df)
        
        over_df.to_csv('./overview.csv',float_format='%.3e')
        
    elif plot=='violin' or plot=='box':
    
        if len(x_list)>1:
            raise Exception('Violins need only 1 x-argument.')
        if len(y_list)>1:
            raise Exception('Violins need only 1 y-argument.')
        
        #prepare plot dataframe
        plot_df=pd.merge(m_df,d_df,on='idx')              
        #select rows
        plot_df=plot_df.loc[(plot_df['all_split']==split) & (plot_df['all_mode']==all_mode) &
                            (plot_df['vic_name']==vic_name) & (plot_df['vic_msg_size']==msg_size) &
                            (plot_df['agg_name'].isin(agg_names))]
        
        #select cols
        plot_df=plot_df[x_list+y_list]        
        
        plot_df['agg_name']=plot_df['agg_name'].cat.remove_unused_categories()
        
        x=x_list[0]
        y=y_list[0]
        
        title=vic_name+' ['+msg_size+']\n'+all_mode+' allocation, '+split
        
        if metric=='tput':
            plot_df[y]/=(10**6) #to KB per second
            ylabel='throughput [KBps]'
            
        elif metric=='dur':
            plot_df[y]*=(10**6) #to micro seconds 
            ylabel='duration [us]'
        
        q99_df=plot_df.groupby([x],as_index=False).quantile(0.99)
        q99s=q99_df[y].to_list()
                     
        if plot=='violin':
            ax=sns.violinplot(data=plot_df,x=x,y=y,cut=0,scale='width')
        elif plot=='box':
            ax=sns.boxplot(data=plot_df,x=x,y=y,notch=True,showmeans=True)
        
        if args.ylim is not None:
            ax.set(ylim=(0, int(args.ylim)))
        
        if not (('incast' in agg_names) and ('all-to-all' in agg_names)):
            new_labels=[]
            for label in ax.get_xticklabels():
                if label.get_text() in ['incast','all-to-all']:
                    new_labels+=['congested']
                else:
                    new_labels+=[label.get_text()]
            ax.set_xticklabels(new_labels)
        ax.set_xlabel('')
        ax.set_ylabel(ylabel)
        ax.set_title(title)
        
        #display 99%-quantile
        plt.scatter(x=range(len(q99s)),y=q99s,c='w',marker='D',s=5)
        ax.figure.savefig(output_path,bbox_inches='tight')
        
    elif plot=='heatmap':
    
        def map_heatmap(*args,**kwargs):
            data=kwargs.pop('data')
            data=data.drop(args[3],axis=1)
            data=data.pivot(columns=args[0],index=args[1],values=args[2])
            #sorted_cols=sorted(data.columns,reverse=True,key=lambda x: (x[0],-x[1]))
            
            sorted_cols = sorted(data.columns,reverse=True)
            data=data[sorted_cols]
            data=data.sort_index()
            sns.heatmap(data,**kwargs)
    
        #prepare plot dataframe:
        d_df=d_df[['idx',data_col]]

        if metric != 'lci': #lci already aggregated
            if args.aggregation=='avg':
                d_df=d_df.groupby(['idx'],as_index=False).mean()
            elif args.aggregation=='min':
                d_df=d_df.groupby(['idx'],as_index=False).min()
            elif args.aggregation=='max':
                d_df=d_df.groupby(['idx'],as_index=False).max()
            elif args.aggregation=='median':
                d_df=d_df.groupby(['idx'],as_index=False).quantile(0.5)
            elif args.aggregation=='q99':
                d_df=d_df.groupby(['idx'],as_index=False).quantile(0.99)
                    
        m_df=m_df[['idx']+x_list+y_list]
        plot_df=pd.merge(m_df,d_df,on='idx')
        
        if metric=='lci' and 'agg_name' in plot_df.columns:
            plot_df=plot_df.drop(plot_df[plot_df['agg_name']=='isolated'].index)
        plot_df=plot_df.drop('idx',axis=1)

        #plot_df=plot_df.pivot(columns=x_list,index=y_list,values=data_col)
        #sorted_cols=sorted(plot_df.columns)
        #plot_df=plot_df[sorted_cols]
        
        v_min=float(plot_df[data_col].min())
        v_max=float(plot_df[data_col].max())
        
        if metric=='tput':
            cmap='RdYlGn'
            norm=LogNorm(vmin=v_min,vmax=v_max)
            annot_fmt='.2f'
            
            plot_df/=(10**6) #to KB per second
            unit='KBps'
            
        elif metric=='dur':
            cmap='RdYlGn_r'
            norm=LogNorm(vmin=v_min,vmax=v_max)
            annot_fmt='.2f'
            
            print(plot_df)
            plot_df*=(10**6) #to micro seconds 
            unit='us'
               
        elif metric=='lci':
            cmap='RdYlGn_r'
            norm=LogNorm(vmin=v_min,vmax=v_max)
            annot_fmt='.1f'
            
            unit=''
        
        fg_col='all_mode'
        FG=sns.FacetGrid(plot_df,col=fg_col,col_order=['linear','random','interleaved'],height=10
                    ,aspect=0.3,gridspec_kws={'right':0.875,'wspace':0.05})   
        x_list.remove(fg_col) 
        cbar_ax=FG.fig.add_axes([0.9, 0.25, 0.05, 0.5])
        cmap_fmt=ticker.FormatStrFormatter('%.1f')
        FG.map_dataframe(map_heatmap,x_list,y_list,data_col,fg_col,
                            xticklabels=True,yticklabels=True,cbar=True,robust=False,
                            cmap=cmap,linewidth=0.1,cbar_ax=cbar_ax,vmin=v_min,vmax=v_max,
                            cbar_kws={'format':cmap_fmt,'ticks':np.linspace(v_min,v_max,5),'label':'Congestion-impact'},
                            square=True,annot=True,annot_kws={'size':10},fmt=annot_fmt)
        
        hlines_coord=[] 
        new_ylabels=[]
        
        new_xlabels_min=[]
        new_xlabels_maj=[]
        
        new_xticks_min=[]
        new_xticks_maj=[]
        
        line_width=3
        for row_idx,row in enumerate(FG.axes):
            for col_idx,ax in enumerate(row):
                ax.set_ylabel('')
                ax.set_xlabel('')
                ax.set_title(ax.get_title().split('=')[-1],size=17)
                ax.axvline(3,c='w',*ax.get_ylim(),lw=line_width)
                
                if col_idx==0:
                    ylabels=ax.get_yticklabels()
                    #get horizontal line coord
                    for label_idx in range(1,len(ylabels)):
                        curr=ylabels[label_idx]
                        prev=ylabels[label_idx-1]
                        if curr.get_text().split('-')[:-1]!=prev.get_text().split('-')[:-1]:
                            x,y=prev.get_position()
                            hlines_coord+=[y+0.5]
                    
                    for label in ylabels:
                        old=label.get_text()
                        name='-'.join(old.split('-')[:-1])
                        msgs=old.split('-')[-1]
                        if name=='barrier':
                            new=name
                        else:    
                            new=name+' ['+msgs+']'
                        new_ylabels+=[new]
                    if row_idx==0:
                        xlabels=ax.get_xticklabels()
                        for label in xlabels:
                            old=label.get_text()
                            x,y=label.get_position()
                            name='-'.join(old.split('-')[:-1])
                            splt=old.split('-')[-1]
                            new_xlabels_min+=[splt.split(':')[-1]+'%']
                            new_xticks_min+=[x]
                            if splt=='50:50':
                                new_xlabels_maj+=['\n'+name]
                                new_xticks_maj+=[x]
                            
                ax.hlines(hlines_coord, colors='w', *ax.get_xlim(),lw=line_width)
                ax.set_yticklabels(new_ylabels,size=12)
                ax.set_xticks(new_xticks_min,minor=True)
                ax.set_xticks(new_xticks_maj,minor=False)
                ax.set_xticklabels(new_xlabels_min,minor=True)
                ax.set_xticklabels(new_xlabels_maj,minor=False,size=15)
                for label in ax.get_xticklabels(minor=False):
                    label.set_rotation('horizontal')
                ax.xaxis.remove_overlapping_locs=False  
                 
        FG.savefig(output_path,bbox_inches='tight')
        
    elif plot=='trend':
    
        if len(x_list)>1:
            raise Exception('Violins need only 1 x-argument.')
        if len(y_list)>1:
            raise Exception('Violins need only 1 y-argument.')
        
        #prepare plot dataframe
        plot_df=pd.merge(m_df,d_df,on='idx')
        
        #select rows
        plot_df=plot_df.loc[(plot_df['all_split']==split) & (plot_df['all_mode']==all_mode) &
                            (plot_df['vic_name']==vic_name) & (plot_df['vic_msg_size']==msg_size) &
                            (plot_df['agg_name'].isin(agg_names))]
        
        #select cols
        plot_df=plot_df[x_list+y_list]
        
        plot_df['agg_name']=plot_df['agg_name'].cat.remove_unused_categories()
        
        x=x_list[0]
        y=y_list[0]
        
        title=vic_name+' ['+msg_size+']\n'+all_mode+' allocation, '+split
        
        if metric=='tput':
            plot_df[y]/=(10**6) #to KB per second
            ylabel='throughput [KBps]'
            
        elif metric=='dur':
            plot_df[y]*=(10**6) #to micro seconds 
            ylabel='duration [us]'
        
        #add indices:
        prev_row=''
        val=0
        plot_idx=[]
        for i,row in plot_df.iterrows():
            if row[x]!=prev_row:
                val=0
            else:
                val+=1
            plot_idx+=[val]    
            prev_row=row[x]
            
        plot_df['plot_idx']=plot_idx
            
        ax=sns.lineplot(y=plot_df[y],x=plot_df['plot_idx'],hue=plot_df[x],data=plot_df)
        
        x_lb=0
        x_ub=plot_df['plot_idx'].max()+1
        x_step=100
        ax.set_xticks(range(x_lb,x_ub+1,x_step))
        ax.margins(x=0)
        
        ax.set_xlabel('samples')
        ax.set_ylabel(ylabel)
        ax.set_title(title)
        
        ax.legend(loc='center left',bbox_to_anchor=(1, 0.5))
        ax.get_legend().set_title(None)
        
        ax.figure.savefig(output_path,bbox_inches='tight')
    else:
        raise Exception('Unknown plot type.')   
        
    #    with pd.option_context('display.max_rows',None,'display.max_columns',None):
    #        print(tput_df.iloc[:10,:])
    #print(dura_df)
    #sns.barplot(x=meta_df['agg_name'],y=meta_df['min_duration'])
    #plt.show()

if __name__=='__main__':
    main()
