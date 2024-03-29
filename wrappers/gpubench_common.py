import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base,sizeof_fmt

class gpubench(base):
    metadata = [
        {'name': 'Transfer Time', 'unit': 's'   , 'conv': True},
        {'name': 'Bandwidth'    , 'unit': 'GB/s', 'conv': False}
    ]

    def get_path(self, name):
        p = ""
        sys = os.environ["BLINK_SYSTEM"]
        if "CudaAware" in name and (sys == "alps" or sys == "lumi"):
            p += os.environ["BLINK_ROOT"] + "/src/microbench-gpu/select_gpu_" + sys + " "
        elif "CudaAware" in name and (sys == "leonardo"):
            p += os.environ["BLINK_ROOT"] + "/src/microbench-gpu/select_nic_" + sys + " "
        p += os.environ["BLINK_ROOT"] + "/src/microbench-gpu/bin/" + name
        return p
    
    def read_data(self):
        output = self.stdout
        lines = output.split('\n')
        lines = [x for x in lines if 'Iteration' in x and x.strip() != '' and not '[Average]' in x]
        tmp_data = [[float(x.split(',')[1].split(':')[1]), float(x.split(',')[2].split(':')[1])] for x in lines]
        data = [list(x) for x in zip(*tmp_data)]
        return data

    def get_bench_input(self):
        args_fields = self.args.split(" ")
        pos = args_fields.index("-x") + 1
        return sizeof_fmt(2**int(args_fields[pos]))
