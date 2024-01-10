class app:

    #data parameters:
    path_to_executable='/scratch/2/t2hx/graph500/mpi/runnable'#(string) path to executable
    num_metrics=17 #(int) specifies how many datapoints are collected
    data_labels=['graph_generation','construction',
           'redistribution_time','min_time','q1_time',
           'median_time','q3_time','max_time',
           'mean_time','stddev_time','min_valid','q1_valid',
           'median_valid','q3_valid','max_valid',
           'mean_valid','stddev_valid']#(list (of length num_metrics) of strings)
    data_units=['s']*17 #(list (of length num_metrics) of strings)
    conv_mask=[True]*17

    #execution functions:
    def __init__(self,id_num,collect_flag,args):
        self.id_num=id_num
        self.args=args
        self.collect_flag=collect_flag
        if len(self.data_labels)!=self.num_metrics:
            raise Exception('Class with id '+str(id_num)+': shape mismatch of data_labels array')
        if len(self.data_units)!=self.num_metrics:
            raise Exception('Class with id '+str(id_num)+': shape mismatch of data_units array')
        if len(self.conv_mask)!=self.num_metrics:
            raise Exception('Class with id '+str(id_num)+': shape mismatch of convergence_mask')
            
    def set_process(self,process):
        self.process=process
    
    def set_output(self,stdout,stderr):
        self.stdout=stdout.decode('utf-8')
        self.stderr=stderr.decode('utf-8')
        
    def set_nodes(self,node_list):
        self.node_list=node_list
        self.num_nodes=len(node_list)
        
    #customizable functions:
    def run_app(self): #return string on how to call app
        return self.path_to_executable+' '+self.args
        
    def read_data(self): #return list (size num_metrics) of variable size lists
        output=self.stdout
        lines=output.split('\n')
        lines=[x for x in lines if x.strip()!='']
        lines=lines[4:15]+lines[-7:]
        lines=([lines[0]]+lines[2:])
        data=[[float(x.split(' ')[-1])] for x in lines]
        return data
        
