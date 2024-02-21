import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base

class app(base):
    metadata = [
        {'name': 'Avg-Duration', 'unit': 'us'   , 'conv': True },
        {'name': 'EBB'         , 'unit': 'MiBps', 'conv': False}
    ]

    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/netgauge-2.4.6/netgauge"
    
    # how to extract the data
    # Returns a list (one element per metric) of lists (one element per measurement) of values
    def read_data(self):  
        out_string = self.stdout
        out_lines = out_string.split('\n')
        data = [[], []]
        for line in out_lines[2:-4]:
            data_string = line.split(':')[-1]
            data_list = data_string.split(' ')
            data1 = float(data_list[1])
            data2 = float(data_list[3][1:])
            data[0] += [data1]
            data[1] += [data2]
        return data

    def get_bench_name(self):
        return "Netgauge - EBB"
    
    def get_bench_input(self):
        return ""