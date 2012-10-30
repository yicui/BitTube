

#include "Trace.h"


#ifdef TRACE

unsigned int Tracer::nt_ = 0;

Tracer::Tracer(const std::string & str) : str_(str)
{
	++ this->nt_;
	for(unsigned int i = 1; i < this->nt_; ++i)
		std::cout << "\t";
	std::cout << "Entering " << this->str_ << " ... " << std::endl;
}


Tracer::~Tracer()
{
	for(unsigned int i = 1; i < this->nt_; ++i)
		std::cout << "\t";
	std::cout << "Leaving " << this->str_ << " ... " << std::endl;
	-- this->nt_;
}

#endif