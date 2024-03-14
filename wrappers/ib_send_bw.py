import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base,sizeof_fmt
import json

class app(base):
    metadata = [
        {'name': 'BW_peak' , 'unit': 'gbit/s', 'conv': False},
        {'name': 'BW_average'  , 'unit': 'gbit/s', 'conv': True },
        {'name': 'MsgRate', 'unit': 'mpp/s' , 'conv': False},
    ]

    def get_binary_path(self):
        return "ib_send_bw"

    def read_data(self):  # return list (size num_metrics) of variable size lists
        for file in os.listdir():
            if file.endswith("ib_send_bw.json"):
                path = file
                break
        if path is None:
            # cannot find the json file created by ib_send_bw
            print('No json file found.')
            return [[] for _ in range(len(self.metadata))]
        with open(path) as json_file:
            json_data = json.load(json_file)
            return [[json_data["results"][metric]] for metric in self.metadata]

    def get_bench_name(self):
        return "ib_send_bw"
    
    def get_bench_input(self):
        args_fields = self.args.split(" ")
        pos = args_fields.index("-s") + 1
        return sizeof_fmt(2**int(args_fields[pos]))