#include <deque>
#include <iostream>

#include "com/antlersoft/Trace.h"
#include "com/antlersoft/StderrEndl.h"

using namespace std;

namespace {
static deque<string> scope_stack;
static bool print_it=false;
}

namespace com { namespace antlersoft {

Trace::Trace( string s)
{
	scope_stack.push_front( s);
	if ( print_it)
		cerr<<"into "<<s<<cerr_endl()<<flush;
}

Trace::~Trace()
{
	if ( print_it)
		cerr<<"out of "<<scope_stack.front()<<cerr_endl()<<flush;
	scope_stack.pop_front();
}

bool Trace::setPrint( bool p)
{
	bool old_p=print_it;
	print_it=p;
	return old_p;
}

void Trace::getTrace()
{
	for ( deque<string>::iterator i=scope_stack.begin();
		i!=scope_stack.end(); i++)
		cerr<<"  from "<<*i<<cerr_endl()<<flush;
}

} }
