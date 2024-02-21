import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from microbench_common import microbench

class app(microbench):
    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/microbench/bin/o2o_bsnbr" 
    
    def get_bench_name(self):
        return "Permutation (blocking send non-blocking recv)"