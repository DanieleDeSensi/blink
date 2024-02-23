import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base,sizeof_fmt

class gpubench(base):
    metadata = [
        {'name': 'Transfer size (B)'     , 'unit': 'B'   , 'conv': False },
        {'name': 'Transfer Time (s)'     , 'unit': 's'   , 'conv': False},
        {'name': 'Bandwidth (GB/s)'      , 'unit': 'GB/s', 'conv': False}
    ]

    def read_data(self):
        output = self.stdout
        lines = output.split('\n')
        lines = [x for x in lines if 'Iteration' in x and x.strip() != '']
        data = [[float(x.split(',')[0].split(':')[1]), float(x.split(',')[1].split(':')[1]), float(x.split(',')[2].split(':')[1])] for x in lines]
        return data

    def get_bench_input(self):
        return ""
