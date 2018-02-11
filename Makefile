
all: example_client example_server


OBJS = ae.o zmalloc.o ikcp.o error.o wrapsock.o common.o sock_ntop.o

example_client : example_client.o ${OBJS}
	g++ -g3 -o example_client  example_client.o ${OBJS}

example_server : example_server.o ${OBJS}
	g++ -g3 -o example_server example_server.o ${OBJS}

example_client.o : example_client.cpp
	g++ -g3 -c example_client.cpp

example_server.o : example_server.cpp
	g++ -g3 -c example_server.cpp


common.o : common.cpp 
	g++ -g3 -c common.cpp


ae.o : ae.c
	g++ -g3 -x c -c ae.c

error.o : error.c
	g++ -g3 -x c -c error.c

wrapsock.o : wrapsock.c
	g++ -g3 -x c -c wrapsock.c

zmalloc.o : zmalloc.c
	g++ -g3 -x c -c zmalloc.c

ikcp.o : ikcp.c
	g++ -g3 -x c -c ikcp.c
	
sock_ntop.o : sock_ntop.c
	g++ -g3 -x c -c sock_ntop.c

.PHONY: clean

clean : 
	rm -f *.o