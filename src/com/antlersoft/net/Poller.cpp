#include "com/antlersoft/net/Poller.h"
#include <algorithm>
#include <cerrno>
#include "com/antlersoft/MyException.h"
#include "com/antlersoft/Trace.h"

#if defined(COM_ANTLERSOFT_SUBPOLL)
/* Lousy, incomplete implementation of poll(2) for systems that don't have it */

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int poll( pollfd* pollfds, unsigned int nfds, int timeout)
{
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	FD_ZERO( &readfds);
	FD_ZERO( &writefds);
	FD_ZERO( &exceptfds);

	int max_fd= -1;

	for ( int i=0; i<nfds; ++i)
	{
		pollfd& fd=pollfds[i];
		if ( max_fd<fd.fd)
			max_fd=fd.fd;
		fd.revents=0;
		FD_SET( fd.fd, &exceptfds);
		if ( fd.events & POLLIN)
			FD_SET( fd.fd, &readfds);
		if ( fd.events & POLLOUT)
			FD_SET( fd.fd, &writefds);
	}

	timeval timeout_v;
	timeout_v.tv_sec=timeout/1000;
	timeout_v.tv_usec=(timeout%1000)*1000;

	int result=select( max_fd+1, &readfds, &writefds, &exceptfds, timeout<0 ? static_cast<timeval*>(0) : &timeout_v);
	if ( result>0)
	{
		result=0;
		for ( int i=0; i<nfds; ++i)
		{
			pollfd& fd=pollfds[i];
			bool set_fd=false;
			if ( FD_ISSET( fd.fd, &exceptfds))
			{
				fd.revents|=POLLHUP;
				set_fd=true;
			}
			if ( FD_ISSET( fd.fd, &readfds))
			{
				fd.revents|=POLLIN;
				set_fd=true;
			}
			if ( FD_ISSET( fd.fd, &writefds))
			{
				fd.revents|=POLLOUT;
				set_fd=true;
			}
			if ( set_fd)
				++result;
		}
	}
	return result;
}

#endif /* defined(COM_ANTLERSOFT_SUBPOLL) */

using namespace std;
using namespace com::antlersoft::net;

void Polled::cleanup( pollfd& )
{
}

Poller::Poller()
: m_pollfds( new pollfd[INITIAL_SIZE]), m_size(INITIAL_SIZE), m_timeout( -1),
m_finishing( false), m_callback( static_cast<PollCallback*>(0))
{
}

Poller::~Poller()
{
	delete[] m_pollfds;
}

int Poller::pollOnce()
{
	Trace trace( "Poller::pollOnce");
	m_orig_size=m_polled.size();
	if ( m_size<m_orig_size)
	{
		delete[] m_pollfds;
		while ( m_size<m_orig_size)
		{
			m_size*=2;
		}
		m_pollfds=new pollfd[m_size];
	}
	for ( int i=0; i<m_orig_size; ++i)
	{
		m_pollfds[i]=m_polled[i]->m_pollfd;
	}
	int poll_result=poll( m_pollfds, m_orig_size, m_timeout);
	if ( poll_result<0)
	{
		if ( errno!=EINTR)
			throw MY_EXCEPTION_ERRNO;
	}
	if ( poll_result>0)
	{
		for ( int i=0; i<m_orig_size; ++i)
		{
			m_polled[i]->m_pollfd=m_pollfds[i];
		}
		for ( m_index=0; m_index<m_orig_size; m_index++)
		{
			pollfd& poll_struct=m_polled[m_index]->m_pollfd;
			if ( poll_struct.revents)
			{
				m_polled[m_index]->m_polled->polled( *this, poll_struct);
			}
		}
	}
	return poll_result;
}

void Poller::start()
{
	if ( ! m_polled.size())
		return;

	if ( m_finishing)
	{
		throw MY_EXCEPTION( "Starting while finished");
	}

	while ( ! m_finishing)
	{
		if ( pollOnce()==0)
			return;
		if ( m_callback!=static_cast<PollCallback*>( 0))
			m_callback->pollCallback( *this);
	}
	for (m_index=0; m_index<(int)m_polled.size(); m_index++)
	{
		m_polled[m_index]->m_polled->cleanup( m_polled[m_index]->m_pollfd);
	}
}

void Poller::setTimeout( int timeout)
{
	m_timeout=timeout;
}

void Poller::requestFinish()
{
	m_finishing=true;
}

bool Poller::isFinished()
{
	return m_finishing;
}

void Poller::addPolled( Polled& to_add)
{
	PolledAndPollfdPtr ptr(new PolledAndPollfd);
	ptr->m_polled= &to_add;
	m_polled.push_back( ptr);
	ptr->m_polled->setPollfd( ptr->m_pollfd);
}

vector<Poller::PolledAndPollfdPtr>::iterator Poller::findPolled( Polled& to_find, int fd)
{
	vector<PolledAndPollfdPtr>::iterator i;
	for ( i=m_polled.begin();
		i!=m_polled.end();
		++i)
	{
		if ( (*i)->m_polled== &to_find)
		{
			if ( (*i)->m_pollfd.fd==fd)
			{
				break;
			}
		}
	}
	return i;
}

void Poller::setEvents( Polled& to_set, int fd, int events)
{
	vector<PolledAndPollfdPtr>::iterator i=findPolled( to_set, fd);
	if ( i!=m_polled.end() )
	{
		(*i)->m_pollfd.events=events;
	}
}

void Poller::removePolled( Polled& to_remove, int fd)
{
	vector<PolledAndPollfdPtr>::iterator i=findPolled( to_remove, fd);
	if ( i!=m_polled.end() )
	{
		int index=i-m_polled.begin();
		if ( index<m_orig_size)
			m_orig_size--;
		if ( index<=m_index)
			m_index--;
		m_polled.erase( i);
	}
}

