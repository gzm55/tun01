#ifndef COM_ANTLERSOFT_NET_READ_STREAM_H
#define COM_ANTLERSOFT_NET_READ_STREAM_H

#include <deque>
#include <string>
#include "com/antlersoft/net/PollConfig.h"

namespace com { namespace antlersoft { namespace net {

class ReadStream
{
private :
	static const size_t BUFFER_SIZE = 4096;
	std::deque<char> m_queue;
	char* m_read_buf;
	size_t m_read_size;
	size_t m_read_count;

public :
	ReadStream()
		: 
		m_read_size( BUFFER_SIZE),
		m_read_buf( new char[BUFFER_SIZE]),
		m_read_count(0)
	{
	}
	~ReadStream()
	{
		delete[] m_read_buf;
	}
	bool available();
	short readShort();
	long readLong();
	std::string readString();
	void fill( pollfd& poll_struct);
	void readBytes( char* buffer, int count);
};

} } }
#endif
