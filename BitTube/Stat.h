

#ifndef __STAT_H__
#define __STAT_H__

#include <time.h>

class Stat
{
public:
	static Stat * Instance(void);

	void ByteFromServer(size_t num) {this->bytesfromserver_ += num;}

	size_t ByteFromServer(void) {return this->bytesfromserver_;}

	void ByteFromPeers(size_t num) {this->bytesfrompeers_ += num;}

	size_t ByteFromPeers(void) {return this->bytesfrompeers_;}

	double ServerRate(void);

	double PeerRate(void);

protected:
	Stat();

private:
	static Stat * instance_;

	time_t start_time_;
	size_t bytesfromserver_;
	size_t bytesfrompeers_;
};


#endif

