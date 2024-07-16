#include <sstream>
#include <iostream>
#include <cerrno>

#include "com/antlersoft/MyException.h"
#include "com/antlersoft/Trace.h"

using namespace std;

namespace com { namespace antlersoft {

MyException::MyException( const char* msg, const char* file, int line)
{
	stringstream s;
	s<<msg<<" at line "<<line<<" in "<<file<<endl;
	m_msg=s.str();
	cerr<<s.str()<<endl;
	Trace::getTrace();
}

MyException::MyException( const char* file, int line)
{
	stringstream s;
	s<<"Error: "<<errno<<endl<<((errno>=1 && errno<500) ? strerror( errno):" errno not set")<<" at line "<<line<<" in "<<file<<endl;
	m_msg=s.str();
	cerr<<s.str()<<endl;
	Trace::getTrace();
}

MyException::~MyException() throw()
{
}

const char* MyException::what() const throw()
{
	return m_msg.c_str();
}

} }
