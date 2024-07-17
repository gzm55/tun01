#ifndef CMAKE_CONFIG_H
#define CMAKE_CONFIG_H

/* Version number of package */
#define VERSION "${PROJECT_VERSION}"
#define PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}"

/* Define to 1 if you have the <pty.h> header file. */
#cmakedefine HAVE_PTY_H 1

/* Define to 1 if you have the <util.h> header file. */
#cmakedefine HAVE_UTIL_H 1

/* Define to 1 if you have the <libutil.h> header file. */
#cmakedefine HAVE_LIBUTIL_H 1

/* Define to 1 if you have the 'cfmakeraw' function. */
#cmakedefine HAVE_CFMAKERAW 1

/* Define to 1 if you have the 'forkpty' function. */
#cmakedefine HAVE_FORKPTY 1

/* Define to 1 if you have the 'poll' function. */
#cmakedefine HAVE_POLL 1

#cmakedefine HAVE_SIZE_T
#ifndef HAVE_SIZE_T
typedef unsigned int size_t
#endif

#cmakedefine HAVE_PID_T
#ifndef HAVE_PID_T
typedef int pid_t
#endif

#cmakedefine HAVE_SOCKLEN_T
#ifndef HAVE_SOCKLEN_T
typedef int socklen_t
#endif

#endif
