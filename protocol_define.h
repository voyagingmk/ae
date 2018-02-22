#ifndef WY_PROTOCOL_DEFINE_H
#define WY_PROTOCOL_DEFINE_H
#include "common.h"

namespace wynet
{

namespace protocol
{

struct Handshake
{
    uint32_t clientID;
    uint16_t udpPort;
};
};
};

#endif