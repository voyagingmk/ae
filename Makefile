
default: test

test : example.o ae.o zmalloc.o
	cc -o test example.o ae.o zmalloc.o

example.o : example.c
	cc -c example.c
zmalloc.o : zmalloc.c zmalloc.h
	cc -c zmalloc.c

clean : rm test example.o ae.o zmalloc.o