import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/apps/internals/")
from common import microbench

class app(microbench):
    def run_app(self):  # return string on how to call app
        return os.environ["BLINK_ROOT"] + "./apps/internals/bdc_b/bdc_b " + self.args