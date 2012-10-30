
#include "PeerDisconnectedState.h"
#include "Peer.h"
#include "Trace.h"

SOCKET PeerDisconnectedState::SetFd(Peer * p, const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp)
{
	Trace("PeerDisconnectedState::SetFd");

	p->sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == p->sock_)
	{
		p->Reset();
		//std::cout << "socket() error" << std::endl;
		return INVALID_SOCKET;
	}

	unsigned long val = 1;
	if(ioctlsocket(p->sock_, FIONBIO, &val) < 0)
	{
		p->Reset();
		//std::cout << "set sock_ to non-block mode error!" << std::endl;
		return INVALID_SOCKET;
	}

	int r = connect(p->sock_, (struct sockaddr *)&(p->addr_), sizeof(p->addr_));

	if(r < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
		r = -2;

	if(0 == r) // connect success
	{
		//std::cout << "connect success" << std::endl;
		if(p->Send_MovieID() < 0 ||	p->Send_Bitmap() < 0)
		{
			//std::cout << "send movie id or bitmap error" << std::endl;
			p->Reset();
			return INVALID_SOCKET;
		}

		FD_SET(p->sock_, &rfdp);
		if(p->send_buffer_.Count() > 0)
			FD_SET(p->sock_, &wfdp);

		p->ChangeState(Peer::HANDSHAKING);
		return p->sock_;
	}
	else if(-1 == r) // failed
	{
		//std::cout << "connect failed" << std::endl;
		p->Reset();
		return INVALID_SOCKET;
	}
	else // -2 == r, in progress
	{
		//std::cout << "connect in progress" << std::endl;
		FD_SET(p->sock_, &efdp);
		FD_SET(p->sock_, &wfdp);

		p->ChangeState(Peer::CONNECTING);
		return p->sock_;
	}
}



void PeerDisconnectedState::CheckFd(Peer *, fd_set &, fd_set &, fd_set &, int &)
{
	return;
}


