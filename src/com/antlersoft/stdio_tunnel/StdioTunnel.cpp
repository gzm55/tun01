#ifdef HAVE_CONFIG_H
#  include <config.h>
#else
#  define HAVE_CFMAKERAW 1
#  define HAVE_PTY_H 1
#  define HAVE_FORKPTY 1
#  ifdef __CYGWIN__
#    undef HAVE_CFMAKERAW
#  endif
#endif
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

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#ifdef HAVE_PTY_H
#  include <pty.h>
#endif
#ifdef HAVE_UTIL_H
#  include <util.h>
#endif
#ifdef HAVE_LIBUTIL_H
#  include <libutil.h>
#endif
#include "StdioTunnel.h"
#include "com/antlersoft/MyException.h"
#include "com/antlersoft/StderrEndl.h"
#include "com/antlersoft/Trace.h"

#ifndef MAX
#  define MAX(x, y) ((x) > (y) ? (x) : (y))
#  define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

using namespace std;
using namespace com::antlersoft;
using namespace com::antlersoft::net;

#ifndef HAVE_CFMAKERAW
int cfmakeraw(termios* termios_p) {
  termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  termios_p->c_oflag &= ~OPOST;
  termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  termios_p->c_cflag &= ~(CSIZE | PARENB);
  termios_p->c_cflag |= CS8;

  return 0;
}
#endif

void ConnectionSpec::deserialize(ReadStream& stream) {
  Trace trace("ConnectionSpec::deserialize");
  m_connection_type = (Direction)stream.readShort();
  m_connect_host = stream.readString();
  m_connect_port = stream.readShort();
  m_listen_port = stream.readShort();
  m_flags = stream.readShort();
}

void ConnectionSpec::serialize(WriteStream& stream) {
  Trace trace("ConnectionSpec::serialize");
  stream.writeShort(m_connection_type);
  stream.writeString(m_connect_host);
  stream.writeShort(m_connect_port);
  stream.writeShort(m_listen_port);
  stream.writeShort(m_flags);
}

void CustomWriteStream::send() {
  Trace trace("CustomWriteStream::send");
  if (!m_poller) {
    throw MY_EXCEPTION("send uninitialized write stream");
  }
  WriteStream::send();
  m_poller->setEvents(*m_polled, m_fd, POLLOUT);
}

ConnectionLink::ConnectionLink(TunnelConnection& connection, short id, int socket)
    : m_connection(connection), m_socket(socket), m_id(id), m_closing(false), m_acknowledged(true) {}

void ConnectionLink::polled(Poller& poller, pollfd& poll_struct) {
  Trace trace("ConnectionLink::polled");
  if (poll_struct.revents & POLLIN) {
    char buffer[TunnelConnection::BUFFER_SIZE];
    int count = read(m_socket, buffer, TunnelConnection::BUFFER_SIZE);
    if (count <= 0) {
      m_connection.closeLink(poller, this);
      return;
    }
    m_connection.sendData(this, buffer, count);
    if (m_connection.getSpec()->m_flags & ConnectionSpec::ACK_PACKETS) {
      Trace t("Setting acknowledged to false");
      setAcknowledged(false, poller);
    }
  }
  if (poll_struct.revents & POLLOUT) {
    m_buffer.writeData(m_socket);
    if (!m_buffer.unsentData()) poller.setEvents(*this, m_socket, m_acknowledged ? POLLIN : 0);
  }
  if (poll_struct.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    Trace t("Error/close on link");
    m_connection.closeLink(poller, this);
  }
}

void ConnectionLink::setPollfd(pollfd& poll_struct) {
  Trace trace("ConnectionLink::setPollfd");
  poll_struct.fd = m_socket;
  poll_struct.events = POLLIN;
}

void ConnectionLink::cleanup(pollfd&) {
  Trace trace("ConnectionLink::cleanup");
  close(m_socket);
}

void ConnectionLink::setAcknowledged(bool ack, Poller& poller) {
  Trace trace("ConnectionLink::setAcknowledged");
  m_acknowledged = ack;
  if (!m_closing) {
    int events = (m_acknowledged ? POLLIN : 0) | (m_buffer.unsentData() ? POLLOUT : 0);
    poller.setEvents(*this, m_socket, events);
  }
}

void ConnectionLink::setClosing(Poller& poller) {
  Trace trace("ConnectionLink::setClosing");
  m_closing = true;
  close(m_socket);
  poller.removePolled(*this, m_socket);
}

StdioTunnel::StdioTunnel() : m_closed_by_request(false) {
  Trace trace("StdioTunnel::StdioTunnel");
  m_magic_string = string("STDIO_TUNNEL MAGIC") + " HANDSHAKE --";
  m_poller.setCallback(this);
  if (tcgetattr(STDIN_FD, &m_termios) == 0) {
    m_is_terminal = true;
    // Make terminal raw
    termios new_termios = m_termios;
    cfmakeraw(&new_termios);
    tcsetattr(STDIN_FD, TCSANOW, &new_termios);
  }
}

StdioTunnel::~StdioTunnel() {
  Trace trace("StdioTunnel::~StdioTunnel");
  if (m_is_terminal) {
    tcsetattr(STDIN_FD, TCSANOW, &m_termios);
  }
  fcntl(STDIN_FD, F_SETFL, 0);
  fcntl(STDOUT_FD, F_SETFL, 0);
}

void StdioTunnel::startFinish() {
  start();
  if (m_closed_by_request) {
    m_write_stream.writeShort(CMD_SHUTDOWN);
    m_write_stream.send();
    getPoller().pollOnce();
  }
}

namespace {
auto parseConnection(ConnectionSpec::Direction direction, char* arg) {
  Trace trace("parseConnection");

  // split by ':'
  vector<string> connect_params;
  istringstream input(arg);
  for (string t; getline(input, t, ':'); connect_params.push_back(t));
  if (connect_params.size() != 3 && connect_params.size() != 4) {
    throw MY_EXCEPTION(static_cast<stringstream&&>(stringstream() << "Bad connection argument: " << arg));
  }

  int listen_port = atoi(connect_params[0].c_str());
  if (listen_port <= 0) {
    throw MY_EXCEPTION(static_cast<stringstream&&>(stringstream() << "Bad listen port: " << connect_params[0].c_str()));
  }

  int connect_port = atoi(connect_params[2].c_str());
  if (connect_port < 0) {
    throw MY_EXCEPTION(
        static_cast<stringstream&&>(stringstream() << "Bad connect port: " << connect_params[2].c_str()));
  }

  if ((connect_params[1].length() == 0 && connect_port != 0) && (connect_params[1].length() > 0 && connect_port == 0)) {
    throw MY_EXCEPTION(
        static_cast<stringstream&&>(stringstream() << "Bad connect host or port: " << connect_params[2].c_str()));
  }

  auto result = make_shared<ConnectionSpec>();
  result->m_connection_type = direction;
  result->m_connect_host = connect_params[1];
  result->m_connect_port = connect_port;
  result->m_listen_port = listen_port;
  result->m_flags = 0;

  if (result->isSocks5() && direction != ConnectionSpec::CONNECT) {
    throw MY_EXCEPTION("socks 5 can only on the remote side");
  }

  if (connect_params.size() == 4) {
    if (connect_params[3].find('a') != string::npos) result->m_flags |= ConnectionSpec::ACK_PACKETS;
    if (connect_params[3].find('p') != string::npos) result->m_flags |= ConnectionSpec::PROMISCUOUS;
  }
  return result;
}
}  // namespace

shared_ptr<StdioTunnel> StdioTunnel::CreateTunnel(const int argc, char* argv[]) {
  Trace trace("StdioTunnel::CreateTunnel");
  bool local = true;
  bool version = false;

  for (int i = 1; i < argc; ++i) {
    char* arg = argv[i];
    if (arg[0] == '-' && arg[1] != '\0') {
      switch (arg[1]) {
        case 'l':
          local = true;
          break;
        case 'r':
          local = false;
          break;
        case 'D':
          Trace::setPrint(true);
          break;
        case 'V':
          cout << PACKAGE_STRING << cerr_endl() << flush;
          version = true;
          break;
        case 'L':
        case 'R':
        case 'p':
        case 'c':
          // skip connection spec, login pattern and remote cmd
          i += (arg[2] == 0);
          break;
        case '-':
          i = argc - 1;  // skip argv after "--"
          break;
        case 'T':
        case 't':
        case 'd':
          break;  // process by configure(...)
        default:
          throw MY_EXCEPTION("Unknown option");
      }
    } else {
      break;  // stop at connect command
    }
  }
  // Get out with no error if all they want is the version
  if (version && argc == 2) {
    throw "";
  }
  shared_ptr<StdioTunnel> tunnel;
  if (local) {
    tunnel = make_shared<StdioTunnelLocal>();
  } else {
    tunnel = make_shared<StdioTunnelRemote>();
  }
  tunnel->configure(argc, argv);
  return tunnel;
}

void StdioTunnel::configure(int, char*[]) { Trace trace("StdioTunnel::configure"); }

MagicStringDetector::MagicStringDetector(StdioTunnelLocal& tunnel, string magic_string)
    : m_recognized_count(0),
      m_magic_string(magic_string),
      m_tunnel(tunnel),
      m_recognized_login_count(0),
      m_login_pattern(""),
      m_remote_cmd("") {}

void MagicStringDetector::processReadData(char* buffer, int& count, int) {
  Trace trace("MagicStringDetector::processReadData");
  for (int i = 0; i < count; ++i) {
    if (buffer[i] == m_magic_string[m_recognized_count]) {
      if (++m_recognized_count >= (int)m_magic_string.length()) {
        m_tunnel.startHandshaking();
        return;
      }
    } else {
      m_recognized_count = 0;
    }
    if (0 < m_remote_cmd.length() && m_recognized_login_count < (int)m_login_pattern.length() &&
        buffer[i] == m_login_pattern[m_recognized_login_count]) {
      if (++m_recognized_login_count >= (int)m_login_pattern.length()) {
        m_tunnel.remoteInit(m_remote_cmd);  // run remote cmd only once
      }
    } else if (m_recognized_login_count < (int)m_login_pattern.length()) {
      m_recognized_login_count = 0;
    }
  }
}

static void daemon() {
  /* Fork off the parent process */
  const auto pid = fork();

  /* An error occurred */
  if (pid < 0) throw MY_EXCEPTION_ERRNO;

  /* Success: Let the parent terminate */
  if (pid > 0) throw MY_EXCEPTION("exit for daemon");

  /* Catch, ignore and handle signals */
  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* Set new file permissions */
  umask(0);

  /* Change the working directory to the root directory */
  /* or another appropriated directory */
  if (chdir("/")) throw MY_EXCEPTION_ERRNO;
}

void StdioTunnelLocal::startHandshaking() {
  Trace trace("StdioTunnelLocal::startHandshaking");
  m_state = HAND_SHAKE;
  m_write_buffer.clear();
  getWriter().writeShort(CMD_CREATE);
  getWriter().writeShort(m_connection_map.size());
  for (map<int, TunnelConnectionPtr>::iterator it = m_connection_map.begin(); it != m_connection_map.end(); ++it) {
    getWriter().writeShort(it->second->getID());
    it->second->getSpec()->serialize(getWriter());
  }
  getWriter().send();
  getPoller().removePolled(*this, STDIN_FD);
  if (m_is_terminal) {
    termios enable_sig;
    enable_sig = m_termios;
    cfmakeraw(&enable_sig);
    enable_sig.c_lflag |= ISIG;
    tcsetattr(STDIN_FD, TCSAFLUSH, &enable_sig);
  }
  if (m_daemon) {
    cerr << "Keep running daemon in background ..." << cerr_endl() << flush;
    daemon();
  }
}

void StdioTunnelLocal::remoteInit(const string& remote_cmd) {
  Trace trace(string("StdioTunnelLocal::remoteInit: ") + remote_cmd);
  const string send_cmd = remote_cmd + "\n";
  if (write(getWriter().m_fd, send_cmd.c_str(), send_cmd.length()) != (ssize_t)send_cmd.length())
    throw MY_EXCEPTION_ERRNO;
}

void StdioTunnelLocal::configure(const int argc, char* argv[]) {
  Trace trace("StdioTunnelLocal::configure");
  int socks5_spec_count = 0;

  for (int i = 1; i < argc && nullptr == m_command; ++i) {
    char* arg = argv[i];
    if (arg[0] == '-' && arg[1] != 0) {
      bool long_arg = (arg[2] != 0 || i + 1 == argc);
      switch (arg[1]) {
        case 'R':
          m_connection_specs.push_back(parseConnection(ConnectionSpec::LISTEN, long_arg ? arg + 2 : argv[i + 1]));
          i += !long_arg;
          break;
        case 'L':
          m_connection_specs.push_back(parseConnection(ConnectionSpec::CONNECT, long_arg ? arg + 2 : argv[i + 1]));
          socks5_spec_count += m_connection_specs.back()->isSocks5();
          i += !long_arg;
          break;
        case 'T':
          m_force_pipe = true;
          break;
        case 't':
          m_expects_pipe = true;
          break;
        case 'd':
          m_daemon = true;
          break;
        case 'p':
          m_read_buffer.setLoginPattern(long_arg ? arg + 2 : argv[i + 1]);
          i += !long_arg;
          break;
        case 'c':
          m_read_buffer.setRemoteCmd(long_arg ? arg + 2 : argv[i + 1]);
          i += !long_arg;
          break;
        case '-':  // all args after "--" are connection commands
          m_command = argv + i + 1;
          break;
      }
    } else {
      m_command = argv + i;
    }
  }
  if (m_connection_specs.size() == 0) throw MY_EXCEPTION("No connections defined");
  if (nullptr == m_command || nullptr == *m_command) throw MY_EXCEPTION("No connection command defined");
  if (socks5_spec_count > 1) throw MY_EXCEPTION("at most one socks 5 connection spec");
}

namespace {
short g_id = 0;
}

pid_t forkpipe(int& read_fd, int& write_fd) {
  int read_pipe[2];
  int write_pipe[2];
  if (pipe(read_pipe) == -1 || pipe(write_pipe) == -1) return (pid_t)-1;
  pid_t result = fork();
  if (result > (pid_t)0) {
    read_fd = read_pipe[0];
    write_fd = write_pipe[1];
    close(read_pipe[1]);
    close(write_pipe[0]);
  } else if (result == (pid_t)0) {
    // Child process
    // STDIN_FD
    dup2(write_pipe[0], 0);
    // STDOUT_FD
    dup2(read_pipe[1], 1);
    close(write_pipe[0]);
    close(write_pipe[1]);
    close(read_pipe[0]);
    close(read_pipe[1]);
  }
  return result;
}

void StdioTunnelLocal::start() {
  Trace trace("StdioTunnelLocal::start");
  pid_t child_pid;
#ifdef HAVE_FORKPTY
  if (m_force_pipe) {
#endif
    child_pid = forkpipe(m_read_fd, m_write_fd);
    if (child_pid < 0) throw MY_EXCEPTION_ERRNO;
#ifdef HAVE_FORKPTY
  } else {
    termios raw;
    termios* termiosptr = (m_is_terminal ? &m_termios : NULL);
    if (m_expects_pipe && m_is_terminal) {
      raw = m_termios;
      cfmakeraw(&raw);
      termiosptr = &raw;
    }

    child_pid = forkpty(&m_read_fd, NULL, termiosptr, NULL);
    if (child_pid < 0) throw MY_EXCEPTION_ERRNO;
    if (child_pid) {
      m_write_fd = dup(m_read_fd);
      if (m_write_fd < 0) throw MY_EXCEPTION_ERRNO;
    }
  }
#endif

  if (child_pid) {
    /**
     * Create connection objects
     */
    for (auto i = m_connection_specs.begin(); i != m_connection_specs.end(); ++i) {
      TunnelConnectionPtr pConn = TunnelConnection::CreateConnection(*this, *i, true, g_id++);
      if (pConn) {
        m_connection_map[pConn->getID()] = pConn;
      }
    }

    /* Make pipes non-blocking and register with the poller
     * a polled that
     * copies standard in to the write pipe
     * and copies the read pipe to stdout,
     * checking for the magic pattern from StdioTunnelRemote
     */
    if (fcntl(m_read_fd, F_SETFL, O_NONBLOCK) == -1 || fcntl(m_write_fd, F_SETFL, O_NONBLOCK) == -1 ||
        fcntl(STDIN_FD, F_SETFL, O_NONBLOCK) == -1 || fcntl(STDOUT_FD, F_SETFL, O_NONBLOCK) == -1)
      throw MY_EXCEPTION_ERRNO;
    getWriter().m_poller = &getPoller();
    getWriter().m_polled = this;
    getWriter().m_fd = m_write_fd;
    m_poller.addPolled(*this);
    m_poller.addPolled(*this);
    m_poller.addPolled(*this);
    m_poller.addPolled(*this);
    m_poller.start();
  } else {
    // argv[argc] shall be a null pointer. (from Program startup section of C Standard)
    execvp(*m_command, m_command);
    /* Should never get here */
    exit(-1);
  }
}

void StdioTunnelLocal::setPollfd(pollfd& poll_struct) {
  Trace trace("StdioTunnelLocal::setPollfd");
  switch (m_state) {
    case INITIAL:
      poll_struct.fd = STDIN_FD;
      poll_struct.events = POLLIN;
      m_state = SET_STDIO_READ;
      break;
    case SET_STDIO_READ:
      poll_struct.fd = STDOUT_FD;
      poll_struct.events = 0;
      m_state = SET_STDIO_WRITE;
      break;
    case SET_STDIO_WRITE:
      poll_struct.fd = m_read_fd;
      poll_struct.events = POLLIN;
      m_state = SET_PIPE_READ;
      break;
    case SET_PIPE_READ:
      poll_struct.fd = m_write_fd;
      poll_struct.events = 0;
      m_state = PASS_THROUGH;
      break;
    default:
      throw MY_EXCEPTION("Bad state");
  }
}

void StdioTunnelLocal::polled(Poller&, pollfd& poll_struct) {
  Trace trace("StdioTunnelLocal::polled");
  switch (m_state) {
    case PASS_THROUGH:
      if (poll_struct.fd == m_read_fd) {
        Trace t("Read from pipe");
        if (m_read_buffer.readData(m_read_fd) == 0) throw MY_EXCEPTION("Read side closed");
        if (m_read_buffer.unsentData()) getPoller().setEvents(*this, STDOUT_FD, POLLOUT);
      }
      if (poll_struct.fd == m_write_fd) {
        Trace t("Write to pipe");
        if (!(poll_struct.revents & POLLOUT)) {
          throw MY_EXCEPTION("unexpected revents in write to pipe");
        }
        m_write_buffer.writeData(m_write_fd);
        if (!m_write_buffer.unsentData()) {
          getPoller().setEvents(*this, m_write_fd, 0);
        }
      }
      if (poll_struct.fd == STDIN_FD) {
        Trace t("Read from stdin");
        if (m_write_buffer.readData(STDIN_FD) == 0) throw MY_EXCEPTION("Stdin closed");
        if (m_write_buffer.unsentData()) getPoller().setEvents(*this, m_write_fd, POLLOUT);
      }
      if (poll_struct.fd == STDOUT_FD) {
        Trace t("Write to stdout");
        if (!(poll_struct.revents & POLLOUT)) throw MY_EXCEPTION("unexpected revents");
        m_read_buffer.writeData(STDOUT_FD);
        if (!m_read_buffer.unsentData()) getPoller().setEvents(*this, STDOUT_FD, 0);
      }
      break;
    case HAND_SHAKE:
    case CONNECTED:
      if (poll_struct.fd == m_read_fd) {
        Trace t("Connected read from pipe");
        if (poll_struct.revents & (POLLERR | POLLNVAL | POLLHUP)) throw MY_EXCEPTION("Read pipe closed/invalid");
        getReader().fill(poll_struct);
        if (getReader().available()) {
          short cmd = getReader().readShort();
          switch (cmd) {
            case CMD_ERROR:
              reportError(getReader().readString());
              break;
            case CMD_CREATE:
              m_state = CONNECTED;
              break;
            case CMD_CONNECTION: {
              int id = getReader().readShort();
              m_connection_map[id]->processMessage(*this);
            } break;
            case CMD_SHUTDOWN:
              getPoller().requestFinish();
              setShutdownByRequest();
              break;
          }
        }
      }
      if (poll_struct.fd == m_write_fd) {
        if (poll_struct.revents & (POLLERR | POLLNVAL | POLLHUP)) throw MY_EXCEPTION("Write pipe closed/invalid");
        Trace t("Connected write to pipe");
        getWriter().empty(poll_struct);
        if (!getWriter().pendingWrite()) getPoller().setEvents(*this, m_write_fd, 0);
      }
      if (poll_struct.fd == STDOUT_FD) {
        Trace t("Connected write to stdout");
        if (m_read_buffer.unsentData())
          m_read_buffer.writeData(STDOUT_FD);
        else
          getPoller().removePolled(*this, STDOUT_FD);
      }
      break;
    default:
      break;
  }
}

void StdioTunnelLocal::reportError(string message) {
  Trace trace("StdioTunnelLocal::reportError");
  cerr << message.c_str() << cerr_endl() << flush;
}

void StdioTunnelRemote::start() {
  Trace trace("StdioTunnelRemote::start");
  if (write(STDOUT_FD, m_magic_string.c_str(), m_magic_string.length()) != (ssize_t)m_magic_string.length())
    throw MY_EXCEPTION_ERRNO;
  if (fcntl(STDIN_FD, F_SETFL, O_NONBLOCK) == -1 || fcntl(STDOUT_FD, F_SETFL, O_NONBLOCK) == -1)
    throw MY_EXCEPTION_ERRNO;
  getWriter().m_poller = &getPoller();
  getWriter().m_polled = this;
  getWriter().m_fd = STDOUT_FD;
  m_poller.addPolled(*this);
  m_poller.addPolled(*this);
  m_poller.start();
}

void StdioTunnelRemote::polled(Poller&, pollfd& poll_struct) {
  Trace trace("StdioTunnelRemote::polled");
  if (poll_struct.fd == STDIN_FD) {
    if (poll_struct.revents & (POLLHUP | POLLNVAL | POLLERR)) throw MY_EXCEPTION("Stdin closed");
    if (poll_struct.revents & POLLIN)
      getReader().fill(poll_struct);
    else
      throw MY_EXCEPTION("Bad poll");
    if (getReader().available()) {
      int command = getReader().readShort();
      switch (m_state) {
        case HANDSHAKING:
          if (command != CMD_CREATE) throw MY_EXCEPTION("Unexpected handshake command");
          {
            int count = getReader().readShort();
            for (int i = 0; i < count; ++i) {
              int id = getReader().readShort();
              auto spec = make_shared<ConnectionSpec>();
              spec->deserialize(getReader());

              if (spec->isSocks5()) {
                if (m_socks5_server) throw MY_EXCEPTION("should the only one socks5 connection");
                spec->m_connect_host = "localhost";

                // start socks5 server
                auto socks5_uninit = make_shared<Socks5Server>();
                signal(SIGPIPE, SIG_IGN);
                if (server_setup(socks5_uninit.get(), spec->m_connect_host.c_str(), spec->m_connect_port)) {
                  throw MY_EXCEPTION("failed to starup socks5 server");
                }
                m_socks5_server = std::move(socks5_uninit);
                if (fcntl(m_socks5_server->fd, F_SETFL, O_NONBLOCK) < 0) throw MY_EXCEPTION_ERRNO;

                // update port
                sockaddr_in saddr;
                socklen_t len = sizeof(saddr);
                if (getsockname(m_socks5_server->fd, (struct sockaddr*)&saddr, &len) != -1) {
                  spec->m_connect_port = ntohs(saddr.sin_port);
                } else {
                  throw MY_EXCEPTION("failed to get socks5 server port");
                }

                getPoller().addPolled(m_socks5_server->getPolled());
              }

              TunnelConnectionPtr pConn = TunnelConnection::CreateConnection(*this, spec, false, id);
              if (pConn) {
                m_connection_map[id] = pConn;
              }
            }
            m_state = CONNECTED;
            getWriter().writeShort(CMD_CREATE);
            getWriter().send();
          }
          break;
        case CONNECTED:
          if (command == CMD_SHUTDOWN) {
            getPoller().requestFinish();
            setShutdownByRequest();
          } else if (command != CMD_CONNECTION)
            throw MY_EXCEPTION(static_cast<stringstream&&>(stringstream() << "Unexpected command: " << command));
          else {
            m_connection_map[getReader().readShort()]->processMessage(*this);
          }
          break;
        default:
          break;
      }
    }
  }
  if (poll_struct.fd == STDOUT_FD) {
    if (poll_struct.revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
      m_state = SHUT_DOWN;
      getPoller().requestFinish();
    } else {
      getWriter().empty(poll_struct);
      if (!getWriter().pendingWrite()) {
        getPoller().setEvents(*this, STDOUT_FD, 0);
      }
    }
  }
}

void StdioTunnelRemote::setPollfd(pollfd& poll_struct) {
  Trace trace("StdioTunnelRemote::setPollfd");
  switch (m_state) {
    case INITIAL:
      poll_struct.fd = STDIN_FD;
      poll_struct.events = POLLIN;
      m_state = SET_STDIO_READ;
      break;
    case SET_STDIO_READ:
      poll_struct.fd = STDOUT_FD;
      poll_struct.events = 0;
      m_state = HANDSHAKING;
      break;
    default:
      throw MY_EXCEPTION("Bad state");
  }
}

void StdioTunnelRemote::reportError(string message) {
  getWriter().writeShort((short)CMD_ERROR);
  getWriter().writeString(message);
  getWriter().send();
}

class ListenSide : public TunnelConnection, public Polled {
  int m_listen_socket;
  int m_next_id;

 public:
  ListenSide(shared_ptr<ConnectionSpec> spec, int id, CustomWriteStream& writer);
  virtual ~ListenSide() = default;
  Polled& getPolled() { return *this; }
  void processMessage(StdioTunnel& tunnel);
  bool initialize();
  void polled(Poller& poller, pollfd& poll_struct);
  void setPollfd(pollfd& poll_struct);
  void cleanup(pollfd& poll_struct);
};

class ConnectSide : public TunnelConnection {
 public:
  ConnectSide(shared_ptr<ConnectionSpec> spec, int id, CustomWriteStream& writer);
  virtual ~ConnectSide() = default;
  void processMessage(StdioTunnel& tunnel);
};

void Socks5Server::setPollfd(pollfd& poll_struct) {
  Trace trace("Socks5Server::setPollfd");
  poll_struct.fd = fd;
  poll_struct.events = POLLIN;
}

#ifdef PTHREAD_STACK_MIN
#  define THREAD_STACK_SIZE MAX(16 * 1024, PTHREAD_STACK_MIN)
#else
#  define THREAD_STACK_SIZE 64 * 1024
#endif

#if defined(__APPLE__)
#  undef THREAD_STACK_SIZE
#  define THREAD_STACK_SIZE 64 * 1024
#elif defined(__GLIBC__) || defined(__FreeBSD__) || defined(__sun__)
#  undef THREAD_STACK_SIZE
#  define THREAD_STACK_SIZE 32 * 1024
#endif

void Socks5Server::collect() {
  for (size_t i = 0; i < sblist_getsize(m_threads);) {
    const auto thread = *((struct thread**)sblist_get(m_threads, i));
    if (thread->done) {
      pthread_join(thread->pt, 0);
      sblist_delete(m_threads, i);
      free(thread);
    } else
      i++;
  }
}

static int connect_socks_target(unsigned char* buf, size_t n, client* client) {
  if (n < 5) return -Socks5Server::EC_GENERAL_FAILURE;
  if (buf[0] != 5) return -Socks5Server::EC_GENERAL_FAILURE;
  if (buf[1] != 1) return -Socks5Server::EC_COMMAND_NOT_SUPPORTED; /* we support only CONNECT method */
  if (buf[2] != 0) return -Socks5Server::EC_GENERAL_FAILURE;       /* malformed packet */

  int af = AF_INET;
  size_t minlen = 4 + 4 + 2, l;
  char namebuf[256];
  struct addrinfo* remote;

  switch (buf[3]) {
    case 4: /* ipv6 */
      af = AF_INET6;
      minlen = 4 + 2 + 16;
      /* fall through */
    case 1: /* ipv4 */
      if (n < minlen) return -Socks5Server::EC_GENERAL_FAILURE;
      if (namebuf != inet_ntop(af, buf + 4, namebuf, sizeof namebuf))
        return -Socks5Server::EC_GENERAL_FAILURE; /* malformed or too long addr */
      break;
    case 3: /* dns name */
      l = buf[4];
      minlen = 4 + 2 + l + 1;
      if (n < 4 + 2 + l + 1) return -Socks5Server::EC_GENERAL_FAILURE;
      memcpy(namebuf, buf + 4 + 1, l);
      namebuf[l] = 0;
      break;
    default:
      return -Socks5Server::EC_ADDRESSTYPE_NOT_SUPPORTED;
  }
  unsigned short port;
  port = (buf[minlen - 2] << 8) | buf[minlen - 1];
  /* there's no suitable errorcode in rfc1928 for dns lookup failure */
  if (resolve(namebuf, port, &remote)) return -Socks5Server::EC_GENERAL_FAILURE;
  int fd = socket(remote->ai_family, SOCK_STREAM, 0);
  if (fd == -1) {
  eval_errno:
    if (fd != -1) close(fd);
    freeaddrinfo(remote);
    switch (errno) {
      case ETIMEDOUT:
        return -Socks5Server::EC_TTL_EXPIRED;
      case EPROTOTYPE:
      case EPROTONOSUPPORT:
      case EAFNOSUPPORT:
        return -Socks5Server::EC_ADDRESSTYPE_NOT_SUPPORTED;
      case ECONNREFUSED:
        return -Socks5Server::EC_CONN_REFUSED;
      case ENETDOWN:
      case ENETUNREACH:
        return -Socks5Server::EC_NET_UNREACHABLE;
      case EHOSTUNREACH:
        return -Socks5Server::EC_HOST_UNREACHABLE;
      case EBADF:
      default:
        // cerr << "socks5 socket/connect" << cerr_endl() << flush;
        return -Socks5Server::EC_GENERAL_FAILURE;
    }
  }
  if (connect(fd, remote->ai_addr, remote->ai_addrlen) == -1) goto eval_errno;

  freeaddrinfo(remote);
#ifndef CONFIG_LOG
#  define CONFIG_LOG 0  // 1
#endif
  if (CONFIG_LOG) {
    char clientname[256];
    af = SOCKADDR_UNION_AF(&client->addr);
    void* ipdata = SOCKADDR_UNION_ADDRESS(&client->addr);
    inet_ntop(af, ipdata, clientname, sizeof clientname);
    cerr << "client[" << client->fd << "] " << clientname << ": connected to " << namebuf << ":" << port << cerr_endl()
         << flush;
  }
  return fd;
}
static int socks5_handshake(Socks5Server::thread* t) {
  unsigned char buf[1024];
  ssize_t n;
  int ret;
  t->state = Socks5Server::SS_1_CONNECTED;
  while ((n = recv(t->client.fd, buf, sizeof buf, 0)) > 0) {
    switch (t->state) {
      case Socks5Server::SS_1_CONNECTED:
        buf[0] = 5;
        buf[1] = Socks5Server::AM_NO_AUTH;
        if (write(t->client.fd, buf, 2) != 2) return -1;
        t->state = Socks5Server::SS_3_AUTHED;
        break;
      case Socks5Server::SS_2_NEED_AUTH:
        buf[0] = 1;
        buf[1] = Socks5Server::EC_SUCCESS;
        if (write(t->client.fd, buf, 2) != 2) return -1;
        t->state = Socks5Server::SS_3_AUTHED;
        break;
      case Socks5Server::SS_3_AUTHED:
        ret = connect_socks_target(buf, n, &t->client);
        /* position 4 contains ATYP, the address type, which is the same as used in the connect
           request. we're lazy and return always IPV4 address type in errors. */
        buf[0] = 5;
        buf[1] = ret < 0 ? ret * -1 : Socks5Server::EC_SUCCESS;
        buf[2] = 0;
        buf[3] = 1; /*AT_IPV4*/
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = 0;
        buf[7] = 0;
        buf[8] = 0;
        buf[9] = 0;
        if (write(t->client.fd, buf, 10) != 10) return -1;
        return ret < 0 ? -1 : ret;
    }
  }
  return -1;
}
static void copyloop(int fd1, int fd2) {
  pollfd fds[2] = {
      [0] = {.fd = fd1, .events = POLLIN, .revents = 0},
      [1] = {.fd = fd2, .events = POLLIN, .revents = 0},
  };

  while (1) {
    /* inactive connections are reaped after 15 min to free resources.
       usually programs send keep-alive packets so this should only happen
       when a connection is really unused. */
    switch (poll(fds, 2, 60 * 15 * 1000)) {
      case 0:
        return;
      case -1:
        if (errno == EINTR || errno == EAGAIN)
          continue;
        else {
          // cerr << "poll" << cerr_endl() << flush;
        }
        return;
    }
    int infd = (fds[0].revents & POLLIN) ? fd1 : fd2;
    int outfd = infd == fd2 ? fd1 : fd2;
    /* since the biggest stack consumer in the entire code is
       libc's getaddrinfo(), we can safely use at least half the
       available stacksize to improve throughput. */
    char buf[MIN(16 * 1024, THREAD_STACK_SIZE / 2)];
    ssize_t sent = 0, n = read(infd, buf, sizeof buf);
    if (n <= 0) return;
    while (sent < n) {
      ssize_t m = write(outfd, buf + sent, n - sent);
      if (m < 0) return;
      sent += m;
    }
  }
}
static void* socks5_clientthread(void* data) {
  const auto t = (Socks5Server::thread*)data;
  int remotefd = socks5_handshake(t);
  if (remotefd != -1) {
    copyloop(t->client.fd, remotefd);
    close(remotefd);
  }
  close(t->client.fd);
  t->done = 1;
  return 0;
}

void Socks5Server::polled(Poller&, pollfd&) {
  collect();
  client c;
  auto curr = (thread*)malloc(sizeof(struct thread));
  if (!curr) goto oom;
  curr->done = 0;
  if (server_waitclient(this, &c)) {
    // cerr << "failed to accept socks5 connection" << cerr_endl() << flush;
    free(curr);
    return;
  }
  curr->client = c;
  if (!sblist_add(m_threads, &curr)) {
    close(curr->client.fd);
    free(curr);
  oom:
    // cerr << "rejecting socks5 connection due to OOM" << cerr_endl() << flush;
    return;
  }

  pthread_attr_t *a = nullptr, attr;
  if (pthread_attr_init(&attr) == 0) {
    a = &attr;
    pthread_attr_setstacksize(a, THREAD_STACK_SIZE);
  }
  if (pthread_create(&curr->pt, a, socks5_clientthread, curr) != 0) {
    // cerr << "pthread_create failed. OOM?" << cerr_endl() << flush;
  }
  if (a) pthread_attr_destroy(&attr);
}

TunnelConnection::TunnelConnection(shared_ptr<ConnectionSpec> spec, int id, CustomWriteStream& writer)
    : m_spec(spec), m_id(id), m_writer(writer) {}

TunnelConnectionPtr TunnelConnection::CreateConnection(StdioTunnel& tunnel, shared_ptr<ConnectionSpec> spec,
                                                       bool is_local, int id) {
  Trace trace("TunnelConnection::CreateConnection");
  TunnelConnectionPtr result;
  if ((is_local && spec->m_connection_type == ConnectionSpec::CONNECT) ||
      (!is_local && spec->m_connection_type == ConnectionSpec::LISTEN)) {
    auto listener = make_shared<ListenSide>(spec, id, tunnel.getWriter());
    if (listener->initialize()) {
      result = listener;
      tunnel.getPoller().addPolled(listener->getPolled());
    }
  } else {
    result = make_shared<ConnectSide>(spec, id, tunnel.getWriter());
  }
  return result;
}

void TunnelConnection::closeLink(com::antlersoft::net::Poller& poller, ConnectionLink* const to_close) {
  if (!to_close->isClosing()) {
    to_close->setClosing(poller);
    m_writer.writeShort(StdioTunnel::CMD_CONNECTION);
    m_writer.writeShort(getID());
    m_writer.writeShort(LINK_CLOSE);
    m_writer.writeShort(to_close->getID());
    m_writer.send();
  }
}

void TunnelConnection::sendData(ConnectionLink* const source, char* bytes, int length) {
  Trace trace("TunnelConnection::sendData");
  m_writer.writeShort(StdioTunnel::CMD_CONNECTION);
  m_writer.writeShort(getID());
  m_writer.writeShort((getSpec()->m_flags & ConnectionSpec::ACK_PACKETS) ? LINK_SEND_FOR_ACK : LINK_SEND);
  m_writer.writeShort(source->getID());
  m_writer.writeShort(length);
  m_writer.writeBytes(bytes, length);
  m_writer.send();
}

void TunnelConnection::receiveData(StdioTunnel& tunnel, const ConnectionLinkPtr& link) {
  Trace trace("TunnelConnection::receiveData");
  char buffer[BUFFER_SIZE];
  short length = tunnel.getReader().readShort();
  if (length > BUFFER_SIZE) throw MY_EXCEPTION("Bad receive length");
  tunnel.getReader().readBytes(buffer, length);
  if (!link->isClosing()) {
    link->getBuffer().pushOnBuffer(buffer, length);
    tunnel.getPoller().setEvents(link->getPolled(), link->getSocket(), POLLOUT);
  }
}

void TunnelConnection::processLinkMessage(StdioTunnel& tunnel, int command, const ConnectionLinkPtr& link) {
  Trace trace("TunnelConnection::processLinkMessage");
  switch (command) {
    case LINK_SEND:
      receiveData(tunnel, link);
      break;
    case LINK_CLOSE: {
      closeLink(tunnel.getPoller(), link.get());
      m_link_map.erase(m_link_map.find(link->getID()));
    } break;
    case LINK_SEND_FOR_ACK:
      receiveData(tunnel, link);
      if (!link->isClosing()) {
        m_writer.writeShort(StdioTunnel::CMD_CONNECTION);
        m_writer.writeShort(getID());
        m_writer.writeShort(LINK_ACK);
        m_writer.writeShort(link->getID());
        m_writer.send();
        break;
      }
      [[gnu::fallthrough]];
    case LINK_ACK:
      link->setAcknowledged(true, tunnel.getPoller());
      break;
  }
}

ListenSide::ListenSide(shared_ptr<ConnectionSpec> spec, int id, CustomWriteStream& writer)
    : TunnelConnection(spec, id, writer), m_next_id(501) {}

void ListenSide::processMessage(StdioTunnel& tunnel) {
  Trace trace("ListenSide::processMessage");
  short cmd = tunnel.getReader().readShort();
  short link_id = tunnel.getReader().readShort();
  map<short, ConnectionLinkPtr>::iterator it = m_link_map.find(link_id);
  if (it == m_link_map.end()) {
    throw MY_EXCEPTION(static_cast<stringstream&&>(stringstream() << "Link: " << link_id << " not found"));
  }
  ConnectionLinkPtr link = it->second;
  switch (cmd) {
    case LINK_CREATE: {
      short result_code = tunnel.getReader().readShort();
      if (result_code == LINK_SUCCESS) {
        tunnel.getPoller().addPolled(link->getPolled());
      } else {
        close(link->getSocket());
        m_link_map.erase(it);
      }
    } break;
    default:
      processLinkMessage(tunnel, cmd, link);
      break;
  }
}

bool ListenSide::initialize() {
  bool result = false;
  Trace trace("ListenSide::initialize");

  sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  if (getSpec()->m_flags & ConnectionSpec::PROMISCUOUS)
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  else
    saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  saddr.sin_port = htons(getSpec()->m_listen_port);
  m_listen_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_listen_socket >= 0) {
    if (::bind(m_listen_socket, (const sockaddr*)&saddr, sizeof(saddr)) == 0) {
      if (fcntl(m_listen_socket, F_SETFL, O_NONBLOCK) >= 0) {
        if (listen(m_listen_socket, 5) == 0) {
          result = true;
        }
      }
    }
  }
  return result;
}

void ListenSide::polled(Poller&, pollfd&) {
  Trace trace("ListenSide::polled");

  sockaddr_in saddr;
  socklen_t size_return = sizeof(&saddr);
  int connect_sock = accept(m_listen_socket, (sockaddr*)&saddr, &size_return);
  if (connect_sock < 0) throw MY_EXCEPTION_ERRNO;
  if (fcntl(connect_sock, F_SETFL, O_NONBLOCK) < 0) throw MY_EXCEPTION_ERRNO;
  int next_id = m_next_id++;
  auto link = make_shared<ConnectionLink>(*this, next_id, connect_sock);
  m_link_map[next_id] = link;
  m_writer.writeShort(StdioTunnel::CMD_CONNECTION);
  m_writer.writeShort(getID());
  m_writer.writeShort(LINK_CREATE);
  m_writer.writeShort(next_id);
  m_writer.send();
}

void ListenSide::setPollfd(pollfd& poll_struct) {
  Trace trace("ListenSide::setPollfd");
  poll_struct.fd = m_listen_socket;
  poll_struct.events = POLLIN;
}

void ListenSide::cleanup(pollfd&) { close(m_listen_socket); }

ConnectSide::ConnectSide(shared_ptr<ConnectionSpec> spec, int id, CustomWriteStream& writer)
    : TunnelConnection(spec, id, writer) {}

void ConnectSide::processMessage(StdioTunnel& tunnel) {
  Trace trace("ConnectSide::processMessage");
  short command = tunnel.getReader().readShort();
  short link_id = tunnel.getReader().readShort();
  map<short, ConnectionLinkPtr>::iterator it;
  ConnectionLinkPtr link;
  if (command != LINK_CREATE) {
    it = m_link_map.find(link_id);
    if (it == m_link_map.end()) {
      Trace t("connect side link id not found");

      if (command == LINK_CREATE) {
        Trace u("for link create");
      }
      if (command == LINK_SEND || command == LINK_SEND_FOR_ACK) {
        Trace v("for link send");
      }
      return;
    }
    link = it->second;
  }
  switch (command) {
    case LINK_CREATE: {
      bool success = false;
      hostent* host_entry = gethostbyname(getSpec()->m_connect_host.c_str());
      if (!host_entry) {
        string s = getSpec()->m_connect_host + " not found";
        tunnel.reportError(s);
      } else {
        sockaddr_in dest_addr;
        dest_addr.sin_family = AF_INET;
        memcpy((char*)&dest_addr.sin_addr.s_addr, host_entry->h_addr, host_entry->h_length);
        dest_addr.sin_port = htons(getSpec()->m_connect_port);
        int connect_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connect_sock < 0) {
          string s = "Cannot create connect socket";
          tunnel.reportError(s);
        } else {
          if (connect(connect_sock, (sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
            tunnel.reportError(string("Failed to connect to ") + getSpec()->m_connect_host);
          } else {
            if (fcntl(connect_sock, F_SETFL, O_NONBLOCK) < 0) {
              tunnel.reportError("Failed to set connect socket non blocking");
            } else {
              success = true;
              link = make_shared<ConnectionLink>(*this, link_id, connect_sock);
              tunnel.getPoller().addPolled(link->getPolled());
              m_link_map[link_id] = link;
            }
          }
          if (!success) close(connect_sock);
        }
      }
      m_writer.writeShort(StdioTunnel::CMD_CONNECTION);
      m_writer.writeShort(getID());
      m_writer.writeShort(LINK_CREATE);
      m_writer.writeShort(link_id);
      m_writer.writeShort(success ? LINK_SUCCESS : LINK_FAILURE);
      m_writer.send();
    } break;
    default:
      processLinkMessage(tunnel, command, link);
      break;
  }
}

namespace {

static volatile bool g_signaled = false;

void signalHandler(int) { g_signaled = true; }

}  // namespace

void StdioTunnel::pollCallback(Poller& poller) {
  if (g_signaled) {
    Trace t("Requesting finish because signaled");
    poller.requestFinish();
  }
}

int main(int argc, char* argv[]) {
  Trace::setPrint(argc >= 2 && 0 == strcmp(argv[1], "-D"));
  Trace trace("main");

  signal(SIGHUP, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGQUIT, signalHandler);
  try {
    shared_ptr<StdioTunnel> tunnel;
    try {
      tunnel = StdioTunnel::CreateTunnel(argc, argv);
    } catch (MyException& e) {
      cerr << cerr_endl()
           << "Usage: StdioTunnel [-D] [-l] [-d] [-T|-t] <-L|-R <connection spec>>... [-p login-pattern] "
              "[-c remote-cmd] connection-command [args...]"
           << cerr_endl() << cerr_endl() << "or (on the remote side)" << cerr_endl() << cerr_endl()
           << "StdioTunnel [-D] -r" << cerr_endl() << cerr_endl() << "or (print version)" << cerr_endl() << cerr_endl()
           << "StdioTunnel -V" << cerr_endl() << cerr_endl()
           << "where connection_spec = <listen port>:<connect host>:<connect port>[:ap]" << cerr_endl()
           << "a remote socks 5 connection_spec is write as -l -L <listen port>::0" << cerr_endl() << flush;
      throw "";
    }
    tunnel->startFinish();
  } catch (const char* const s) {
    cerr << s << cerr_endl() << flush;
  } catch (const exception& e) {
    cerr << e.what() << cerr_endl() << flush;
  } catch (...) {
    cerr << "Unknown exception" << cerr_endl() << flush;
  }
  cerr << "Normal exit" << cerr_endl() << flush;

  return 0;
}
