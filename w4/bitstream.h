#pragma once
#include <cstdint>
#include <cassert>
#include <cstring> // memcpy

class Bitstream
{
public:
	Bitstream(void* data, size_t size):
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

	template <typename T>
	void Read(T& val)
	{
		assert(m_bufferSize >= sizeof(T));
		memcpy(&val, m_buffer, sizeof(T));
		m_buffer += sizeof(T);
		m_bufferSize -= sizeof(T);
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
