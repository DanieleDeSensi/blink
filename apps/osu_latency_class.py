class app:

    #data parameters:
    path_to_executable='./osu-micro-benchmarks-5.7.1/mpi/pt2pt/osu_latency'#(string) path to executable
    num_metrics=1 #(int) specifies how many datapoints are collected
    data_labels=['Avg-Duration']#(list (of length num_metrics) of strings)
    data_units=['us'] #(list (of length num_metrics) of strings)
    conv_mask=[True]

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
        out_string=self.stdout
        data=out_string.split(' ')[-1]
        data=float(data.strip())
        return [[data]]   
