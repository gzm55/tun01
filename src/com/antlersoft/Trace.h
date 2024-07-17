#ifndef COM_ANTLERSOFT_TRACE_H
#define COM_ANTLERSOFT_TRACE_H

#include <string>

namespace com {
namespace antlersoft {
  class Trace {
   public:
    Trace(std::string scope_name);
    ~Trace();
    static bool setPrint(bool to_print);
    static void getTrace();
  };
}  // namespace antlersoft
}  // namespace com

#endif
