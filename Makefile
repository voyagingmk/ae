
all: example_client example_server

OBJS = ae.o zmalloc.o ikcp.o error.o wrapsock.o common.o

example_client : example_client.o ${OBJS}
	g++  -o example_client  example_client.o ${OBJS}

example_server : example_server.o ${OBJS}
	g++  -o example_server example_server.o ${OBJS}

example_client.o : example_client.cpp
	g++ -c example_client.cpp

example_server.o : example_server.cpp
	g++ -c example_server.cpp


common.o : common.cpp 
	g++ -c common.cpp


ae.o : ae.c
	g++ -x c -c ae.c

error.o : error.c
	g++ -x c -c error.c

wrapsock.o : wrapsock.c
	g++ -x c -c wrapsock.c

zmalloc.o : zmalloc.c
	g++ -x c -c zmalloc.c

ikcp.o : ikcp.c
	g++ -x c -c ikcp.c
	
.PHONY: clean

clean : 
	rm -f *.o