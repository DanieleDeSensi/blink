import os

class app:
    # data parameters:
    num_metrics = 5  # (int) specifies how many datapoints are collected
    data_labels = ['spatial_operator', 'IJ_vector_setup',
                   'AMG_setup', 'AMG_solve', 'total']  # (list (of length num_metrics) of strings)
    data_units = ['s']*5  # (list (of length num_metrics) of strings)
    conv_mask = [True]*5

    # execution functions:
    def __init__(self, id_num, collect_flag, args):
        self.id_num = id_num
        self.args = args
        self.collect_flag = collect_flag
        if len(self.data_labels) != self.num_metrics:
            raise Exception('Class with id '+str(id_num) +
                            ': shape mismatch of data_labels array')
        if len(self.data_units) != self.num_metrics:
            raise Exception('Class with id '+str(id_num) +
                            ': shape mismatch of data_units array')
        if len(self.conv_mask) != self.num_metrics:
            raise Exception('Class with id '+str(id_num) +
                            ': shape mismatch of convergence_mask')

    def set_process(self, process):
        self.process = process

    def set_output(self, stdout, stderr):
        self.stdout = stdout.decode('utf-8')
        self.stderr = stderr.decode('utf-8')

    def set_nodes(self, node_list):
        self.node_list = node_list
        self.num_nodes = len(node_list)

    # customizable functions:
    def run_app(self):  # return string on how to call app
        if "BLINK_AMG_PATH" in os.environ and os.environ["BLINK_AMG_PATH"] != '':
            return os.environ["BLINK_AMG_PATH"]+' '+self.args
        else:
            return ""

    def read_data(self):  # return list (size num_metrics) of variable size lists
        if "BLINK_AMG_PATH" in os.environ and os.environ["BLINK_AMG_PATH"] != '':
            output = self.stdout
            lines = output.split('\n')
            lines = lines[11], lines[22], lines[31], lines[44]
            data = [float(x.split(' ')[-2]) for x in lines]
            data += [sum(data)]
            data = [[x] for x in data]
            return data
        else:
            return [[0]*self.num_metrics]
