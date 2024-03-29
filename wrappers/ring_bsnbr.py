import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from microbench_common import microbench

class app(microbench):
    def get_binary_path(self):
        return self.get_path("ring_bsnbr")
    
    def get_bench_name(self):
        return "Ring (blocking send, non-blocking recv)"