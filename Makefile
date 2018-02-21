
all: example_client example_server


OBJS = ae.o zmalloc.o ikcp.o error.o wrapsock.o sock_ntop.o uniqid.o  \
	wykcp.o wyudpserver.o wyudpclient.o wynet.o wyserver.o

example_client : example_client.o ${OBJS}
	g++ -g3 -o example_client  example_client.o ${OBJS}

example_server : example_server.o ${OBJS}
	g++ -g3 -o example_server example_server.o ${OBJS}

example_client.o : example_client.cpp
	g++ -g3 -c example_client.cpp

example_server.o : example_server.cpp
	g++ -g3 -c example_server.cpp


wynet.o : wynet.cpp 
	g++ -g3 -c wynet.cpp



wyserver.o : wyserver.cpp 
	g++ -g3 -c wyserver.cpp


common.o : common.cpp 
	g++ -g3 -c common.cpp
	
wyudpserver.o : wyudpserver.cpp 
	g++ -g3 -c wyudpserver.cpp
	

wyudpclient.o : wyudpclient.cpp 
	g++ -g3 -c wyudpclient.cpp
	

wykcp.o : wykcp.cpp 
	g++ -g3 -c wykcp.cpp


uniqid.o : uniqid.cpp 
	g++ -g3 -c uniqid.cpp


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