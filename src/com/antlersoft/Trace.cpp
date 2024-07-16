#include <deque>
#include <iostream>

#include "com/antlersoft/Trace.h"

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
		cerr<<"into "<<s<<endl;
}

Trace::~Trace()
{
	if ( print_it)
		cerr<<"out of "<<scope_stack.front()<<endl;
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
		cerr<<"from "<<*i<<endl;
}

} }
