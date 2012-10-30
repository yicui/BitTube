
#include "PeerHandshakingState.h"
#include "Peer.h"
#include "Trace.h"


SOCKET PeerHandshakingState::SetFd(Peer * p, const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp)
{
	Trace("PeerHandshakingState::SetFd");


	FD_SET(p->sock_, &rfdp);
	if(p->send_buffer_.Count() > 0)
		FD_SET(p->sock_, &wfdp);

	
	return p->sock_;
}

void PeerHandshakingState::CheckFd(Peer * p, fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds)
{
	Trace("PeerHandshakingState::CheckFd");
	
	if(FD_ISSET(p->sock_, &wfdp))// I can write now
	{//std::cout << "write data" << std::endl;
		FD_CLR(p->sock_, &wfdp);
		--nfds;
		//std::cout << "send_buffer length " << p->send_buffer_.Count() << std::endl;
		if(p->send_buffer_.FlushOut(p->sock_) < 0)
		{
			//std::cout << "send data error" << std::endl;
			p->Reset();
			return;
		}

		//std::cout << "send_buffer length " << p->send_buffer_.Count() << std::endl;
	}
	
	if(FD_ISSET(p->sock_, &rfdp)) // I can read now 
	{//std::cout << "read data" << std::endl;
		FD_CLR(p->sock_, &rfdp);
		--nfds;

		int r = p->receive_buffer_.FeedIn(p->sock_);

		//std::cout << "recevied data " << p->receive_buffer_.Count() << std::endl;
		if(r < 0)
		{
			//std::cout << r << std::endl;
			//std::cout << "read from socket error or no more buffer or remote peer closed the connection" << std::endl;
			p->Reset();
			return;
		}


		// r is the number of data in the underlying buffer
		if(p->receive_buffer_.BasePointer()[0] != MOVIE_ID &&
			p->receive_buffer_.BasePointer()[0] != _BITMAP)
		{
			//std::cout << "We should do handshaking before say other things" << std::endl;
			p->Reset();
			return;
		}
		
		if(p->DecodeMessage() < 0)
		{
			//std::cout << "decode message error" << std::endl;
			p->Reset();
			return;
		}

		if(p->bitmap_.NBit() != 1 && !(p->movie_id_.empty())) // I already got bitmap and movie id from the remote peer
			p->ChangeState(Peer::READY);
	}
}

