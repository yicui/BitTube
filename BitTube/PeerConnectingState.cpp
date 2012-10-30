

#include "PeerConnectingState.h"
#include "Peer.h"
#include "Trace.h"

SOCKET PeerConnectingState::SetFd(Peer * p, const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp)
{
	Trace("PeerConnectingState::SetFd");

	FD_SET(p->sock_, &efdp);
	FD_SET(p->sock_, &wfdp);

	return p->sock_;
}



void PeerConnectingState::CheckFd(Peer * p, fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds)
{
	Trace("PeerConnectingState::CheckFd");

	if(FD_ISSET(p->sock_, &wfdp))// connect success
	{//std::cout << "connect success" << std::endl;
		FD_CLR(p->sock_, &wfdp);
		--nfds;

		if(p->Send_MovieID() < 0 ||	p->Send_Bitmap() < 0)
		{
			//std::cout << "send movie id or bitmap error" << std::endl;
			p->Reset();
			return;
		}

		p->ChangeState(Peer::HANDSHAKING);
	}
	else if(FD_ISSET(p->sock_, &efdp)) // connect failed
	{
		FD_CLR(p->sock_, &efdp);
		--nfds;

		//std::cout << "connect to remote peer failed" << std::endl;
		p->Reset();
	}
}

