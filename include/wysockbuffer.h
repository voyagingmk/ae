#ifndef WY_SOCKET_BUFFER_H
#define WY_SOCKET_BUFFER_H

#include "common.h"
#include "noncopyable.h"
#include "wybuffer.h"

namespace wynet
{

/*
    一个连接只在一开始有large级别的数据，之后就都是small数据或者idle了
    怎么利用这些空闲的内存：
    1.超时检查，resize小的，并把剩余copy，释放掉内存，还给系统
    2.
*/

// 支持读/写的buffer
/*

read data from socket into buffer & read data from buffer into callback:

[ free(read) | data | free ]

write data from user into buffer & send data from buffer into socket:

[ free(sent) | data | free ]

when to move data to head:

Condition 1: need to append x bytes
Condition 2: has no tail free space
Condition 3: head free space >= x

when to do real append:

Condition 1: need to append x bytes
Condition 2: has no tail free space
Condition 3: head free space < x (difference)


*/
class SockBuffer : public Noncopyable
{
  public:
    BufferRef m_bufRef;
    size_t m_pos1;
    size_t m_pos2;

  public:
    SockBuffer() : m_pos1(0),
                   m_pos2(0)
    {
    }
    SockBuffer &operator=(SockBuffer &&s)
    {
        m_bufRef = std::move(s.m_bufRef);
        m_pos1 = s.m_pos1;
        m_pos2 = s.m_pos2;
        s.m_pos1 = 0;
        s.m_pos2 = 0;
        return *this;
    }

    SockBuffer(SockBuffer &&s)
    {
        m_bufRef = std::move(s.m_bufRef);
        m_pos1 = s.m_pos1;
        m_pos2 = s.m_pos2;
        s.m_pos1 = 0;
        s.m_pos2 = 0;
        s.m_pos1 = 0;
    }

    inline BufferRef &getBufRef()
    {
        return m_bufRef;
    }

    inline int headFreeSize()
    {
        return m_pos1;
    }

    inline int tailFreeSize()
    {
        return m_bufRef->length() - m_pos2;
    }

    inline int readableSize()
    {
        return m_pos1;
    }

    inline void resetBuffer()
    {
        m_pos1 = 0;
        m_pos2 = 0;
    }

    uint8_t *begin()
    {
        return &*(m_bufRef->getDataVector().begin());
    }

    void append(uint8_t *data, size_t n);

    // return value = readv/read
    size_t readIn(int sockfd, int *nreadTotal);
};
};

#endif