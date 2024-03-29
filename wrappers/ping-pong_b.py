import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from microbench_common import microbench

class app(microbench):
    metadata = [
        {'name': 'MainRank-Duration', 'unit': 's', 'conv': True},
        {'name': 'MainRank-Bandwidth', 'unit': 'Gb/s', 'conv': False}
    ]

    def get_binary_path(self):
        return self.get_path("ping-pong_b")
    
    def get_bench_name(self):
        return "Ping-Pong"