#include "com/antlersoft/MyException.h"

#include <cerrno>
#include <cstring>
#include <iostream>

#include "com/antlersoft/StderrEndl.h"
#include "com/antlersoft/Trace.h"

using namespace std;

namespace com {
namespace antlersoft {

  MyException::MyException(std::stringstream&& msg_stream, const char* const file, const int line) {
    msg_stream << " at " << file << ':' << line;
    m_msg = msg_stream.str();
    cerr << m_msg << cerr_endl() << flush;
    Trace::getTrace();
  }
  MyException::MyException(const char* const msg, const char* const file, const int line)
      : MyException(stringstream() << msg, file, line) {}

  MyException::MyException(const char* file, int line)
      : MyException(stringstream() << "Error: " << errno << cerr_endl()
                                   << ((errno >= 1 && errno < 500) ? strerror(errno) : " errno not set"),
                    file, line) {}

  const char* MyException::what() const throw() { return m_msg.c_str(); }

}  // namespace antlersoft
}  // namespace com
