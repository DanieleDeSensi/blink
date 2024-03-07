#SBATCH --time=01:00:00
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --partition=boost_usr_prod 
#SBATCH --exclusive
#SBATCH --job-name=blink-intra-node
#SBATCH --cpu-bind=map_cpu:1,2,17,18

ARGS="auto -n 1 -sp 100 -am l --ppn 4 -ro +file"

./run.sh test_suites/gpubench ${ARGS}
./run.sh test_suites/nccl_sendrecv ${ARGS}
./run.sh test_suites/nccl_coll ${ARGS}