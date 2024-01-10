
CC=/scratch/2/t2hx/dep/openmpi/bin/mpicc
CFLAGS=-O3 -std=c99
LIBS=

halo3d: halo3d.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm halo3d

