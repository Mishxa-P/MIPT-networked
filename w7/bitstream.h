#pragma once
#include <cstdint>
#include <cassert>
#include <cstring> // memcpy

class Bitstream
{
public:
	Bitstream(void* data, size_t size) :
		m_buffer((uint8_t*)data), m_bufferSize(size)
	{
	}

	template <typename T>
	void Write(const T& val)
	{
		assert(m_bufferSize >= sizeof(T));
		memcpy(m_buffer, &val, sizeof(T));
		m_buffer += sizeof(T);
		m_bufferSize -= sizeof(T);
	}

    void WritePackedUint16(uint16_t v)
    {
        assert(v < UINT16_MAX / 2);
        if (v < UINT8_MAX / 2)
        {
            uint8_t val = v;
            memcpy(m_buffer, &val, sizeof(uint8_t));
            m_buffer += sizeof(uint8_t);
            m_bufferSize -= sizeof(uint8_t);
            return;
        }
        if (v < UINT16_MAX / 2)
        {
            uint16_t val = (1 << 15) + v;
            memcpy(m_buffer, &val, sizeof(uint16_t));
            m_buffer += sizeof(uint16_t);
            m_bufferSize -= sizeof(uint16_t);
            return;
        }
        std::cout << "Can't pack value " << v << '\n';
        assert(false);
    }

    void WritePackedUint32(uint32_t v)
    {
        assert(v < UINT32_MAX / 4);
        if (v < UINT8_MAX / 2)
        {
            uint8_t val = v;
            memcpy(m_buffer, &val, sizeof(uint8_t));
            m_buffer += sizeof(uint8_t);
            m_bufferSize -= sizeof(uint8_t);
            return;
        }
        if (v < UINT16_MAX / 4)
        {
            uint16_t val = (2 << 14) + v;
            memcpy(m_buffer, &val, sizeof(uint16_t));
            m_buffer += sizeof(uint16_t);
            m_bufferSize -= sizeof(uint16_t);
            return;
        }
        if (v < UINT32_MAX / 4)
        {
            int32_t val = (3 << 30) + v;
            memcpy(m_buffer, &val, sizeof(int32_t));
            m_buffer += sizeof(int32_t);
            m_bufferSize -= sizeof(int32_t);
            return;
        }
        std::cout << "Can't pack value " << v << '\n';
        assert(false);
    }
  
	template <typename T>
	void Read(T& val)
	{
		assert(m_bufferSize >= sizeof(T));
		memcpy(&val, m_buffer, sizeof(T));
		m_buffer += sizeof(T);
		m_bufferSize -= sizeof(T);
	}

    void ReadPackedUint16(uint16_t& v)
    {
        assert(m_bufferSize > 0);
        uint8_t val8;
        memcpy(reinterpret_cast<uint8_t*>(&val8), m_buffer, sizeof(uint8_t));
        uint8_t val_for_check = val8;
        if (val_for_check >> 7 == 0)
        {
            v = val8;
            m_buffer += sizeof(uint8_t);
            m_bufferSize -= sizeof(uint8_t);
            return;
        }
        if (val_for_check >> 7 == 1)
        {
            uint16_t val16;
            memcpy(reinterpret_cast<uint8_t*>(&val16), m_buffer, sizeof(uint16_t));
            v = val16 & 0x7FFF;
            m_buffer += sizeof(uint16_t);
            m_bufferSize -= sizeof(uint16_t);
            return;
        }

        std::cout << "Can't unpack value " << v << '\n';
        assert(false);
    }

    void ReadPackedUint32(uint32_t& v)
    {
        assert(m_bufferSize > 0);
        uint8_t val8;
        memcpy(reinterpret_cast<uint8_t*>(&val8), m_buffer, sizeof(uint8_t));
        uint8_t val_for_check = val8;
        if (val_for_check >> 7 == 0)
        {
            v = val8;
            m_buffer += sizeof(uint8_t);
            m_bufferSize -= sizeof(uint8_t);
            return;
        }
        if (val_for_check >> 6 == 2)
        {
            uint16_t val16;
            memcpy(reinterpret_cast<uint8_t*>(&val16), m_buffer, sizeof(uint16_t));
            v = val16 & 0x3FFF;
            m_buffer += sizeof(uint16_t);
            m_bufferSize -= sizeof(uint16_t);
            return;
        }
        if (val_for_check >> 6 == 3)
        {
            uint32_t val32;
            memcpy(reinterpret_cast<uint8_t*>(&val32), m_buffer, sizeof(uint32_t));
            v = val32 & 0x3FFFFFFF;
            m_buffer += sizeof(uint32_t);
            m_bufferSize -= sizeof(uint32_t);
            return;
        }

        std::cout << "Can't unpack value " << v << '\n';
        assert(false);
    } 

	template <typename T>
	void Skip()
	{
		assert(m_bufferSize >= sizeof(T));
		m_buffer += sizeof(T);
		m_bufferSize -= sizeof(T);
	}

private:
	uint8_t* m_buffer;
	size_t m_bufferSize;
};
