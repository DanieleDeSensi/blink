# Base class (don't modify this file)
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

    # customizable functions:
    # If None is returned, the application will not be executed
    def get_binary_path(self):
        return None

    def run_app(self):  # return string on how to call app
        path = self.get_binary_path()
        if path is not None:
            return self.path_to_executable + ' ' + self.args
        else:
            return ""
        
    # Returns a list (one element per metric) of lists (one element per measurement) of values
    def read_data(self):
        return []
