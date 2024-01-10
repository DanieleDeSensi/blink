BINS=inc_nb/inc_nb inc_b/inc_b inc_bsnbr/inc_bsnbr inc_put/inc_put inc_get/inc_get a2a_nb/a2a_nb a2a_man/a2a_man a2a_b/a2a_b o2o_bsnbr/o2o_bsnbr o2o_nb/o2o_nb bdc_b/bdc_b bdc_nb/bdc_nb ring_bsnbr/ring_bsnbr ring_nb/ring_nb null_dummy/null_dummy checker/checker ping-pong_b/ping-pong_b ardc_b/ardc_b ardc_nb/ardc_nb barrier/barrier pw-ping-pong_b/pw-ping-pong_b

all: $(BINS)

inc_nb/inc_nb: inc_nb/inc_nb.c
	$(CC) -o inc_nb/inc_nb inc_nb/inc_nb.c -lm
	
inc_b/inc_b: inc_b/inc_b.c
	$(CC) -o inc_b/inc_b inc_b/inc_b.c -lm

inc_bsnbr/inc_bsnbr: inc_bsnbr/inc_bsnbr.c
	$(CC) -o inc_bsnbr/inc_bsnbr inc_bsnbr/inc_bsnbr.c -lm

inc_put/inc_put: inc_put/inc_put.c
	$(CC) -o inc_put/inc_put inc_put/inc_put.c -lm

inc_get/inc_get: inc_get/inc_get.c
	$(CC) -o inc_get/inc_get inc_get/inc_get.c -lm

a2a_nb/a2a_nb: a2a_nb/a2a_nb.c
	$(CC) -o a2a_nb/a2a_nb a2a_nb/a2a_nb.c -lm
	
a2a_man/a2a_man: a2a_man/a2a_man.c
	$(CC) -o a2a_man/a2a_man a2a_man/a2a_man.c -lm
	
a2a_b/a2a_b: a2a_b/a2a_b.c
	$(CC) -o a2a_b/a2a_b a2a_b/a2a_b.c -lm
	
ardc_b/ardc_b: ardc_b/ardc_b.c
	$(CC) -o ardc_b/ardc_b ardc_b/ardc_b.c -lm
	
ardc_nb/ardc_nb: ardc_nb/ardc_nb.c
	$(CC) -o ardc_nb/ardc_nb ardc_nb/ardc_nb.c -lm

o2o_bsnbr/o2o_bsnbr: o2o_bsnbr/o2o_bsnbr.c
	$(CC) -o o2o_bsnbr/o2o_bsnbr o2o_bsnbr/o2o_bsnbr.c -lm

o2o_nb/o2o_nb: o2o_nb/o2o_nb.c
	$(CC) -o o2o_nb/o2o_nb o2o_nb/o2o_nb.c -lm

bdc_b/bdc_b: bdc_b/bdc_b.c
	$(CC) -o bdc_b/bdc_b bdc_b/bdc_b.c -lm

bdc_nb/bdc_nb: bdc_nb/bdc_nb.c
	$(CC) -o bdc_nb/bdc_nb bdc_nb/bdc_nb.c -lm
	
ring_bsnbr/ring_bsnbr: ring_bsnbr/ring_bsnbr.c
	$(CC) -o ring_bsnbr/ring_bsnbr ring_bsnbr/ring_bsnbr.c -lm

ring_nb/ring_nb: ring_nb/ring_nb.c
	$(CC) -o ring_nb/ring_nb ring_nb/ring_nb.c -lm
	
null_dummy/null_dummy: null_dummy/null_dummy.c
	$(CC) -o null_dummy/null_dummy null_dummy/null_dummy.c
	
checker/checker: checker/checker.c
	$(CC) -o checker/checker checker/checker.c -lm
	
ping-pong_b/ping-pong_b: ping-pong_b/ping-pong_b.c
	$(CC) -o ping-pong_b/ping-pong_b ping-pong_b/ping-pong_b.c -lm

pw-ping-pong_b/pw-ping-pong_b: pw-ping-pong_b/pw-ping-pong_b.c
	$(CC) -o pw-ping-pong_b/pw-ping-pong_b pw-ping-pong_b/pw-ping-pong_b.c -lm
	
barrier/barrier: barrier/barrier.c
	$(CC) -o barrier/barrier barrier/barrier.c -lm

clean:
	-rm -f $(BINS)
