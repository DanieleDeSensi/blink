import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base

class app(base):
    metadata = [
        {'name': 'Time'                 , 'unit': 's'        , 'conv': True },
        {'name': 'MaxData_xchanged/rank', 'unit': 'KB/rank'  , 'conv': False},
        {'name': 'Throughput/rank'      , 'unit': 'MB/s/Rank', 'conv': False}
    ]

    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/ember/mpi/halo3d-26/halo3d-26"
        
    def read_data(self): 
        data_list=[None]*self.num_metrics
        data_line=self.stdout.splitlines()[-1].split()
        for i in range(self.num_metrics):
            data_list[i]=[float(data_line[i])]    
        return data_list
        
    def get_bench_name(self):
        return "Ember - Halo3D"
    
    def get_bench_input(self):
        return ""