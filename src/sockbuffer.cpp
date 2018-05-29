#include "sockbuffer.h"
#include <sys/uio.h>

using namespace wynet;

void SockBuffer::append(const uint8_t *data, size_t n)
{
    if (tailFreeSize() > 0)
    {
        size_t size = static_cast<size_t>(tailFreeSize());
        if (n <= size)
        {
            std::copy(data, data + n, begin() + m_pos2);
            m_pos2 += n;
        }
        else
        {
            std::copy(data, data + size, begin() + m_pos2);
            m_pos2 += size;
            int remain = n - size;
            append(data + size, remain);
        }
    }
    else
    {
        std::vector<uint8_t> &dataVec = m_bufRef->getDataVector();
        // make sure there is enough space
        if (static_cast<size_t>(headFreeSize()) >= n)
        {
            // move
            std::copy(dataVec.begin() + m_pos1, dataVec.end(), dataVec.begin());
            m_pos2 = m_pos2 - m_pos1;
            m_pos1 = 0;
        }
        else
        {
            dataVec.resize(dataVec.size() + n);
        }
        std::copy(data, data + n, begin() + m_pos2);
        m_pos2 += n;
    }
}

size_t SockBuffer::readIn(int sockfd)
{
    const size_t free = tailFreeSize();
    uint8_t *buf = m_bufRef->data() + m_pos2;
    const int stackBufLength = 65536;
    char stackBuf[stackBufLength];
    struct iovec iov[2];
    iov[0].iov_base = buf;
    iov[0].iov_len = free;
    iov[1].iov_base = stackBuf;
    iov[1].iov_len = stackBufLength;
    const int iovcnt = (free < stackBufLength) ? 2 : 1;
    const int nRead = ::readv(sockfd, iov, iovcnt);
    // log_debug("readIn, nRead=%d, %d,%d,%d,%d", nRead, free, m_pos1, m_pos2, m_bufRef->length());
    if (nRead > 0)
    {
        if (static_cast<size_t>(nRead) <= free)
        {
            m_pos2 += nRead;
        }
        else
        {
            m_pos2 += free;
            const size_t nStackBufUsed = static_cast<size_t>(nRead) - free;
            // log_debug("readIn, nRead %d free %d", nRead, free);
            // log_debug("readIn, m_pos2=%d, nStackBufUsed=%d", m_pos2, nStackBufUsed);
            append((uint8_t *)stackBuf, nStackBufUsed);
        }
    }
    else if (nRead < 0)
    {
        // log_error("%s", explain_readv(errno, sockfd, iov, iovcnt));
    }
    return nRead;
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