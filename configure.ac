#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(StdioTunnel, 0.2, [mars@antlersoft.com])
AM_INIT_AUTOMAKE( StdioTunnel, 0.2)
AC_LANG(C++)
AC_CONFIG_SRCDIR([src/com/antlersoft/MyException.cpp])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
#AC_CHECK_LIB([util], [cfmakeraw])
AC_SEARCH_LIBS([forkpty],[util],[AC_DEFINE(HAVE_FORKPTY)])
AC_SEARCH_LIBS([listen],[socket],[AC_DEFINE(HAVE_GETHOSTBYNAME)])
AC_SEARCH_LIBS([gethostbyname],[nsl],[AC_DEFINE(HAVE_GETHOSTBYNAME)])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h netdb.h netinet/in.h stdlib.h sys/socket.h termios.h unistd.h util.h pty.h libutil.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPE( socklen_t,,[AC_DEFINE([socklen_t],[int],[likely type for missing typedef])])

# Checks for library functions.
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([listen gethostbyname memmove socket strerror cfmakeraw forkpty poll])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
