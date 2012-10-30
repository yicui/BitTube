
#include "PeerReadyState.h"
#include "Peer.h"
#include "Trace.h"
#include "Option.h"
#include "Content.h"


SOCKET PeerReadyState::SetFd(Peer * p, const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp)
{
	Trace("PeerReadyState::SetFd");

	size_t idx;
	while(p->pending_queue_.size() < Option::Instance()->Max_Concurrent_Request())// we can send more request
	{
		if(-1 == Content::Instance()->Fetch_Piece(Option::Instance()->Movie_ID(), p->bitmap_, idx))
			break;

		p->Send_Request(idx);
	}

	
	FD_SET(p->sock_, &rfdp);
	if(p->send_buffer_.Count() > 0)
		FD_SET(p->sock_, &wfdp);

	return p->sock_;
}

void PeerReadyState::CheckFd(Peer * p, fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds)
{
	Trace("PeerReadyState::CheckFd");

	if(FD_ISSET(p->sock_, &wfdp))// I can write now
	{
		FD_CLR(p->sock_, &wfdp);
		--nfds;

		if(p->send_buffer_.FlushOut(p->sock_) < 0)
		{
			//std::cout << "send data error" << std::endl;
			p->Reset();
			return;
		}
	}
	else if(FD_ISSET(p->sock_, &rfdp)) // I can read now
	{
		FD_CLR(p->sock_, &rfdp);
		--nfds;

		int r = p->receive_buffer_.FeedIn(p->sock_);

		if(-1 == r || -3 == r)
		{
			//std::cout << "read from socket error or no more buffer" << std::endl;
			p->Reset();
			return;
		}

		// r is the number of data in the underlying buffer
		if(p->receive_buffer_.BasePointer()[0] == MOVIE_ID ||
			p->receive_buffer_.BasePointer()[0] == _BITMAP)
		{
			//std::cout << "We should never receive bitmap or movie id again" << std::endl;
			p->Reset();
			return;
		}

		if(p->DecodeMessage() < 0)
		{
			//std::cout << "decode message error" << std::endl;
			p->Reset();
			return;
		}

		if(-2 == r)// remote peer closed the connection
		{
			p->Reset();
			return;
		}
	}
}



