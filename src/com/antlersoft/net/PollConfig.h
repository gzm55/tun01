#ifndef COM_ANTLERSOFT_NET_POLLCONFIG_H
#define COM_ANTLERSOFT_NET_POLLCONFIG_H

#if defined(HAVE_CONFIG_H)
#include "config.h"
#else
/* undef if poll system call is not available */
#define HAVE_POLL 1
#endif

#ifndef HAVE_POLL

#define COM_ANTLERSOFT_SUBPOLL 1
/*
 * If the platform provides no implementation of poll(2), we provide our own
 * (incomplete) implementation based on select
 */
struct pollfd
{
int fd;
short events;
short revents;
};

#define POLLIN		1
#define POLLOUT		4
#define POLLERR		8
#define POLLHUP		16
#define POLLNVAL	32

extern "C" {
int poll( pollfd* pollfds, unsigned int nfds, int timeout);
}

#else

#include <sys/poll.h>

#endif

#endif /* Guard */
