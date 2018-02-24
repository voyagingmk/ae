#include "wytcpclient.h"

namespace wynet {


TCPClient::TCPClient(const char *host, int port) {	
    int		n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    char buf[5];
    sprintf(buf, "%d", port);
    const char *serv = (char *)&buf;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("tcp_connect error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	ressave = res;

	do {
		m_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (m_sockfd < 0)
			continue;	/* ignore this one */

		if (connect(m_sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		Close(m_sockfd);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno set from final connect() */
		err_sys("tcp_connect error for %s, %s", host, serv);

    m_family = res->ai_family;
    memcpy(&m_sockaddr, res->ai_addr, res->ai_addrlen);
	m_socklen = res->ai_addrlen;	/* return size of protocol address */

	freeaddrinfo(ressave);
}

TCPClient::~TCPClient()
{
    close(m_sockfd);
}



void TCPClient::Recvfrom() {

}

void TCPClient::Send(const char *data, size_t len)
{
    ::Send(m_sockfd, data, len, 0);
    // Sendto(m_sockfd, data, len, 0, (struct sockaddr *)&m_sockaddr, m_socklen);
}


};
