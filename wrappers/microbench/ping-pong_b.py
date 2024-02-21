import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers/microbench/")
from common import microbench

class app(microbench):
    metadata = [
        {'name': 'MainRank-Duration', 'unit': 's', 'conv': True}
    ]

    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/microbench/bin/ping-pong_b" 
    
    def get_bench_name(self):
        return "Ping-Pong"