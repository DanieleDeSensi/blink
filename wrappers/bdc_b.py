import sys
import os
from common import microbench

class app(microbench):
    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/microbench/bin/bdc_b"
    
    def get_bench_name(self):
        return "Broadcast"