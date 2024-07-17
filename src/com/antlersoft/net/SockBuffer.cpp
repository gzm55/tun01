#include "com/antlersoft/net/SockBuffer.h"

#include <errno.h>
#include <unistd.h>

#include <algorithm>

#include "com/antlersoft/MyException.h"
#include "com/antlersoft/Trace.h"

using namespace std;
using namespace com::antlersoft;
using namespace com::antlersoft::net;

SockBuffer::SockBuffer() : m_output_buf(new char[BUFFER_SIZE]), m_output_position(BUFFER_SIZE) {
  Trace trace("SockBuffer::SockBuffer");
}

SockBuffer::~SockBuffer() {
  Trace trace("SockBuffer::~SockBuffer");
  delete[] m_output_buf;
}

int SockBuffer::readData(int read_fd) {
  Trace trace("SockBuffer::readData");
  char input_buf[BUFFER_SIZE];
  int read_count;
  int read_total = 0;
  do {
    read_count = read(read_fd, input_buf, BUFFER_SIZE);
    if (read_count < 0) {
      if (errno == EAGAIN || errno == EHOSTUNREACH || errno == ECONNRESET)
        read_count = 0;
      else
        throw MY_EXCEPTION_ERRNO;
    }
    processReadData(input_buf, read_count, BUFFER_SIZE);
    m_queue.insert(m_queue.end(), input_buf, input_buf + read_count);
    read_total += read_count;
  } while (read_count == BUFFER_SIZE);
  return read_total;
}

int SockBuffer::writeData(int write_fd) {
  Trace trace("SockBuffer::writeData");
  int write_count;
  if (m_output_position == BUFFER_SIZE) {
    // Refill output buffer
    int fill_size = m_queue.size();
    if (fill_size > (int)BUFFER_SIZE) fill_size = BUFFER_SIZE;
    m_output_position -= fill_size;
    for (; fill_size > 0; fill_size--) {
      m_output_buf[BUFFER_SIZE - fill_size] = m_queue.front();
      m_queue.pop_front();
    }
  }

  write_count = write(write_fd, m_output_buf + m_output_position, BUFFER_SIZE - m_output_position);
  if (write_count < 0) throw MY_EXCEPTION_ERRNO;
  processWrittenData(m_output_buf + m_output_position, write_count);
  m_output_position += write_count;
  return write_count;
}

void SockBuffer::processReadData(char*, int&, int) {}

void SockBuffer::processWrittenData(char*, int) {}

void SockBuffer::getCopyOfBuffer(char*& bytes, int& length) {
  length = m_queue.size();
  bytes = new char[length];
  copy(m_queue.begin(), m_queue.end(), bytes);
}

void SockBuffer::pushOnBuffer(const char* bytes, int length) { m_queue.insert(m_queue.end(), bytes, bytes + length); }

void SockBuffer::clear() {
  m_output_position = BUFFER_SIZE;
  m_queue.clear();
}
