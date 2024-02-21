This folder contains the Python scripts used to run different applications and benchmarks and to parse its output so that it can be plotted by the plotting scripts provided by the framework.
Files MUST NOT be put in subfolders (mostly to guarantee uniqueness of wrappers names).
Three main components need to be defined (e.g., look at g500.py).

1. metadata: An array of dictionaries, representing the metrics analyzed for the benchmark. It has one element per metric. Each element is a dictionary
   with three keys:
   - 'name': The name of the metric
   - 'unit': The unit of the metric
   - 'conv': If true, the benchmark runs until this metric converges (or until the max time threshold is reached)
2. get_binary_path: A function returning a string with the path to the binary to be executed. If None is returned, the benchmark is skipped.
3. read_data: A function that parses the output of the application (either from self.stdout or from a file) and returns a list of lists, where each list
   contains the values of the metrics defined in metadata (one element per sample).
   The order of the outer lists must be the same as specified in metadata.
4. get_bench_name: A function returning a string representing the name of the benchmark. This string will be used to label the plots.
5. get_bench_input: A function returning a string representing the input of the benchmark. This string will be used to label the plots.