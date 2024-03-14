import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base,sizeof_fmt
import ast

class app(base):
    metadata = [
        {'name': 'time' , 'unit': 'us', 'conv': True},
    ]

    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/wrappers/ib_send_lat.sh"

    def read_data(self):  # return list (size num_metrics) of variable size lists
        ib_devices = os.environ["BLINK_IB_DEVICES"].count("#") + 1
        files = []
        for i in range(ib_devices):
            file = "ib_send_lat" + str(i)
            files += [file]
        if len(files) == 0:
            # cannot find the json file created by ib_send_lat
            print('No output files found.')
            return [[] for _ in range(len(self.metadata))]
        samples = []
        for path in files:
            start = False
            i = 0
            warmup = 10
            with open(path) as file:
                lines = file.readlines()
                samples_rank = []
                for line in lines:
                    line_clean = line.strip()
                    if line_clean == "#, usec":
                        start = True
                        continue
                    if start and not line_clean.startswith("---"):
                        if i >= warmup: # Discard first warmup samples
                            time = line_clean.split(",")[1].strip()
                            samples_rank += [float(time)]
                        i += 1
                    else:
                        start = False
            samples += [samples_rank]
        
        samples_max = []
        for i in range(len(samples[0])):
            max_time = 0
            for j in range(len(samples)):
                max_time = max(max_time, samples[j][i])
            samples_max += [max_time]
        #for f in files:
        #    os.remove(f)
        return [samples_max]

    def get_bench_name(self):
        return "ib_send_lat"
    
    def get_bench_input(self):
        args_fields = self.args.split(" ")
        pos = args_fields.index("-s") + 1
        return sizeof_fmt(int(args_fields[pos]))
