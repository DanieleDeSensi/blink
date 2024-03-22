import os
import sys
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import sizeof_fmt
from gpubench_common import gpubench

class app(gpubench):
    def get_binary_path(self):
        return self.get_path("hlo_CudaAware")

    def get_bench_name(self):
        return "gpubench hlo CudaAware"
