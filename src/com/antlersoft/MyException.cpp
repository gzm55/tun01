#include "com/antlersoft/MyException.h"

#include <cerrno>
#include <iostream>
#include <sstream>
#include <cstring>

#include "com/antlersoft/StderrEndl.h"
#include "com/antlersoft/Trace.h"

using namespace std;

namespace com {
namespace antlersoft {

  MyException::MyException(const char* msg, const char* file, int line) {
    stringstream s;
    s << msg << " at line " << line << " in " << file;
    m_msg = s.str();
    cerr << s.str() << cerr_endl() << flush;
    Trace::getTrace();
  }

  MyException::MyException(const char* file, int line) {
    stringstream s;
    s << "Error: " << errno << cerr_endl() << ((errno >= 1 && errno < 500) ? strerror(errno) : " errno not set")
      << " at line " << line << " in " << file;
    m_msg = s.str();
    cerr << s.str() << cerr_endl() << flush;
    Trace::getTrace();
  }

  MyException::~MyException() throw() {}

  const char* MyException::what() const throw() { return m_msg.c_str(); }

}  // namespace antlersoft
}  // namespace com
