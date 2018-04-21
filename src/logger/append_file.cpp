
#include "logger/append_file.h"
#include "wyutils.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace wynet;
using namespace std;

AppendFile::AppendFile(string &filename) : m_fp(::fopen(filename.c_str(), "ae")), // 'e' for O_CLOEXEC
                                           m_writtenBytes(0)
{
    assert(m_fp);
}

AppendFile::~AppendFile()
{
    ::fclose(m_fp);
}

void AppendFile::append(const char *logline, const size_t len)
{
    size_t written = write(logline, len);
    while (written < len)
    {
        size_t remain = len - written;
        size_t x = write(logline + written, remain);
        if (x == 0)
        {
            int err = ferror(m_fp);
            if (err)
            {
                fprintf(stderr, "AppendFile::append() failed %s\n", strerror(err));
            }
            break;
        }
        written += x;
    }
    m_writtenBytes += written;
}

void AppendFile::flush()
{
    ::fflush(m_fp);
}

size_t AppendFile::write(const char *logline, size_t len)
{
    return ::fwrite_unlocked(logline, sizeof(char), len, m_fp);
}