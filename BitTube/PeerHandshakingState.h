
#ifndef __PEERHANDSHAKINGSTATE_H__
#define __PEERHANDSHAKINGSTATE_H__



#include "PeerState.h"

class PeerHandshakingState : public PeerState
{
	public:
	virtual SOCKET SetFd(Peer * p, const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp);

	virtual void CheckFd(Peer * p, fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds);
};


#endif

