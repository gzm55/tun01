#ifndef COM_ANTLERSOFT_STDERRENDL_H
#define COM_ANTLERSOFT_STDERRENDL_H

#include <termios.h>

namespace com { namespace antlersoft {

static inline const char* cerr_endl()
{
	static int stderr_fd = 2;
	static const char *const endl_tty_raw = "\r\n";
	static const char *const endl_normal = "\n";
	struct termios termios_now;
	if (0 != tcgetattr(stderr_fd, &termios_now)) {
		return endl_normal; // not a tty
	}
	return (termios_now.c_oflag & OPOST) ? endl_normal : endl_tty_raw;
}

} }

#endif
