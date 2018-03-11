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
    LoggingBuffer():
        used(0)
    {
    }

    ~LoggingBuffer()
    {
    }

    void append(const char *buf, size_t len)
    {
        if (leftOpacity() > len)
        {
            memcpy(m_data + used, buf, len);
            used += len;
        }
    }

    const char *data() const { 
        return (const char *)m_data; 
    }

    int length() const { 
        return used;
    }

    char *current() {
         return (char *)m_data + used;
    }

    size_t leftOpacity() const { 
        return m_size - used;
    }

    void add(size_t len) { 
        used += len; 
    }

    void clean() { 
        used = 0;
        StaticBuffer::clean();
    }

    const char *debugString();

    string toString() const { 
        return string((const char *)m_data, length());
    }

  private:
    
    const char *end() const { 
        return (const char *)m_data + m_size; 
    }
    
    size_t used;
};
    
};

#endif
