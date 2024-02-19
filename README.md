# Framework
This framework is designed to run multiple applications and/or benchmarks together and collect data from them to estimate congestion on the interconnect network. 

## Requirements
- Python >= 3.6.8
- numpy
- pandas
- seaborn

## Python scripts
Three python scripts are part of the framework:
- `runner.py`: Main part of the framework. Runs multiple applications and/or benchmarks together and and collects data.
- `driver.py`: Orchestrates multiple executions. It writes scripts for the runner and executes them.
- `plotter.py`: Plots the data collected from the runner

For a schematic overview of how the components of the framwork interact see [overview](./framework__1_.pdf). \
Use `-h` or `--help` command line flag for an explanation of the arguments for each of the three scripts. \
The plotter assumes a certain format and naming of the data and the applications/benchmarks used and needs to be adjusted otherwise.

Example: \
`$ python3 runner.py wl_manager/MPI_wlm.py schedule_files/pairings/pw-ping-pong_b_1MiB_a2a_b_128KiB node_files/SF_all_nodes_ordered -am r -as 10:90`\
to execute the runner. \
`$ python3 plotter.py violin data/pairings -x agg_name -y max_duration -sp 10:90 -ms 1MiB -v pw-pingpong -a isolated,all-to-all`\
to create a corresponding violin plot.

## apps directory
Holds applications/benchmarks and the corresponding python classes that are needed to parse their outputs. \
In the `internals` sub-directory one can find the benchmarks coming with the framework. \
Additionally there are parts that support external applications, which were used in the thesis and experiments on the slimfly:
- `netgauge-2.4.6`: network performance measurement toolkit, can be found [here](https://htor.inf.ethz.ch/research/netgauge/).
- `ember-master`: communication pattern library, can be found [here](https://github.com/sstsimulator/ember).
- `osu-micro-benchmarks-5.7.1`: benchmark suite, can be found [here](http://mvapich.cse.ohio-state.edu/benchmarks/).
- `real_apps`: directory holding python classes that parse the outputs of
  - Graph500
  - MiniFE
  - AMG

  Which can be found [here](https://gitlab.com/domke/t2hx).
  
Finally there is a template `class_template.py` explaining the format of an application class, such that the corresponing application can be correctly integrated into the framework. Note an application class should be named "app".

## data directory
Holds the data collected on the slimfly for the bachelor thesis.\
The runner will dump the data here by default. It will create a sub-directory for each execution of the script holding a file with prefix `d` (actual data), a file with prefix `md` (meta data) and if specified a file with prefix `r` (runtime report: what was printed to the console during the execution).

## driver_files directory
Holds files that list paths to schedule-files (one per line) for which the the driver writes a script.

## node_files directory
Holds files that list available nodes on the system. Default format is one per line. If custom allocation is specified for the runner then one column per application/benchmark (csv style).

## plots directory
Holds plots used in the bachelor thesis. The plotter will dump created plots here.

## schedule_files directory
Holds schedule_files which are passed to the runner. \
Also includes `schedule_file_format_explained.txt` explaining the format and showing an example of the schedule-file.

## plotter_backup directory
Holds binaries of the data the plotter used for faster redoing & modifying of plots.

## scripts directory
Holds scripts used for experiments of the thesis. The driver will dump scripts here.

## specific_plotter directory
Holds additional visualization scripts used in the thesis.

## wl_manager directory
Holds workload manager classes that specify how runner can execute applications on the current system. \
Also includes a template `wlm_template.py` explaining the functions needed to properly integrate a new workload manager into the framework. Note a workload manager class should be named "wl_manager".

# How to add the support for a new system
Let's assume that the system name is newsystem'.
1) Add a newsystem.sh file in the conf/ directory. Copy the content from one of the existing files and adjust the parameters to match the new system.
2) Set the SYSTEM variable to "newsystem" in the conf.sh script.
.
# Credits
A preliminary version of this benchmark was used for the paper "An In-Depth Analysis of the Slingshot Interconnect" by Daniele De Sensi, Salvatore Di Girolamo, Kim H. McMahon, Duncan Roweth, Torsten Hoefler. The framework has been later extended by Loic Holbein as part of his bachelor thesis "Congestion Benchmarking and Visualization of Large-Scale Interconnection Networks". Many people contributed to the development of the framework and the applications/benchmarks it supports. 

- 