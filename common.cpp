#include "common.h"

UDPServer::UDPServer(int port) {
    m_sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&m_sockaddr, sizeof(m_sockaddr));
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_sockaddr.sin_port = htons(port);
    Bind(m_sockfd, (struct sockaddr*)&m_sockaddr, sizeof(m_sockaddr));
}

UDPClient::UDPClient(char *host, int port) {
    h = gethostbyname(host);
    if(h == NULL) {
        printf("unknown host '%s' \n", host);
        exit(1);
    }
    printf("host: '%s'  IP : %s \n",  h->h_name,
	 inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));
}


KCPObject::KCPObject(int conv, int interval) {
    m_kcp = ikcp_create(conv, this);
    m_kcp->output = KCPObject::kcp_output;
    ikcp_wndsize(m_kcp, 128, 128);
    ikcp_nodelay(m_kcp, 0, interval, 0, 0);
}

KCPObject::~KCPObject() {
    ikcp_release(m_kcp);
    m_kcp = NULL;
}

const ikcpcb * KCPObject::kcp() {
    return m_kcp;
}

void KCPObject::bindSocket(SocketBase* s) {
    m_socket = s;
}

void KCPObject::sendPackage(const char *buf, int len)
{     
    //  Sendto(m_socket->m_sockfd, buf, len, 0, pcliaddr, len);
    // ptr->send_udp_packet(std::string(buf, len), udp_remote_endpoint_);
}