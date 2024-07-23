set(CMAKE_OSX_ARCHITECTURES x86_64;arm64) # build universal binary on macos

cmake_minimum_required(VERSION 3.13)
# CMAKE_CXX_STANDARD requires 3.1
# HOMEPAGE_URL requires 3.12
# target_link_options requires 3.13

project(StdioTunnel
	VERSION 0.3.0
	LANGUAGES CXX C
	HOMEPAGE_URL https://github.com/gzm55/tun01
	DESCRIPTION "StdioTunnel allows you to tunnel arbitrary TCP connections through any connection that approximates a tty with a clear 8-bit data path.")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_C_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
  src/com/antlersoft/MyException.cpp
  src/com/antlersoft/Trace.cpp
  src/com/antlersoft/net/Poller.cpp
  src/com/antlersoft/net/ReadStream.cpp
  src/com/antlersoft/net/SockBuffer.cpp
  src/com/antlersoft/net/WriteStream.cpp
  src/com/antlersoft/stdio_tunnel/StdioTunnel.cpp
  src/microsocks/sblist.c
  src/microsocks/sblist_delete.c
  src/microsocks/server.c
)

INCLUDE(CMakePushCheckState)
cmake_push_check_state(RESET)

# Checks for header files.
INCLUDE(CheckIncludeFile)
CHECK_INCLUDE_FILE(pty.h HAVE_PTY_H)
CHECK_INCLUDE_FILE(util.h HAVE_UTIL_H)
CHECK_INCLUDE_FILE(libutil.h HAVE_LIBUTIL_H)
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE(pthread.h HAVE_PTHREAD_H)
CHECK_INCLUDE_FILE(arpa/inet.h HAVE_ARPA_INET_H)

if(NOT HAVE_PTHREAD_H)
  message(FATAL_ERROR "sys/socket.h header is required")
endif()
if(NOT HAVE_PTHREAD_H)
  message(FATAL_ERROR "pthread.h header is required")
endif()
if(NOT HAVE_ARPA_INET_H)
  message(FATAL_ERROR "arpa/inet.h header is required")
endif()

# Checks for typedefs, structures, and compiler characteristics.
include(CheckTypeSize)
check_type_size(size_t SIZE_T)
check_type_size(pid_t PID_T)
if(HAVE_UNISTD_H)
  list(APPEND CMAKE_EXTRA_INCLUDE_FILES unistd.h)
endif()
list(APPEND CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
check_type_size("socklen_t" SOCKLEN_T)
cmake_reset_check_state()

# Checks for libraries.
# FIXME
# The AC_SEARCH_LIBS macro from autotools doesn't
# appear to have an equivalent in CMake.
find_library(HAVE_UTIL_LIB util)
find_library(HAVE_SOCKET_LIB socket)
find_library(HAVE_NSL_LIB nsl)
find_library(HAVE_PTHREAD_LIB pthread)
if(HAVE_UTIL_LIB)
  list(APPEND libs util)
endif()
if(HAVE_SOCKET_LIB)
  list(APPEND libs socket)
endif()
if(HAVE_NSL_LIB)
  list(APPEND libs nsl)
endif()
if(HAVE_PTHREAD_LIB)
  list(APPEND libs pthread)
endif()
target_link_libraries(${PROJECT_NAME} ${libs})

# Checks for library functions.
include(CheckSymbolExists)
check_symbol_exists(listen sys/socket.h HAVE_LISTEN)
check_symbol_exists(socket sys/socket.h HAVE_SOCKET)
check_symbol_exists(gethostbyname netdb.h HAVE_GETHOSTBYNAME)
check_symbol_exists(memmove memory.h HAVE_MEMMOVE)
check_symbol_exists(strerror string.h HAVE_STRERROR)

check_symbol_exists(poll poll.h HAVE_POLL)
check_symbol_exists(cfmakeraw termios.h HAVE_CFMAKERAW)
if(HAVE_PTY_H)
  list(APPEND CMAKE_REQUIRED_INCLUDES pty.h)
endif()
if(HAVE_UTIL_H)
  list(APPEND CMAKE_REQUIRED_INCLUDES util.h)
endif()
set(CMAKE_REQUIRED_LIBRARIES ${libs})
check_symbol_exists(forkpty ${CMAKE_REQUIRED_INCLUDES} HAVE_FORKPTY)
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
target_include_directories(${PROJECT_NAME} PRIVATE src ${CMAKE_BINARY_DIR}/include)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(${PROJECT_NAME} PRIVATE -Wl,--as-needed -static-libgcc -static-libstdc++)
endif()

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND $<$<CONFIG:Release>:${CMAKE_STRIP}> ARGS $<TARGET_FILE:${PROJECT_NAME}>)
