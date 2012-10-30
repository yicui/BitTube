

#ifndef __PEERSTATE_H__
#define __PEERSTATE_H__

#include <Winsock2.h>

class Peer;

class PeerState
{
public:
	virtual SOCKET SetFd(Peer * p, const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp) = 0;

	virtual void CheckFd(Peer * p, fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds) = 0;
};



#endif

