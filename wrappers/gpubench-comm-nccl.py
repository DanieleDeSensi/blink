import os
import sys
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import sizeof_fmt
from gpubench_common import gpubench

class app(gpubench):
    metadata = [
        {'name': 'groupStartEnd', 'unit': 's'   , 'conv': True},
        {'name': 'initRank'     , 'unit': 's'   , 'conv': False},
    ]

    def get_binary_path(self):
        return self.get_path("comm_Nccl")

    def get_bench_name(self):
        return "gpubench comm Nccl"
    
    def read_data(self):
        output = self.stdout
        lines = output.split('\n')
        lines = [x for x in lines if x.startswith("#")]
        tmp_data = [[float(x.split(' ')[1]), float(x.split(' ')[2])] for x in lines]
        data = [list(x) for x in zip(*tmp_data)]
        return data

    def get_bench_input(self):
        return ""
