from common import microbench
import sys
import os

class app(microbench):
    # assign these fields
    # (string) path to executable
    path_to_executable = os.environ["BLINK_ROOT"] + '/src/microbench/bin/null_dummy'
    num_metrics = 0  # (int) specifies how many datapoints are collected
    data_labels = []  # (list (of length num_metrics) of strings)
    data_units = []  # (list (of length num_metrics) of strings)
    # list (of length num_metrics) of booleans) true if the data in the corresponding list should be measured until it converged else if should be ignored in convergence checks set to false
    conv_mask = []

    def run_app(self):  # return string on how to call app, insert path to executable for standard calling
        return self.path_to_executable + ' ' + self.args

    def read_data(self):  # return list (size num_metrics) of variable size lists
        data_list = []
        return data_list
