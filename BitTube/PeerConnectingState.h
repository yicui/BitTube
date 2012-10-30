

#ifndef __PEERCONNECTINGSTATE_H__
#define __PEERCONNECTINGSTATE_H__


#include "PeerState.h"

class PeerConnectingState : public PeerState
{
	public:
	virtual SOCKET SetFd(Peer * p, const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp);

	virtual void CheckFd(Peer * p, fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds);
};

#endif

