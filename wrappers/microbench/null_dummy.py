from common import microbench
import os

class app(microbench):
    metadata = []
    
    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + '/src/microbench/bin/null_dummy'
    
    def read_data(self):  # return list (size num_metrics) of variable size lists
        data_list = []
        return data_list
