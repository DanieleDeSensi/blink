class microbench:
    # data parameters:
    num_metrics = 5  # (int) specifies how many datapoints are collected
    # avg, min, max, median time across all the ranks (per iteration). We also report the time (per iteration) of the main rank.
    data_labels = ['Avg-Duration', 'Min-Duration', 'Max-Duration', 'Median-Duration', 'MainRank-Duration']  # (list (of length num_metrics) of strings)
    # (list (of length num_metrics) of strings)
    data_units = ['s', 's', 's', 's', 's']
    # Convergence mask (list (of length num_metrics) of bools)
    conv_mask = [True, False, False, False, False] # I want to stop once the Avg-Duration converged

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

    def read_data(self):  # return list (size num_metrics) of variable size lists
        out_string = self.stdout
        tmp_list = []
        for line in out_string.splitlines()[2:-1]:
            tmp_list += [[float(x) for x in line.split(',')]]
        data_list = [list(x) for x in zip(*tmp_list)]
        return data_list
