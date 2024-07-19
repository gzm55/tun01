#ifndef COM_ANTLERSOFT_NET_SOCK_BUFFER_H
#define COM_ANTLERSOFT_NET_SOCK_BUFFER_H

#include <cstddef>
#include <deque>

#include "com/antlersoft/net/PollConfig.h"

namespace com {
namespace antlersoft {
  namespace net {

    class SockBuffer {
     private:
      static const size_t BUFFER_SIZE = 4096;
      std::deque<char> m_queue;
      char* m_output_buf;
      int m_output_position;

     public:
      SockBuffer();
      ~SockBuffer();
      int readData(int read_fd);
      int writeData(int write_fd);
      /** This function may be overridden by sub-classes.
       * The default implementation does nothing. */
      virtual void processReadData(char* bytes, int& length, int max_length);
      /** This function may be overridden by sub-classes.
       * The default implementation does nothing. */
      virtual void processWrittenData(char* bytes, int length);
      /**
       * Returns a copy of the current buffer.  bytes is set
       * to a newly-allocated char array; length is set to the array
       * length.  It is the caller's responsibility to
       * delete[] the buffer returned in bytes[].
       */
      void getCopyOfBuffer(char*& bytes, int& length);
      void pushOnBuffer(const char* bytes, int length);
      bool unsentData() { return (m_output_position != BUFFER_SIZE || !m_queue.empty()); }
      virtual void clear();
    };

  }  // namespace net
}  // namespace antlersoft
}  // namespace com
#endif /* ndef COM_ANTLERSOFT_NET_SOCK_BUFFER_H */
