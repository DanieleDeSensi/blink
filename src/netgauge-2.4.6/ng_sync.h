/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

void ng_sync_init_stage1(MPI_Comm comm);
void ng_sync_init_stage2(MPI_Comm comm, unsigned long long *win);
long ng_sync(unsigned long long win);
