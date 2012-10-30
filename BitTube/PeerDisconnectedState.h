

#ifndef __PEERDISCONNECTEDSTATE_H__
#define __PEERDISCONNECTEDSTATE_H__

#include "PeerState.h"

class PeerDisconnectedState : public PeerState
{
public:
	virtual SOCKET SetFd(Peer * p, const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp);

	virtual void CheckFd(Peer * p, fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds);
};

#endif


