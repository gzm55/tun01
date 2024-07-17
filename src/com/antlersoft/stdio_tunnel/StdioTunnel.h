#ifndef STDIO_TUNNEL_H
#define STDIO_TUNNEL_H
/*
 * Copyright (c) 2004,2005  Michael A. MacDonald
 * ----- - - -- - - --
 *     This package is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 * 
 *     This package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with the package (see COPYING); if not, see www.gnu.org
 * ----- - - -- - - --
 */

#include <vector>
#include <string>
#include <map>
#include <termios.h>
#include "com/antlersoft/RefPtr.h"
#include "com/antlersoft/net/PollConfig.h"
#include "com/antlersoft/net/Poller.h"
#include "com/antlersoft/net/SockBuffer.h"
#include "com/antlersoft/net/ReadStream.h"
#include "com/antlersoft/net/WriteStream.h"

class CustomWriteStream : public com::antlersoft::net::WriteStream
{
public :
	CustomWriteStream()
	: m_poller(0), m_polled(0)
	{}
	com::antlersoft::net::Poller* m_poller;
	com::antlersoft::net::Polled* m_polled;
	int m_fd;
	void send();
};

/**
 * Information about a request for a connection that is to be passed
 * through the tunnel.  The actual connection is a different object.
 */
class ConnectionSpec
{
public :
	/**
	 * LISTEN = Open listen port on the remote side
	 * CONNECT = Connect to port on remote side (listen on local side)
	 */
	enum Direction
	{
		LISTEN,
		CONNECT
	} m_connection_type;
	enum
	{
		ACK_PACKETS = 1,
		PROMISCUOUS = 2
	};
	/** The host to connect to */
	std::string m_connect_host;
	/** The port to connect to */
	int m_connect_port;
	/** The port to listen to */
	int m_listen_port;
	/** Connection flags (see anon enumeration above )*/
	int m_flags;

	void serialize( com::antlersoft::net::ReadStream& stream);
	void serialize( CustomWriteStream& stream);
};

class StdioTunnel;
class TunnelConnection;

class ConnectionLink : public RefObject, public com::antlersoft::net::Polled
{
	TunnelConnection& m_connection;
	int m_socket;
	short m_id;
	com::antlersoft::net::SockBuffer m_buffer;
	bool m_closing;
	bool m_acknowledged;
public :
	ConnectionLink( TunnelConnection& connection, short id, int socket);
	void polled( com::antlersoft::net::Poller& poller, pollfd& poll_struct);
	void setPollfd( pollfd& poll_struct);
	void cleanup( pollfd& poll_struct);
	com::antlersoft::net::Polled& getPolled()
	{ return *this; }
	com::antlersoft::net::SockBuffer& getBuffer()
	{ return m_buffer; }
	int getSocket()
	{ return m_socket; }
	short getID()
	{ return m_id; }
	bool isClosing()
	{ return m_closing; }
	void setClosing( com::antlersoft::net::Poller& poller);
	bool isAcknowledged()
	{ return m_acknowledged; }
	void setAcknowledged( bool ack, com::antlersoft::net::Poller& poller);
};

typedef RefPtr<ConnectionLink> ConnectionLinkPtr;

class TunnelConnection : public RefObject
{
private :
	ConnectionSpec& m_spec;
	int m_id;
	void receiveData( StdioTunnel& tunnel, ConnectionLinkPtr link);
protected :
	CustomWriteStream& m_writer;
	TunnelConnection( ConnectionSpec& spec, int id, CustomWriteStream& writer);
	std::map<short,ConnectionLinkPtr> m_link_map;
	void processLinkMessage( StdioTunnel& tunnel, int command, ConnectionLinkPtr link);
public :
	static const int BUFFER_SIZE=4096;
	/** Command codes for connections */
	enum
	{
		LINK_CREATE=387,
		LINK_SEND=388,
		LINK_CLOSE=389,
		LINK_SUCCESS=390,
		LINK_FAILURE=391,
		LINK_SEND_FOR_ACK=392,
		LINK_ACK=393
	};
	static RefPtr<TunnelConnection> CreateConnection( StdioTunnel& tunnel, ConnectionSpec& spec, bool is_local, int id);
	ConnectionSpec& getSpec()
	{
		return m_spec;
	}
	int getID()
	{
		return m_id;
	}
	virtual void processMessage( StdioTunnel& tunnel)=0;
	void closeLink( com::antlersoft::net::Poller& poller, ConnectionLinkPtr to_close);
	void sendData( ConnectionLinkPtr source, char* bytes, int length);
};

typedef RefPtr<TunnelConnection> TunnelConnectionPtr;

/**
 * This class holds the common portions of StdioTunnelLocal
 * and StdioTunnelRemote
 */
class StdioTunnel : public RefObject, public com::antlersoft::net::PollCallback
{
private :
	/** Not implemented */
	StdioTunnel( const StdioTunnel& to_copy);
	/** Not implemented */
	StdioTunnel& operator=( const StdioTunnel& to_copy);
	com::antlersoft::net::ReadStream m_read_stream;
	CustomWriteStream m_write_stream;
	bool m_closed_by_request;
protected :
	struct termios m_termios;
	bool m_is_terminal;
	/** List of objects that describe connections requested on the command line */
	com::antlersoft::net::Poller m_poller;

	/** This function is overridden by sub-classes to perform
	 * additional configuration steps */
	virtual void configure( int argc, char* argv[]);

	StdioTunnel();
	virtual ~StdioTunnel();

	void setShutdownByRequest()
	{ m_closed_by_request=true; }

	/** String transmitted from remote to local to kick off handshake */
	std::string m_magic_string;

	/** Map of connection ids to connections */
	std::map<int, TunnelConnectionPtr> m_connection_map;
	static const int STDIN_FD=0;
	static const int STDOUT_FD=1;

public :
	enum Commands {
		CMD_ERROR=401,
		CMD_CREATE=402,
		CMD_CONNECTION=403,
		CMD_SHUTDOWN=404
	};
	/** Create one or another side of a tunnel */
	static RefPtr<StdioTunnel> CreateTunnel( int argc, char* argv[]);
	/** This function starts the tunnel operating; it does not return */
	virtual void start()=0;
	/** This function sends an error message to the user */
	virtual void reportError( std::string message)=0;
	com::antlersoft::net::Poller& getPoller( )
	{
		return m_poller;
	}
	com::antlersoft::net::ReadStream& getReader() { return m_read_stream; }
	CustomWriteStream& getWriter() { return m_write_stream; }
	/** Calls start and attempts to clean up on finish */
	void startFinish();
	/** Check for interrupt for clean exit */
	void pollCallback( com::antlersoft::net::Poller& poller);
};

class StdioTunnelLocal;

class MagicStringDetector : public com::antlersoft::net::SockBuffer
{
private :
	int m_recognized_count;
	std::string m_magic_string;
	StdioTunnelLocal& m_tunnel;

public :
	MagicStringDetector( StdioTunnelLocal& tunnel, std::string magic_string);
	void processReadData( char* buffer, int& count, int max_length);
};

class StdioTunnelLocal : public StdioTunnel, com::antlersoft::net::Polled
{
private :
	enum
	{
		INITIAL,
		SET_STDIO_READ,
		SET_STDIO_WRITE,
		SET_PIPE_READ,
		PASS_THROUGH,
		HAND_SHAKE,
		CONNECTED,
		SHUT_DOWN
	} m_state;
	std::string m_command;
	int m_read_fd;
	int m_write_fd;
	bool m_force_pipe;
	bool m_expects_pipe;
	std::vector<ConnectionSpec> m_connection_specs;
	MagicStringDetector m_read_buffer;
	com::antlersoft::net::SockBuffer m_write_buffer;
	friend class MagicStringDetector;
	void startHandshaking();
protected :
	void configure( int argc, char* argv[]);
public :
	StdioTunnelLocal()
	: m_state( INITIAL), m_read_fd( -1), m_write_fd( -1), m_force_pipe( false), m_expects_pipe( false), m_read_buffer( *this, m_magic_string)
	{}
	void start();
	void reportError( std::string message);
	void polled( com::antlersoft::net::Poller& poller, pollfd& poll_struct);
	void setPollfd( pollfd& poll_struct);
};

class StdioTunnelRemote : public StdioTunnel, public com::antlersoft::net::Polled
{
	enum {
		INITIAL,
		SET_STDIO_READ,
		HANDSHAKING,
		CONNECTED,
		SHUT_DOWN
	} m_state;

public :
	StdioTunnelRemote()
	: m_state( INITIAL)
	{}
	void start();
	void reportError( std::string message);
	void polled( com::antlersoft::net::Poller& poller, pollfd& poll_struct);
	void setPollfd( pollfd& poll_struct);
};
#endif
