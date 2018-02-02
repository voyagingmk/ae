
all: example_client example_server

example_client : example_client.o ae.o zmalloc.o ikcp.o
	cc -o example_client example_client.o ae.o zmalloc.o ikcp.o

example_server : example_server.o ae.o zmalloc.o ikcp.o
	cc -o example_server example_server.o ae.o zmalloc.o ikcp.o

example_client.o : example_client.c
	cc -c example_client.c

example_server.o : example_server.c
	cc -c example_server.c

zmalloc.o : zmalloc.c zmalloc.h
	cc -c zmalloc.c

ikcp.o : ikcp.c ikcp.h
	cc -c ikcp.c

clean : rm example_client example_server example_client.o example_server.o ae.o zmalloc.o ikcp.o