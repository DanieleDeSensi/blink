import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base,sizeof_fmt
import ast

class app(base):
    metadata = [
        {'name': 'BW_peak' , 'unit': 'gbit/s', 'conv': False},
        {'name': 'BW_average'  , 'unit': 'gbit/s', 'conv': True },
        {'name': 'MsgRate', 'unit': 'mpp/s' , 'conv': False},
    ]

    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/wrappers/ib_send_bw.sh"

    def read_data(self):  # return list (size num_metrics) of variable size lists
        files = []
        for file in os.listdir():
            if "ib_send_bw_" in file and ".json" in file:
                files += [file]
        if len(files) == 0:
            # cannot find the json file created by ib_send_bw
            print('No json files found.')
            return [[] for _ in range(len(self.metadata))]
        bw_peak = 0
        bw_average = 0
        msgrate = 0
        for path in files:
            with open(path) as file:
                lines = file.readlines()
                for line in lines:
                    line_clean = line.strip()
                    if line_clean.startswith("BW_peak"):
                        bw_peak += float(line_clean.split(",")[0].split(":")[1].strip())
                    elif line_clean.startswith("BW_average"):
                        bw_average += float(line_clean.split(",")[0].split(":")[1].strip())
                    elif line_clean.startswith("MsgRate"):
                        msgrate += float(line_clean.split(",")[0].split(":")[1].strip().replace("}",""))
        return [[bw_peak], [bw_average], [msgrate]]

    def get_bench_name(self):
        return "ib_send_bw"
    
    def get_bench_input(self):
        args_fields = self.args.split(" ")
        pos = args_fields.index("-s") + 1
        return sizeof_fmt(2**int(args_fields[pos]))
