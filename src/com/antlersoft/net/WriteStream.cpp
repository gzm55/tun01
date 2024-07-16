#include <algorithm>
#include <cerrno>
#include <unistd.h>
#include <netinet/in.h>
#include "com/antlersoft/net/WriteStream.h"
#include "com/antlersoft/MyException.h"
#include "com/antlersoft/Trace.h"

using namespace std;
using namespace com::antlersoft::net;

WritePacket::WritePacket( deque<char>& queue)
	: m_size( queue.size()+4), m_written(0)
{
	m_buf=new char[m_size];
	*reinterpret_cast<size_t*>( m_buf)=htonl( m_size-4);
	copy( queue.begin(), queue.end(), m_buf+4);
	queue.clear();
}

bool WritePacket::sendPacket( int socket)
{
	Trace trace( "WritePacket::sendPacket");
	ssize_t write_length=write( socket, m_buf+m_written,
		m_size-m_written);
	if ( write_length<0)
		throw MY_EXCEPTION_ERRNO;
	m_written+=write_length;
	return m_written==m_size;
}

void WriteStream::writeString( string message)
{
	writeLong( message.length());
	m_queue.insert( m_queue.end(), message.begin(), message.end());
}

void WriteStream::writeBytes( char* buffer, int length)
{
	m_queue.insert( m_queue.end(), buffer, buffer+length);
}

void WriteStream::writeShort( short s)
{
	char buf[2];
	buf[0]=(s >> 8);
	buf[1]=( s & 0xff);
	m_queue.insert( m_queue.end(), buf, buf+2);
}

void WriteStream::writeLong( long s)
{
	s=htonl( s);
	char* i=reinterpret_cast<char*>( &s);
	m_queue.insert( m_queue.end(), i, i+4);
}

void WriteStream::send()
{
	Trace trace( "WriteStream::send");
	if ( m_queue.size()>0)
		m_packet_queue.push_back( new WritePacket( m_queue));
}

void WriteStream::empty( pollfd& poll_struct)
{
	Trace trace( "WriteStream::empty");
	if ( ( poll_struct.revents & POLLOUT) && pendingWrite())
	{
		WritePacket& front= *m_packet_queue.front();
		if ( front.sendPacket( poll_struct.fd))
		{
			delete &front;
			m_packet_queue.pop_front();
		}
		if ( ! pendingWrite())
			poll_struct.events &= ~POLLOUT;
	}
}
