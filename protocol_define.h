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
    static const Protocol protocol = Protocol::Handshake;
};
};

template<class P>
class ProtocolWrapper
{
public:
    PacketHeader header;
    P content;
    ProtocolWrapper(P c, char* buf):
        content(c)
    {
        header.setProtocol(P::protocol);
        header.setFlag(HeaderFlag::PacketLen, true);
        header.updateHeaderLength();
        memcpy(buf, (uint8_t *)&header, header.getHeaderLength());
        memcpy(buf + header.getHeaderLength(),
               (uint8_t *)&content, sizeof(P));
    }
};
    
};

#endif
