#include <cstring>
#include <cerrno>
#include <algorithm>
#include <unistd.h>
#include <netinet/in.h>

#include "com/antlersoft/net/ReadStream.h"
#include "com/antlersoft/MyException.h"
#include "com/antlersoft/Trace.h"

#ifdef COM_OUT
#include <iostream>
#endif

using namespace std;
using namespace com::antlersoft::net;

bool ReadStream::available()
{
#ifdef COM_OUT
cerr<<"Queue size "<<m_queue.size()<<" Queue contents: "<<endl;
for ( deque<char>::iterator i=m_queue.begin(); i!=m_queue.end(); ++i)
{
	int ch= (*i) & 0xff;
	cerr<<ch;
	char buf[2];
	buf[1]=0;
	if ( ch>31 && ch<127)
	{
		buf[0]=ch;
		cerr<<"("<<buf<<")";
	}
	cerr<<",";
}
cerr<<endl;
#endif
	return m_queue.size()!=0;
}

short ReadStream::readShort()
{
	int x=m_queue.front() & 0xff;
	m_queue.pop_front();
	int y=m_queue.front() & 0xff;
	m_queue.pop_front();

	return (x<<8)+y;
}

long ReadStream::readLong()
{
	long result;
	char* buf=reinterpret_cast<char*>( &result);
	buf[0]=m_queue.front();
	m_queue.pop_front();
	buf[1]=m_queue.front();
	m_queue.pop_front();
	buf[2]=m_queue.front();
	m_queue.pop_front();
	buf[3]=m_queue.front();
	m_queue.pop_front();

	return ntohl( result);
}

string ReadStream::readString()
{
	Trace trace( "ReadStream::readString()");
	long length=readLong();
	deque<char>::iterator i_begin( m_queue.begin());
	deque<char>::iterator i_end( i_begin+length);
	string result( i_begin, i_end);
	m_queue.erase( i_begin, i_end);
	return result;
}

void ReadStream::readBytes( char* buffer, int length)
{
	deque<char>::iterator i_begin( m_queue.begin());
	deque<char>::iterator i_end( i_begin+length);
	copy( i_begin, i_end, buffer);
	m_queue.erase( i_begin, i_end);
}

void ReadStream::fill( pollfd& poll_struct)
{
	Trace trace( "ReadStream::fill");
	if ( poll_struct.revents & POLLIN)
	{
		ssize_t read_len;
		if ( m_read_count<4)
		{
			read_len=read( poll_struct.fd, m_read_buf+m_read_count,
				4-m_read_count);
			if ( read_len<0)
				throw MY_EXCEPTION_ERRNO;
			if ( read_len==0)
				throw MY_EXCEPTION( "End of file in ReadStream::fill");
#ifdef COM_OUT
cerr<<"Read size "<<read_len<<" read contents: "<<endl;
for ( int i=0; i!=read_len; ++i)
{
	int ch= m_read_buf[m_read_count+i];
	cerr<<ch;
	char buf[2];
	buf[1]=0;
	if ( ch>31 && ch<127)
	{
		buf[0]=ch;
		cerr<<"("<<buf<<")";
	}
	cerr<<",";
}
cerr<<endl;
#endif
			m_read_count+=read_len;
			if ( m_read_count<4)
				return;
		}
		size_t length= ntohl( *reinterpret_cast<long*>( m_read_buf));
		if ( length<0 || length>50000000)
			throw MY_EXCEPTION( "Bad read length");
		if ( length+4>m_read_size)
		{
			m_read_size=length+4;
			char* temp_buf=new char[m_read_size];
			memmove( temp_buf, m_read_buf, m_read_count);
			delete[] m_read_buf;
			m_read_buf=temp_buf;
		}
		read_len=read( poll_struct.fd, m_read_buf+m_read_count,
			length+4-m_read_count);
#ifdef COM_OUT
cerr<<"Read size "<<read_len<<" read contents: "<<endl;
for ( int i=0; i!=read_len; ++i)
{
	int ch= m_read_buf[m_read_count+i];
	cerr<<ch;
	char buf[2];
	buf[1]=0;
	if ( ch>31 && ch<127)
	{
		buf[0]=ch;
		cerr<<"("<<buf<<")";
	}
	cerr<<",";
}
cerr<<endl;
#endif
		if ( read_len<0)
		{
			if ( errno!=EAGAIN)
				throw MY_EXCEPTION_ERRNO;
			read_len=0;
		}
		m_read_count+=read_len;
		if ( m_read_count==length+4)
		{
			m_queue.insert( m_queue.end(), m_read_buf+4,
				m_read_buf+m_read_count);
			m_read_count=0;
		}
	}
}
