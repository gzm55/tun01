#ifndef MY_EXCEPTION_H
#define MY_EXCEPTION_H

#include <exception>
#include <string>

namespace com {
namespace antlersoft {
  class MyException : std::exception {
   private:
    std::string m_msg;

   public:
    MyException(const char* msg, const char* file, int line);
    MyException(const char* file, int line);
    ~MyException() throw();
    const char* what() const throw();
  };

}  // namespace antlersoft
}  // namespace com

#define MY_EXCEPTION(X) MyException((X), __FILE__, __LINE__)
#define MY_EXCEPTION_ERRNO MyException(__FILE__, __LINE__)
#endif
