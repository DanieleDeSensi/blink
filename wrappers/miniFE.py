import sys
import os
sys.path.append(os.environ["BLINK_ROOT"] + "/wrappers")
from base import base

class app(base):
    exists = True

    metadata = [
        {'name': 'matrix_structure', 'unit': 's', 'conv': False},
        {'name': 'FE_assambly'     , 'unit': 's', 'conv': False},
        {'name': 'WAXPY'           , 'unit': 's', 'conv': False},
        {'name': 'DOT'             , 'unit': 's', 'conv': False},
        {'name': 'MATVEC'          , 'unit': 's', 'conv': False},
        {'name': 'CG_total'        , 'unit': 's', 'conv': False},
        {'name': 'CG_per_iteration', 'unit': 's', 'conv': False},
        {'name': 'total'           , 'unit': 's', 'conv': True}
    ]

    def get_binary_path(self):
        env_name = "BLINK_MINIFE_PATH"
        if env_name not in os.environ or os.environ[env_name] == "":
            self.exists = False
            return None
        else:
            return os.environ[env_name]

    def read_data(self):  # return list (size num_metrics) of variable size lists
        if self.exists:
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

    def get_bench_name(self):
        return "MiniFE"
    
    def get_bench_input(self):
        return ""