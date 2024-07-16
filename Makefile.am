C_A = src/com/antlersoft
C_A_src = $(C_A)/RefObject.cpp $(C_A)/Trace.cpp $(C_A)/MyException.cpp
C_A_hdr = $(C_A)/RefObject.h $(C_A)/Trace.h $(C_A)/MyException.h $(C_A)/RefPtr.h $(C_A)/tokenize.h
C_A_N = $(C_A)/net
C_A_N_src = $(C_A_N)/SockBuffer.cpp $(C_A_N)/Poller.cpp $(C_A_N)/ReadStream.cpp $(C_A_N)/WriteStream.cpp
C_A_N_hdr = $(C_A_N)/SockBuffer.h $(C_A_N)/Poller.h $(C_A_N)/ReadStream.h $(C_A_N)/WriteStream.h $(C_A_N)/PollConfig.h
C_A_S = $(C_A)/stdio_tunnel

bin_PROGRAMS = StdioTunnel
StdioTunnel_SOURCES = $(C_A_S)/StdioTunnel.cpp $(C_A_src) $(C_A_N_src)

man_MANS = doc/StdioTunnel.1
EXTRA_DIST = $(man_MANS) doc/stdio_tunnel_man.xml

noinst_HEADERS = $(C_A_S)/StdioTunnel.h $(C_A_hdr) $(C_A_N_hdr)

AM_CPPFLAGS = -I ./src
