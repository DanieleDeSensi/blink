import os
import sys
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base,sizeof_fmt

class app(base):  
    metadata = [
        {'name': 'avg-iteration-runtime', 'unit': 's', 'conv': True},
    ]

    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/dnn-proxies/bin/dlrm"
    
    def read_data(self):
        output = self.stdout
        lines = output.split('\n')
        outline = lines[-2]
        return [[float(outline.split(" ")[-2])]]

    def get_bench_name(self):
        return "DLRM Proxy"
    
    def get_bench_input(self):
        return ""