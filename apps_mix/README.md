# Users can specify applications to be run together (the application mix) by writing a small file. 
# The first line of the file is the delimiter. If the line is empty, by default the comma (',') is used
# After the first line, each subsequent line specifies an application to be run 
# (an arbitrary number of lines can be specified).
# Each line has the following fields (separated by the delimiter):
#     - path: path to a Python file describing how to run the application and how to collect/parse its 
#             output. The path is relative to the root directory of the repository. Please refer to
#             the apps/README.md file for more information on how to create such a file.
#     - args: string of arguments to be passed to the application.
#     - collection-flag: '1' if the timings of the application should be collected, '0' otherwise.
#                        (i.e., 1 if the application is the victim, 0 otherwise)    
#     - start: After how many second from the start of the benchmark the application should start.
#              If not specified, the benchmark assumes a 0 (i.e., it starts at the beginning of the benchmark).
#     - end: After how many second from the start of the benchmark the application should start. 
#            If empty, the framework will never kill the application.
#            If 'f', the application will be killed only after all the other applications (excluding those 
#            that specified end=f) have terminated  or were killed. 
#
# E.g., if using the comma as delimiter a possible configuration file would look as follows:
,
./apps/internals/a2a_b/a2a_b_class.py,-msgsize 1048576 -iter 100 -grty 1,1,5,
./apps/internals/inc_b/inc_b_class.py,-msgsize 131072 -endl -iter 1 -grty 1,0,0,f
# This will execute the a2a_b benchmark and collect data from it after 5 seconds, 
# while inc_b is running since it was started at 0 and will terminate after a2a_b has +
# finished
