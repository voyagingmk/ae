#include "utils.h"

namespace wynet
{

int little_endian()
{
    int x = 0x1234;
    return *(char *)&x == 0x34;
}

int big_endian()
{
    return !little_endian();
}

void ignoreSignalPipe()
{
    ::signal(SIGPIPE, SIG_IGN);
}

std::string hostname()
{
    char buf[256];
    if (::gethostname(buf, sizeof buf) == 0)
    {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }
    else
    {
        return "unknownhost";
    }
}

void checkOpenFileNum(int expectedNum)
{
#if defined(__APPLE__)
    struct rlimit rlp;
    // fprintf(stderr, "checkOpenFileNum:\n");
    getrlimit(RLIMIT_NOFILE, &rlp);
    // fprintf(stderr, "before %d %d\n", rlp.rlim_cur, rlp.rlim_max);
    rlp.rlim_cur = 10000;
    setrlimit(RLIMIT_NOFILE, &rlp);
    getrlimit(RLIMIT_NOFILE, &rlp);
    // fprintf(stderr, "after %d %d\n", rlp.rlim_cur, rlp.rlim_max);
#endif
}

} // namespace wynet
