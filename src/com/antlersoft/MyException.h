#ifndef MY_EXCEPTION_H
#define MY_EXCEPTION_H

#include <exception>
#include <sstream>
#include <string>

namespace com {
namespace antlersoft {
  class MyException : public std::exception {
   private:
    std::string m_msg;

   public:
    MyException(std::stringstream&& msg_stream, const char* file, int line);
    MyException(const char* msg, const char* file, int line);
    MyException(const char* file, int line);
    ~MyException() throw() = default;
    const char* what() const throw();
  };

}  // namespace antlersoft
}  // namespace com

#define MY_EXCEPTION(X) MyException((X), __FILE__, __LINE__)
#define MY_EXCEPTION_ERRNO MyException(__FILE__, __LINE__)
#endif
