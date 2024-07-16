#ifndef COM_ANTLERSOFT_NET_WRITE_STREAM_H
#define COM_ANTLERSOFT_NET_WRITE_STREAM_H

#include <string>
#include <deque>
#include "com/antlersoft/net/PollConfig.h"

namespace com { namespace antlersoft { namespace net {

class WritePacket
{
private :
	char* m_buf;
	size_t m_size;
	size_t m_written;
public :
	WritePacket( std::deque<char>& queue);

	~WritePacket()
	{
		delete[] m_buf;
	}

	bool sendPacket( int sock);
};

class WriteStream
{
private :
	std::deque<char> m_queue;
	std::deque<WritePacket*> m_packet_queue;

public :
	WriteStream( )
	{
	}
	~WriteStream()
	{
		for ( std::deque<WritePacket*>::iterator i=m_packet_queue.begin();
			i!=m_packet_queue.end(); i++)
		{
			delete *i;
		}
	}
	bool pendingWrite()
	{
		return m_packet_queue.size()>0;
	}
	void writeString( std::string message);
	void writeShort( short s);
	void writeLong( long l);
	void writeBytes( char* buffer, int length);
	void send();
	void empty( pollfd& poll_struct);
};

} } }
#endif
