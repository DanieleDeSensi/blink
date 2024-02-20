import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers/microbench/")
from common import microbench

class app(microbench):
    def get_binary_path(self):
        return os.environ["BLINK_ROOT"] + "/src/microbench/bin/inc_put" 