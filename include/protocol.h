#ifndef WY_PROTOCOL_H
#define WY_PROTOCOL_H

#include "common.h"
#include <string>
#include <sstream>

namespace wynet
{
    
enum class HeaderFlag : uint32_t
{
    PacketLen = 1 << 0,
	Base64 = 1 << 1,
	Encrypt = 1 << 2,
	Compress = 1 << 3
};

static const size_t HeaderBaseLength = 8;
static const size_t HeaderDataLength = 0xff;

static const std::map<const HeaderFlag, const size_t> FlagToBytes{
    {HeaderFlag::PacketLen, 4}, // record packet size ( slice stream data )
	{HeaderFlag::Base64, 0},
	{HeaderFlag::Encrypt, 0},
	{HeaderFlag::Compress, 4} // record the uncompressed size
};

// for debug
static const std::map<const HeaderFlag, const std::string> FlagToStr{
    {HeaderFlag::PacketLen, "PacketLen"},
	{HeaderFlag::Base64, "Base64"},
	{HeaderFlag::Encrypt, "Encrypt"},
	{HeaderFlag::Compress, "Compress"},
};

// small endian
class PacketHeader
{
  public:
	PacketHeader(bool reset = true)
	{
        if (reset) {
            memset(data, 0, HeaderDataLength);
            version = 1;
            length = 0;
            protocol = 0;
            hash = 0;
            flag = 0;
        }
	}

	inline uint8_t getVersion() const
	{
		return version;
	}

	inline uint8_t getProtocol() const
	{
		return protocol;
	}

	inline void setProtocol(uint8_t p)
	{
		protocol = p;
	}

	inline uint8_t getHeaderLength() const
	{
		return length;
	}

	void updateHeaderLength()
	{
		length = calDataSize() + HeaderBaseLength;
	}

	bool isFlagOn(HeaderFlag f) const
	{
		return (flag & (uint32_t)f) > 0;
	}

	void cleanFlag()
	{
		flag = 0;
	}

	void setFlag(HeaderFlag f, bool on)
	{
		if (on)
		{
			flag = flag | uint32_t(f);
		}
		else
		{
			flag = flag & (~uint32_t(f));
		}
	}

	size_t getFlagPos(HeaderFlag f) const
	{
		size_t count = 0;
		for (size_t i = 0; i < 32; i++)
		{
			const HeaderFlag _f = HeaderFlag(1 << i);
			if (_f == f)
			{
				return count;
			}
			else if (flag & (uint32_t)_f)
			{
				count += FlagToBytes.at(_f);
			}
		}
		return 0;
	}

	size_t calDataSize() const
	{
		size_t bytesNum = 0;
		for (size_t i = 0; i < 32; i++)
		{
			const HeaderFlag _f = HeaderFlag(1 << i);
			if (flag & (uint32_t)_f)
			{
				bytesNum += FlagToBytes.at(_f);
			}
		}
		return bytesNum;
	}
    
    uint32_t getUInt(HeaderFlag f) const {
        size_t bytes = FlagToBytes.at(f);
        if (bytes == 1) {
            return (uint32_t)(getUInt8(f));
        } else if (bytes == 4) {
            return getUInt32(f);
        }
        return 0;
    }
    
    uint8_t getUInt8(HeaderFlag f) const {
        return *(data + getFlagPos(f));
    }

    void setUInt8(HeaderFlag f, uint8_t val) {
        *(data + getFlagPos(f)) = val;
    }
    
    uint32_t getUInt32(HeaderFlag f) const {
        return ntohl(*((uint32_t *)(data + getFlagPos(f))));
    }

    void setUInt32(HeaderFlag f, uint32_t val) {
        *((uint32_t *)(data + getFlagPos(f))) = htonl(val);
    }

	std::string getHeaderDebugInfo() const
	{
		std::string info = "";
        info += to_string(version) + "\n";
        info += to_string(length) + "\n";
        info += to_string(protocol) + "\n";
        info += to_string(hash) + "\n";
        info += to_string(flag) + "\n";
		for (auto it = FlagToBytes.begin(); it != FlagToBytes.end(); it++)
		{
			info += FlagToStr.at(it->first);
			if (isFlagOn(it->first))
			{
				info += " on";
			}
			else
			{
				info += " off";
			}
			info += "\n";
		}
		info += "headerLen = " + to_string(getHeaderLength());
        info += "packetLen = " + to_string(getUInt(HeaderFlag::PacketLen));
		return info;
	}
    
    template <typename T>
    std::string to_string(const T& value) const
    {
        std::ostringstream os;
        os << value;
        return os.str();
    }

  public:
	uint8_t version;
	uint8_t length;
	uint8_t protocol;
	uint8_t hash;
	uint32_t flag;
	uint8_t data[HeaderDataLength];
};
};

#endif
