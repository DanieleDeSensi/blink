
int DYN_Recv(MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
             void **buf, int *count, MPI_Status *status );
int DYN_Irecv(MPI_Datatype datatype, int src, int tag, MPI_Comm comm,
             void **buf, int *count, int *flag, MPI_Request *req );
int DYN_Send(void *buf, int count, MPI_Datatype datatype, int dest, 
                  int tag, MPI_Comm comm);
int DYN_Init_comm(int flags, MPI_Comm comm);
