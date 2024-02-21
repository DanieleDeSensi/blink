import os
from base import base,sizeof_fmt

class app(base):  
    exists = True

    metadata = [
        {'name': 'time-oop' , 'unit': 'us'  , 'conv': True }, # Runtime (out-of-place)
        {'name': 'algbw-oop', 'unit': 'GB/s', 'conv': False}, # Algorithmic Bandwidth (out-of-place)
        {'name': 'busbw-oop', 'unit': 'GB/s', 'conv': False}, # Bus Bandwidth (out-of-place)
        {'name': 'time-ip'  , 'unit': 'us'  , 'conv': True }, # Runtime (in-place)
        {'name': 'algbw-ip' , 'unit': 'GB/s', 'conv': False}, # Algorithmic Bandwidth (in-place)
        {'name': 'busbw-ip' , 'unit': 'GB/s', 'conv': False}  # Bus Bandwidth (in-place)
    ]

    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/nccl-tests/build/sendrecv_perf"

    def read_data(self):  # return list (size num_metrics) of variable size lists
        if self.exists:
            output = self.stdout
            lines = output.split('\n')
            for l in lines:
                if l[0] != "#":
                    l = ' '.join(l.strip().split()) # Replace multiple whitespaces with one
                    fields = l.split(' ')
                    return fields[5:11]
    
        return [[0]*self.num_metrics]

    def get_bench_name(self):
        return "NCCL SendRecv"
    
    def get_bench_input(self):
        if "-b" not in self.args or "-e" not in self.args:
            raise ValueError("No message size specified")
        else:
            args_values = self.args.split(" ") 
            lower = sizeof_fmt(int(args_values[args_values.index('-b') + 1]))
            upper = sizeof_fmt(int(args_values[args_values.index('-e') + 1]))
            if(lower != upper):
                raise ValueError("Benchmark was called with different lower and upper bounds (-b and -e)")
            return lower