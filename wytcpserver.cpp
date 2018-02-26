#include "wytcpserver.h"

namespace wynet {


TCPServer::TCPServer(int port)
{
	int				listenfd, n;
	const int		on = 1;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    const char *host = NULL;
    char buf[5];
    sprintf(buf, "%d", port);
    const char *serv = (char *)&buf;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("tcp_listen error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		listenfd = socket(res->ai_family, res->ai_socktype | SOCK_NONBLOCK, res->ai_protocol);
		if (listenfd < 0)
			continue;		/* error, try next one */

		Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		Close(listenfd);	/* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno from final socket() or bind() */
		err_sys("tcp_listen error for %s, %s", host, serv);

	Listen(listenfd, LISTENQ);

    m_sockfd = listenfd;
    m_family = res->ai_family;
    memcpy(&m_sockaddr, res->ai_addr, res->ai_addrlen);
	m_socklen = res->ai_addrlen;	/* return size of protocol address */

	freeaddrinfo(ressave);

    char *str = Sock_ntop((struct sockaddr *)&m_sockaddr, m_socklen);
    printf("TCP Server created: %s\n", str);
}


TCPServer::~TCPServer()
{
    close(m_sockfd);
}

};
