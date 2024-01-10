class app:

    #data parameters:
    path_to_executable='./apps/ember-master/mpi/pingpong/pingpong'
    num_metrics=6 #(int) specifies how many datapoints are collected
    data_labels=['MsgSize','Time','Msgs','Bytes','Msg-Throughput','Throughput']#(list (of length num_metrics) of strings)
    data_units=['B','s','KMsgs','MB','KMsg/s','MB/s'] #(list (of length num_metrics) of strings)
    conv_mask=[False,True,True,True,True,True]

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
        data_list=[None]*self.num_metrics
        data_line=self.stdout.splitlines()[-1].split()
        for i in range(self.num_metrics):
            data_list[i]=[float(data_line[i])]    
        return data_list
        
