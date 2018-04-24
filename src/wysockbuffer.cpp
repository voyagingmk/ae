#include "wysockbuffer.h"

using namespace wynet;

// Warning: must consume queued packet before call readIn
//  0: closed
// -1: error
//  1: has available packet
//  2: EAGAIN
int SockBuffer::readIn(int sockfd, int *nreadTotal)
{
    *nreadTotal = 0;
    do
    {
        /*
            int required = -1;
            int ret = validatePacket(&required); // just to get required bytes
            log_debug("validatePacket ret %d required %d", ret, required);
            if (ret == -1)
            {
                // error
                return -1;
            }
            else if (ret == 0 && required == 0)
            {
                return 1;
            }
            int npend;
            ioctl(sockfd, FIONREAD, &npend);
            // make sure there is enough space for recv
            while (npend > leftSpace())
            {
                m_bufRef->expand(m_bufRef->length() + npend);
            }
            log_debug("readIn npend %d", npend);
            int nread = recv(sockfd, m_bufRef->data() + m_pos, required, 0);
            log_debug("recv nread %d required %d m_pos %d", nread, required, m_pos);
            if (nread == 0)
            {
                // closed
                return 0;
            }
            if (nread < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return 2;
                }
                else
                {
                    err_msg("[SockBuffer] sockfd %d readIn err: %d %s",
                            sockfd, errno, strerror(errno));
                    return -1;
                }
            }*/
        // nreadTotal += nread;
        // m_pos += nread;
    } while (1);
    return -1;
}

/*
// -2: receiving
// -1: error
//  0: ok
int SockBuffer::validatePacket(int *required)
{
    if (m_pos < HeaderBaseLength)
    {
        // receiving pakcet header base
        *required = HeaderBaseLength - m_pos;
        return -2;
    }
    const uint8_t *pData = m_bufRef->data();

    for (int i = 0; i < m_pos; i++)
    {
        //    printf("--%hhu\n", *(pData + i));
    }
    PacketHeader *header = (PacketHeader *)(pData);
    for (int i = 0; i < m_pos; i++)
    {
        //    printf("==%hhu\n", *(pData + i));
    }
    if (m_pos < header->getHeaderLength())
    {
        // receiving pakcet header options
        *required = header->getHeaderLength() - m_pos;
        return -2;
    }
    if (!header->isFlagOn(HeaderFlag::PacketLen))
    {
        // log_error("header->isFlagOn PacketLen false, flag: %u", header->flag);
        // error: no packetLen flag
        return -1;
    }
    // check length
    uint32_t packetLen = header->getUInt(HeaderFlag::PacketLen);
    if (m_pos < packetLen)
    {
        // receiving pakcet pData
        *required = packetLen - m_pos;
        return -2;
    }
    *required = 0;
    // TODO:validate whether it is a legal packet
    return 0;
}*/