class app:
    # data parameters:
    # (string) path to executable
    path_to_executable = './src/netgauge-2.4.6/netgauge'
    num_metrics = 2  # (int) specifies how many datapoints are collected
    # (list (of length num_metrics) of strings)
    data_labels = ['Avg-Duration', 'EBB']
    data_units = ['us', 'MiBps']  # (list (of length num_metrics) of strings)
    conv_mask = [True, False]

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
        return self.path_to_executable+' '+self.args

    def read_data(self):  # return list (size num_metrics) of variable size lists
        out_string = self.stdout
        out_lines = out_string.split('\n')
        data = [[], []]
        for line in out_lines[2:-4]:
            data_string = line.split(':')[-1]
            data_list = data_string.split(' ')
            data1 = float(data_list[1])
            data2 = float(data_list[3][1:])
            data[0] += [data1]
            data[1] += [data2]
        return data
