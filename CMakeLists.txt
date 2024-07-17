cmake_minimum_required(VERSION 3.12)
# CMAKE_CXX_STANDARD requires 3.1
# HOMEPAGE_URL requires 3.12
# target_link_options requires 3.13

project(StdioTunnel
	VERSION 0.3.0
	LANGUAGES CXX
	HOMEPAGE_URL https://github.com/gzm55/tun01
	DESCRIPTION "StdioTunnel allows you to tunnel arbitrary TCP connections through any connection that approximates a tty with a clear 8-bit data path.")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(SOURCES
src/com/antlersoft/MyException.cpp
src/com/antlersoft/MyException.h
src/com/antlersoft/RefObject.cpp
src/com/antlersoft/RefObject.h
src/com/antlersoft/RefPtr.h
src/com/antlersoft/StderrEndl.h
src/com/antlersoft/Trace.cpp
src/com/antlersoft/Trace.h
src/com/antlersoft/net/PollConfig.h
src/com/antlersoft/net/Poller.cpp
src/com/antlersoft/net/Poller.h
src/com/antlersoft/net/ReadStream.cpp
src/com/antlersoft/net/ReadStream.h
src/com/antlersoft/net/SockBuffer.cpp
src/com/antlersoft/net/SockBuffer.h
src/com/antlersoft/net/WriteStream.cpp
src/com/antlersoft/net/WriteStream.h
src/com/antlersoft/stdio_tunnel/StdioTunnel.cpp
src/com/antlersoft/stdio_tunnel/StdioTunnel.h
src/com/antlersoft/tokenize.h
)
add_executable(${PROJECT_NAME} ${SOURCES})

INCLUDE(CMakePushCheckState)
cmake_push_check_state(RESET)

# Checks for header files.
INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(pty.h HAVE_PTY_H)
CHECK_INCLUDE_FILE_CXX(util.h HAVE_UTIL_H)
CHECK_INCLUDE_FILE_CXX(libutil.h HAVE_LIBUTIL_H)
CHECK_INCLUDE_FILE_CXX(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CXX(sys/socket.h HAVE_SYS_SOCKET_H)

if(NOT HAVE_SYS_SOCKET_H)
  message(FATAL_ERROR "sys/socket.h header is required")
endif()

# Checks for typedefs, structures, and compiler characteristics.
include(CheckTypeSize)
check_type_size(size_t SIZE_T LANGUAGE CXX)
check_type_size(pid_t PID_T LANGUAGE CXX)
if(HAVE_UNISTD_H)
  list(APPEND CMAKE_EXTRA_INCLUDE_FILES unistd.h)
endif()
list(APPEND CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
check_type_size("socklen_t" SOCKLEN_T LANGUAGE CXX)
cmake_reset_check_state()

# Checks for libraries.
# FIXME
# The AC_SEARCH_LIBS macro from autotools doesn't
# appear to have an equivalent in CMake.
find_library(HAVE_UTIL_LIB util)
find_library(HAVE_SOCKET_LIB socket)
find_library(HAVE_NSL_LIB nsl)
if(HAVE_UTIL_LIB)
  list(APPEND libs util)
endif()
if(HAVE_SOCKET_LIB)
  list(APPEND libs socket)
endif()
if(HAVE_NSL_LIB)
  list(APPEND libs nsl)
endif()
target_link_libraries(StdioTunnel ${libs})

# Checks for library functions.
include(CheckCXXSymbolExists)
check_cxx_symbol_exists(listen sys/socket.h HAVE_LISTEN)
check_cxx_symbol_exists(socket sys/socket.h HAVE_SOCKET)
check_cxx_symbol_exists(gethostbyname netdb.h HAVE_GETHOSTBYNAME)
check_cxx_symbol_exists(memmove memory.h HAVE_MEMMOVE)
check_cxx_symbol_exists(strerror string.h HAVE_STRERROR)

check_cxx_symbol_exists(poll poll.h HAVE_POLL)
check_cxx_symbol_exists(cfmakeraw termios.h HAVE_CFMAKERAW)
if(HAVE_PTY_H)
  list(APPEND CMAKE_REQUIRED_INCLUDES pty.h)
endif()
if(HAVE_UTIL_H)
  list(APPEND CMAKE_REQUIRED_INCLUDES util.h)
endif()
check_cxx_symbol_exists(forkpty ${CMAKE_REQUIRED_INCLUDES} HAVE_FORKPTY)
cmake_reset_check_state()

if(NOT HAVE_LISTEN)
  message(FATAL_ERROR "listen function is required")
endif()
if(NOT HAVE_GETHOSTBYNAME)
  message(FATAL_ERROR "gethostbyname function is required")
endif()
if(NOT HAVE_MEMMOVE)
  message(FATAL_ERROR "memmove function is required")
endif()
if(NOT HAVE_SOCKET)
  message(FATAL_ERROR "socket function is required")
endif()
if(NOT HAVE_STRERROR)
  message(HAVE_STRERROR "strerror function is required")
endif()

cmake_pop_check_state()

CONFIGURE_FILE(config.h.cmake ${CMAKE_BINARY_DIR}/include/config.h)
target_include_directories(StdioTunnel PRIVATE src ${CMAKE_BINARY_DIR}/include)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(StdioTunnel PRIVATE -Wl,--as-needed)
endif()

ADD_DEFINITIONS(-DHAVE_CONFIG_H)
