import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from common import microbench

class app(microbench):
    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/microbench/bin/ring_bsnbr" 
    
    def get_bench_name(self):
        return "Ring (blocking send, non-blocking recv)"