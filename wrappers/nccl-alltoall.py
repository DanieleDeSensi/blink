import os
import sys
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import sizeof_fmt
from nccl_common import ncclbase

class app(ncclbase):  
    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/nccl-tests/build/alltoall_perf"
    
    def get_bench_name(self):
        return "NCCL Alltoall"