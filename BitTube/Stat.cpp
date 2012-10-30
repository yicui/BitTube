

#include "Stat.h"


Stat * Stat::instance_ = NULL;

Stat * Stat::Instance(void)
{
	if(NULL == Stat::instance_)
		Stat::instance_ = new Stat;

	return Stat::instance_;
}

double Stat::ServerRate(void)
{
	time_t now;
	time(&now);

	if(0 == now - this->start_time_)
		return 0;

	return this->bytesfromserver_ * 1. / ((now - this->start_time_) * 1024);
}

double Stat::PeerRate(void)
{
	time_t now;
	time(&now);

	if(0 == now - this->start_time_)
		return 0;

	return this->bytesfrompeers_ * 1. / ((now - this->start_time_) * 1024);
}

Stat::Stat() : bytesfromserver_(0), bytesfrompeers_(0)
{
	time(&(this->start_time_));
}