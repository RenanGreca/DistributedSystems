all: mutex 
	
mutex: mutex.o smpl.o rand.o
	$(LINK.c) -o $@ -Bstatic mutex.o smpl.o rand.o -lm

smpl.o: smpl.c smpl.h
	$(COMPILE.c)  -g smpl.c

tempo.o: tempo.c smpl.h
	$(COMPILE.c) -g  tempo.c

rand.o: rand.c
	$(COMPILE.c) -g rand.c

clean:
	$(RM) *.o mutex relat saida
