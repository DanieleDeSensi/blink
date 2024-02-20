import sys
import os
from common import microbench

class app(microbench):
    def run_app(self):  # return string on how to call app
        return os.environ["BLINK_ROOT"] + "/src/microbench/bin/ping-pong_b " + self.args