import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from microbench_common import microbench

class app(microbench):
    metadata = []
    
    def get_binary_path(self):
        return self.get_path("null_dummy")
    
    def read_data(self):  # return list (size num_metrics) of variable size lists
        data_list = []
        return data_list
    
    def get_bench_name(self):
        return "Isolated"
