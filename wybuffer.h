#ifndef WY_BUFFER_H
#define WY_BUFFER_H

#include "common.h"

namespace wynet
{

const size_t MaxBufferSize = 1024 * 1024 * 256; // 256 MB

class Buffer
{
    uint8_t *buffer;
    size_t size;

  public:
    Buffer()
    {
        size = 0xff;
        buffer = new uint8_t[size]{0};
    }

    uint8_t *reserve(size_t n)
    {
        if (n > MaxBufferSize)
        {
            return nullptr;
        }
        if (!buffer || n > size)
        {
            while (n > size)
            {
                size = size << 1;
            }
            delete[] buffer;
            buffer = new uint8_t[size]{0};
        }
        return buffer;
    }
};

class BufferSet
{
    std::vector<Buffer> buffers;

  public:
    BufferSet(int bufferNum = 8)
    {
        buffers.resize(bufferNum);
    }

    uint8_t *reserve(uint8_t bufferID, size_t n)
    {
        return buffers[bufferID].reserve(n);
    }
};

// TODO
static BufferSet gBufferSet;
};

#endif