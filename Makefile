
all: example_client example_server

example_client : example_client.o ae.o zmalloc.o ikcp.o error.o wrapsock.o
	g++ $^ -o example_client

example_server : example_server.o ae.o zmalloc.o ikcp.o error.o wrapsock.o
	g++ $^ -o example_server

example_client.o : example_client.cpp
	g++ -c example_client.cpp

example_server.o : example_server.cpp
	g++ -c example_server.cpp

ae.o : ae.c ae.h
	g++ -x c -c ae.c

error.o : error.c unp.h
	g++ -x c -c error.c unp.h

wrapsock.o : wrapsock.c unp.h
	g++ -x c -c wrapsock.c unp.h

zmalloc.o : zmalloc.c zmalloc.h
	g++ -x c -c zmalloc.c

ikcp.o : ikcp.c ikcp.h
	g++ -x c -c ikcp.c
	
.PHONY: clean

clean : 
	rm -f *.o