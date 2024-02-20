import os

class app:
    # data parameters:
    num_metrics = 8  # (int) specifies how many datapoints are collected
    data_labels = ['matrix_structure', 'FE_assambly', 'WAXPY',
                   'DOT', 'MATVEC', 'CG_total', 'CG_per_iteration',
                   'total']  # (list (of length num_metrics) of strings)
    data_units = ['s']*8  # (list (of length num_metrics) of strings)
    conv_mask = [True]*8

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
        if "BLINK_MINIFE_PATH" in os.environ and os.environ["BLINK_MINIFE_PATH"] != '':
            return os.environ["BLINK_MINIFE_PATH"]+' '+self.args
        else:
            return ""

    def read_data(self):  # return list (size num_metrics) of variable size lists
        if "BLINK_MINIFE_PATH" in os.environ and os.environ["BLINK_MINIFE_PATH"] != '':
            path = None
            for file in os.listdir():
                if file[:6] == 'miniFE':
                    path = file
                    break
            if path is None:
                # cannot find a file yaml file created by miniFE
                print('No yaml file found.')
                return [[] for _ in range(8)]
            with open(path, 'r') as file:
                lines = file.readlines()
            idxs = [28, 30, 45, 48, 51, 55, 58, 61]
            data = [[float(lines[idx].split(' ')[-1])] for idx in idxs]
            os.remove(path)
            return data
        else:
            return [[0]*self.num_metrics]
