#ifndef COM_ANTLERSOFT_NET_POLLER_H
#define COM_ANTLERSOFT_NET_POLLER_H

#include <vector>

#include "com/antlersoft/net/PollConfig.h"

#include "com/antlersoft/RefPtr.h"

namespace com { namespace antlersoft { namespace net {

class Poller;

class Polled
{
public :
	virtual void polled( Poller& poller, pollfd& poll_struct)=0;
	virtual void setPollfd( pollfd& poll_struct)=0;
	virtual void cleanup( pollfd& poll_struct);
};

class PollCallback
{
public :
	virtual void pollCallback( Poller& poller)=0;
};

class Poller
{
private :
	struct PolledAndPollfd : public RefObject
	{
		Polled* m_polled;
		pollfd m_pollfd;
	};
	typedef RefPtr<PolledAndPollfd> PolledAndPollfdPtr;
	const static int INITIAL_SIZE=10;
	std::vector<PolledAndPollfdPtr> m_polled;
	pollfd* m_pollfds;
	/** Size of m_pollfds array */
	int m_size;
	/** Size before traversing polled objects */
	int m_orig_size;
	/** Current index while traversing polled objects */
	int m_index;

	int m_timeout;
	bool m_finishing;
	PollCallback* m_callback;
	/** Not implemented */
	Poller( const Poller&);
	/** Not implemented */
	Poller& operator=( const Poller&);
	std::vector<PolledAndPollfdPtr>::iterator findPolled( Polled& to_find, int fd);

public :
	Poller();
	~Poller();
	void start();
	int pollOnce();
	void setTimeout( int timeout);
	void requestFinish();
	bool isFinished();
	void addPolled( Polled& to_add);
	void setEvents( Polled& to_set, int fd, int events);
	void removePolled( Polled& to_remove, int fd);
	PollCallback* getCallback()
	{ return m_callback; }
	void setCallback( PollCallback* callback)
	{ m_callback=callback; }
};

} } } 
#endif
