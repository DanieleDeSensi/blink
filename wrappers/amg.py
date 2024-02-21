import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")

class app(base):  
    exists = True

    metadata = [
        {'name': 'spatial_operator', 'unit': 's', 'conv': False},
        {'name': 'IJ_vector_setup' , 'unit': 's', 'conv': False},
        {'name': 'AMG_setup'       , 'unit': 's', 'conv': False},
        {'name': 'AMG_solve'       , 'unit': 's', 'conv': False},
        {'name': 'total'           , 'unit': 's', 'conv': True}
    ]

    def get_binary_path(self):
        env_name = "BLINK_AMG_PATH"
        if env_name not in os.environ or os.environ[env_name] == "":
            self.exists = False
            return None
        else:
            return os.environ[env_name]

    def read_data(self):  # return list (size num_metrics) of variable size lists
        if self.exists:
            output = self.stdout
            lines = output.split('\n')
            lines = lines[11], lines[22], lines[31], lines[44]
            data = [float(x.split(' ')[-2]) for x in lines]
            data += [sum(data)]
            data = [[x] for x in data]
            return data
        else:
            return [[0]*self.num_metrics]

    def get_bench_name(self):
        return "AMG"
    
    def get_bench_input(self):
        return ""