import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/apps/internals/")
from common import microbench

class app(microbench):
    def run_app(self):  # return string on how to call app
        return os.environ["BLINK_ROOT"] + "./apps/internals/pw-ping-pong_b/pw-ping-pong_b " + self.args