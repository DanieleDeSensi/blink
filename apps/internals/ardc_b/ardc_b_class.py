class app:

    #data parameters:
    path_to_executable='./apps/internals/ardc_b/ardc_b'#(string) path to executable
    num_metrics=4 #(int) specifies how many datapoints are collected
    data_labels=['Avg-Duration','Min-Duration','Max-Duration','Median-Duration']#(list (of length num_metrics) of strings)
    data_units=['s','s','s','s'] #(list (of length num_metrics) of strings)
    conv_mask=[True,True,True,True]

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
        tmp_list=[]
        for line in out_string.splitlines()[2:-1]:
            tmp_list+=[[float(x) for x in line.split(',')]]
        data_list=[list(x) for x in zip(*tmp_list)]
        return data_list
