#!/bin/bash
#SBATCH --partition=cn-eth
#SBATCH --ntasks=4             
#SBATCH --nodes=4                 
#SBATCH --ntasks-per-node=1           
#SBATCH --time=00:10:00         
#SBATCH --exclusive      

module load GCC
module load OpenMPI

#export OMPI_MCA_base_verbose=100
#export OMPI_MCA_btl_base_verbose=100

#export OMPI_MCA_coll_base_verbose=1
#export OMPI_MCA_coll_tuned_use_dynamic_rules=1
#export OMPI_MCA_coll_tuned_allreduce_algorithm=4
#export OMPI_MCA_coll_tuned_allreduce_algorithm_segmentsize=4096
#export OMPI_MCA_coll_tuned_allreduce_algorithm_tree_fanout=4
#export OMPI_MCA_coll_tuned_dynamic_rules_filename="/home/lpiarulli/rules.txt"

#./run.sh test_suites/lor-cpu-ardc_b-a2a_b-agtr_b auto -n 4 -sp 100 -am l -ro +file -e cn-ib
./run.sh test_suites/lor-cpu-ardc_b-a2a_b-agtr_b auto -n 4 -sp 100 -am l -ro +file -e cn-eth

#--mca coll_base_verbose 1

#ompi_info --param coll tuned --level 9      to see available algorithms

# Set the mpirun flags to change the algorithm of all reduce
#--mca coll_tuned_use_dynamic_rules 1 \
#--mca coll_tuned_allreduce_algorithm 3 \
#--mca coll_tuned_allreduce_algorithm_segmentsize 4096 \
#--mca coll_tuned_allreduce_algorithm_tree_fanout 4
