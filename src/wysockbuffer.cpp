#include "wysockbuffer.h"
#include <sys/uio.h>

using namespace wynet;

void SockBuffer::append(uint8_t *data, size_t n)
{
    assert(tailFreeSize() == 0);
    std::vector<uint8_t> &dataVec = m_bufRef->getDataVector();
    // make sure there is enough space
    if (headFreeSize() >= n)
    {
        // move
        std::copy(dataVec.begin() + m_pos2, dataVec.end(), dataVec.begin());
        m_pos2 = m_pos2 - m_pos1;
        m_pos1 = 0;
    }
    else
    {
        dataVec.resize(dataVec.size() + n);
    }
    std::copy(data, data + n, begin());
    m_pos2 += n;
}

size_t SockBuffer::readIn(int sockfd)
{
    const size_t free = tailFreeSize();
    uint8_t *buf = m_bufRef->data() + m_pos1;
    const int stackBufLength = 65536;
    char stackBuf[stackBufLength];
    struct iovec vec[2];
    vec[0].iov_base = buf;
    vec[0].iov_len = free;
    vec[1].iov_base = stackBuf;
    vec[1].iov_len = stackBufLength;
    const int iovcnt = (free < stackBufLength) ? 2 : 1;
    const size_t nRead = (size_t)::readv(sockfd, vec, iovcnt);
    log_debug("readIn, nRead=%d", nRead);
    if (nRead > 0)
    {
        if (nRead <= free)
        {
            m_pos1 += nRead;
        }
        else
        {
            const int nStackBufUsed = nRead - free;
            append((uint8_t *)stackBuf, nStackBufUsed);
        }
    }
    return nRead;

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
            while (npend > tailFreeSize())
            {
                m_bufRef->expand(m_bufRef->length() + npend);
            }
            log_debug("readIn npend %d", npend);
            int nread = recv(sockfd, m_bufRef->data() + m_pos1, required, 0);
            log_debug("recv nread %d required %d m_pos1 %d", nread, required, m_pos1);
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
        // m_pos1 += nread;
    } while (1);
    return -1;
}

/*
// -2: receiving
// -1: error
//  0: ok
int SockBuffer::validatePacket(int *required)
{
    if (m_pos1 < HeaderBaseLength)
    {
        // receiving pakcet header base
        *required = HeaderBaseLength - m_pos1;
        return -2;
    }
    const uint8_t *pData = m_bufRef->data();

    for (int i = 0; i < m_pos1; i++)
    {
        //    printf("--%hhu\n", *(pData + i));
    }
    PacketHeader *header = (PacketHeader *)(pData);
    for (int i = 0; i < m_pos1; i++)
    {
        //    printf("==%hhu\n", *(pData + i));
    }
    if (m_pos1 < header->getHeaderLength())
    {
        // receiving pakcet header options
        *required = header->getHeaderLength() - m_pos1;
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
    if (m_pos1 < packetLen)
    {
        // receiving pakcet pData
        *required = packetLen - m_pos1;
        return -2;
    }
    *required = 0;
    // TODO:validate whether it is a legal packet
    return 0;
}*/