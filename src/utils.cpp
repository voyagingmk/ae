#include "utils.h"

namespace wynet
{

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

} // namespace wynet
