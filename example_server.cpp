#include "common.h"

#define MAX_MSG 100
int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    UDPServer server(Port);
    
	fd_set rset;
    struct sockaddr_in cliAddr;
    char msg[MAX_MSG];
    FD_ZERO(&rset);
	int maxfd = server.m_sockfd + 1;
    int nready;
    while(1) {
		FD_SET(server.m_sockfd, &rset);
        if ( (nready = select(maxfd, &rset, NULL, NULL, NULL)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				err_sys("select error");
		}
        if (FD_ISSET(server.m_sockfd, &rset)) {
            /* init buffer */
            memset(msg, 0x0, MAX_MSG);
            /* receive message */
            socklen_t len = sizeof(cliAddr);
            Recvfrom(server.m_sockfd, msg, MAX_MSG, 0, 
                (struct sockaddr *) &cliAddr, &len);
            
            /* print received message */
            printf("recv from UDP %s:%u : %s \n", 
                inet_ntoa(cliAddr.sin_addr),
                ntohs(cliAddr.sin_port),
                msg);
        }
        
    }
    /*
    KCPObject kcpObject(Conv, Interval);
    kcpObject.bindSocket(&server);

    loop = aeCreateEventLoop(64);
    aeMain(loop);*/
    return 0;
}

