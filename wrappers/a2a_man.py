import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from microbench_common import microbench

class app(microbench):
    def get_binary_path(self):
        return self.get_path("a2a_man")
    
    def get_bench_name(self):
        return "Alltoall - Manual"