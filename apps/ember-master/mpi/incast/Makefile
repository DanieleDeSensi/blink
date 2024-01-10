
CC=/scratch/2/t2hx/dep/openmpi/bin/mpicc
CFLAGS=-O3 -std=c99
LIBS=

incast: incast.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm incast

