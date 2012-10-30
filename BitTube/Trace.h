
#ifndef __TRACE_H__
#define __TRACE_H__

//switch of whether need trace infomation
//#define TRACE

#include <iostream>
#include <string>


#ifdef TRACE

class Tracer
{
public:
	Tracer(const std::string & str);
	~Tracer();
private:
	static unsigned int nt_;
	const std::string str_;
};


#define Trace(str) Tracer t(str)

#else

#define Trace(str)

#endif


#endif


