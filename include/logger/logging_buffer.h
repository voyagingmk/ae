#ifndef WY_LOGGING_BUFFER_H
#define WY_LOGGING_BUFFER_H

#include <cstddef>
#include "../wybuffer.h"

namespace wynet
{

using namespace std;

class LoggingBuffer : public StaticBuffer<4 * 1024 * 1024>
{
  public:
    LoggingBuffer() : used(0)
    {
    }

    ~LoggingBuffer()
    {
    }

    void append(const char *buf, size_t len)
    {
        if (leftOpacity() > len)
        {
            memcpy(data() + used, buf, len);
            used += len;
        }
    }

    int length() const
    {
        return used;
    }

    char *current()
    {
        return (char *)data() + used;
    }

    size_t leftOpacity() const
    {
        return length() - used;
    }

    void add(size_t len)
    {
        used += len;
    }

    void reset()
    {
        used = 0;
    }

    void clean()
    {
        used = 0;
        StaticBuffer::clean();
    }

    const char *debugString();

    string toString() const
    {
        return string((const char *)data(), length());
    }

  private:
    const char *end() const
    {
        return (const char *)data() + length();
    }

    size_t used;
};
};

#endif
