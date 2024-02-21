# Base class (don't modify this file)

def sizeof_fmt(num, suffix='B'):
    for unit in ['','Ki','Mi','Gi','Ti','Pi','Ei','Zi']:
        if abs(num) < 1024.0:
            return "%.0f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%f%s%s" % (num, 'Yi', suffix)

class base:
    def __init__(self, id_num, collect_flag, args):
        self.id_num = id_num
        self.args = args
        self.collect_flag = collect_flag

    def set_process(self, process):
        self.process = process

    def set_output(self, stdout, stderr):
        self.stdout = stdout.decode('utf-8')
        self.stderr = stderr.decode('utf-8')

    def set_nodes(self, node_list):
        self.node_list = node_list
        self.num_nodes = len(node_list)

    # Function that MUST be overriden
    # If None is returned, the application will not be executed
    def get_binary_path(self):
        return None
    
    # Returns a list (one element per metric) of lists (one element per measurement) of values
    def read_data(self):
        return []
    
    # Returns a human-readable string representing the benchmark name
    def get_bench_name(self):
        return ""
    
    # Returns a human-readable string representing the benchmark input description
    # args are in self.args
    def get_bench_input(self):
        return ""

    # Functions that CAN be overriden
    def run_app(self):  # return string on how to call app
        path = self.get_binary_path()
        if path is not None:
            return path + ' ' + self.args
        else:
            return ""        

