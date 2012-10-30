

#include "Peer.h"
#include "Trace.h"
#include "Option.h"
#include "Content.h"
#include "Peers.h"
#include "Stat.h"

PeerDisconnectedState Peer::disconnected_state_;
PeerConnectingState Peer::connecting_state_;
PeerHandshakingState Peer::handshaking_state_;
PeerReadyState Peer::ready_state_;



Peer::Peer(sockaddr_in addr, SOCKET sk) : bitmap_(1), sock_(sk), addr_(addr)
{
	Trace("Peer::Peer()");

	if(INVALID_SOCKET == this->sock_)// called from tracker
	{
		this->state_ = &Peer::disconnected_state_;
	}
	else// called from listener
	{
		this->state_ = &Peer::handshaking_state_;
		
		unsigned long val = 1;
		if(ioctlsocket(this->sock_, FIONBIO, &val) < 0)
		{
			closesocket(this->sock_);
			throw "set sock_ to non-block mode error!";
		}

		if(this->Send_MovieID() < 0 || this->Send_Bitmap() < 0)
		{
			closesocket(this->sock_);
			throw "send movie id or bitmap error";
		}
	}
}

void Peer::ChangeState(State state)
{
	Trace("Peer::ChangeState()");

	switch(state)
	{
	case DISCONNECTED:
		this->state_ = &Peer::disconnected_state_;
		break;
	case CONNECTING:
		this->state_ = &Peer::connecting_state_;
		break;
	case HANDSHAKING:
		this->state_ = &Peer::handshaking_state_;
		break;
	case READY:
		this->state_ = &Peer::ready_state_;
		break;
	default:
		throw "should never be here";
	}
}


SOCKET Peer::SetFd(const time_t & tt, fd_set & rfdp, fd_set & wfdp, fd_set & efdp)
{
	Trace("Peer::SetFd()");

	return this->state_->SetFd(this, tt, rfdp, wfdp, efdp);
}

void Peer::CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds)
{
	Trace("Peer::CheckFd()");

	this->state_->CheckFd(this, rfdp, wfdp, efdp, nfds);
}

void Peer::Fill(char * buf, char c, size_t size)
{
	Trace("Peer::Fill()");

	buf[0] = c;
	size_t ns = htonl(size);
	memcpy(buf + 1, &ns, sizeof(size));
}


int Peer::Send_MovieID(void)
{
	Trace("Peer::Send_MovieID()");

	//    1         4       ...
	// -----------------------------
	//| MOVIE_ID | size|  payload ...
	// -------------------------------
	char buf[256];

	size_t len = strlen(Option::Instance()->Movie_ID());
	Fill(buf, MOVIE_ID, len);
	memcpy(buf + 5, Option::Instance()->Movie_ID(), len); 

	//std::cout << "movie id length " << 5 + len << std::endl;
	//std::cout << "send a movie id message" << std::endl;
	return this->send_buffer_.Put(this->sock_, buf, 5 + len);
}



int Peer::Send_Bitmap(void)
{
	Trace("Peer::Send_Bitmap()");

	//    1        4       ...
	// -----------------------------
	//| BITMAP  | size|  payload ...
	// -------------------------------
	char buf[5];

	size_t len = Content::Instance()->Piece_Number(Option::Instance()->Movie_ID());
	Fill(buf, _BITMAP, len);

	if(this->send_buffer_.Put(this->sock_, buf, 5) < 0)
		return -1;
	
	//std::cout << "send a bitmap message" << std::endl;
	
	const unsigned char * b;

	len = Content::Instance()->Get_Bitmap(Option::Instance()->Movie_ID(), b);
	//std::cout << "bitmap length " << 5 + len << std::endl;
	return this->send_buffer_.Put(this->sock_, (char *)b, len);
}


int Peer::Send_Have(size_t idx)
{
	Trace("Peer::Send_Have()");

	//    1     4  
	// -------------
	//| HAVE | idx |
	// -------------
	char buf[5];
	Fill(buf, HAVE, idx);

	//std::cout << "send a have message" << std::endl;
	return this->send_buffer_.Put(this->sock_, buf, 5);
}


int Peer::Send_Request(size_t idx)
{
	Trace("Peer::Send_Request()");

	//    1        4  
	// ----------------
	//| REQUEST | idx |
	// ----------------
	char buf[5];
	Fill(buf, REQUEST, idx);
	this->pending_queue_.insert(idx);

	//std::cout << "send a request message" << std::endl;
	return this->send_buffer_.Put(this->sock_, buf, 5);
}


int Peer::Send_Bye(void)
{
	Trace("Peer::Send_Bye()");

	//    1        
	// ----------
	//|   BYE   |
	// ----------

	//std::cout << "send a bye message" << std::endl;
	return this->send_buffer_.Put(this->sock_, (char *)&BYE, 1);
}


int Peer::Send_Piece(size_t idx)
{
	Trace("Peer::Send_Piece()");

	//    1        4       ...
	// -----------------------------
	//| PIECE   | idx |  payload ...
	// -------------------------------
	char buf[5];
	Fill(buf, PIECE, idx);

	if(this->send_buffer_.Put(this->sock_, buf, 5) < 0)
		return -1;

	
	const unsigned char * tb;
	if(Content::Instance()->Get_Piece(Option::Instance()->Movie_ID(), idx, tb) < 0)
		return -1;

	//std::cout << "send a piece message" << std::endl;
	return this->send_buffer_.Put(this->sock_,
		(char *)tb,
		Content::Instance()->Piece_Size(Option::Instance()->Movie_ID(), idx));
}



void Peer::Reset(void)
{
	Trace("Peer::Reset()");

	for(std::set<size_t>::iterator itor = this->pending_queue_.begin();
		itor != this->pending_queue_.end();
		++itor)
	{
		Content::Instance()->Cancel_Piece(Option::Instance()->Movie_ID(), *itor);
	}

	if(INVALID_SOCKET != this->sock_)
		closesocket(this->sock_);


	Peers::Instance()->Remove(this->addr_);
}


// 0 : success
// -1 : error
int Peer::DecodeMessage(void)
{
	Trace("Peer::DecodeMessage()");

	//std::cout << "received message length " << this->receive_buffer_.Count() << std::endl;

	while(this->receive_buffer_.Count() != 0)
	{
		char command = this->receive_buffer_.BasePointer()[0];
		size_t idx, size;
		switch(command)
		{
		case MOVIE_ID:
			if(this->receive_buffer_.Count() < 1 + 4)
				return 0;
			size = *(size_t *)(this->receive_buffer_.BasePointer() + 1);
			size = ntohl(size);
			if(this->receive_buffer_.Count() < 1 + 4 + size)
				return 0;

			{
				std::string tmp(this->receive_buffer_.BasePointer() + 1 + 4, size);
				this->movie_id_ = tmp;
			}
			this->receive_buffer_.PickUp(1 + 4 + size);
			//std::cout << "received a movie_id message!" << std::endl;
			//std::cout << "buffer len " << this->receive_buffer_.Count() << std::endl;
			break;

		case _BITMAP:
			if(this->receive_buffer_.Count() < 1 + 4)
				return 0;
			size = *(size_t *)(this->receive_buffer_.BasePointer() + 1);
			size = ntohl(size);
			if(this->receive_buffer_.Count() < 1 + 4 + (size % 8 ? size / 8 + 1 : size / 8))
				return 0;

			{
				Bitmap bt(size, (unsigned char *)this->receive_buffer_.BasePointer() + 1 + 4);
				this->bitmap_ = bt;
			}
			this->receive_buffer_.PickUp(1 + 4 + (size % 8 ? size / 8 + 1 : size / 8));
			//std::cout << "received a bitmap message!" << std::endl;
			//std::cout << "buffer len " << this->receive_buffer_.Count() << std::endl;
			break;

		case HAVE:
			if(this->receive_buffer_.Count() < 1 + 4)
				return 0;
			idx = *(size_t *)(this->receive_buffer_.BasePointer() + 1);
			idx = ntohl(idx);

			this->bitmap_.Set(idx);
			this->receive_buffer_.PickUp(1 + 4);
			//std::cout << "received a have message!" << std::endl;
			//std::cout << "buffer len " << this->receive_buffer_.Count() << std::endl;
			break;

		case REQUEST:
			if(this->receive_buffer_.Count() < 1 + 4)
				return 0;
			idx = *(size_t *)(this->receive_buffer_.BasePointer() + 1);
			idx = ntohl(idx);

			const unsigned char * buf;
			if(0 == Content::Instance()->Get_Piece(Option::Instance()->Movie_ID(), idx, buf))// I have this piece
			{
			/*	if(this->send_buffer_.PutFlush(this->sock_,
					(char *)buf,
					Content::Instance()->Piece_Size(Option::Instance()->Movie_ID(), idx)) < 0)
					return -1;
					*/
				this->Send_Piece(idx);
			}
			else
			{
				//std::cout << "I have no this piece" << std::endl;
			}
			this->receive_buffer_.PickUp(1 + 4);
			//std::cout << "received a request message!" << std::endl;
			break;

		case BYE:
			//std::cout << "received a bye message!" << std::endl;
			return -1;
			break;

		case PIECE:
			if(this->receive_buffer_.Count() < 1 + 4)
				return 0;
			idx = *(size_t *)(this->receive_buffer_.BasePointer() + 1);
			idx = ntohl(idx);

			{
				size_t piece_len = Content::Instance()->Piece_Size(Option::Instance()->Movie_ID(), idx);
				if(this->receive_buffer_.Count() < 1 + 4 + piece_len)
					return 0;

				Content::Instance()->Put_Piece(Option::Instance()->Movie_ID(),
					idx,
					(unsigned char *)(this->receive_buffer_.BasePointer() + 1 + 4));
				Stat::Instance()->ByteFromPeers(Content::Instance()->Piece_Size(Option::Instance()->Movie_ID(), idx));

				this->receive_buffer_.PickUp(1 + 4 + piece_len);

				//std::cout << "==================================================================" << std::endl;
				this->pending_queue_.erase(idx);
			}

			//std::cout << "received a piece message!" << std::endl;
			break;
		default:
			//std::cout << "heeeeee" << std::endl;
			//std::cout << int(command) << std::endl;
			return -1;
		}
	}

	return 0;
}



bool Peer::IsMe(sockaddr_in addr)
{
	Trace("Peer::IsMe");

	return memcmp(&(this->addr_.sin_addr), &addr.sin_addr, sizeof(struct in_addr)) == 0;
	//return memcmp(&(this->addr_), &addr, sizeof(struct sockaddr_in)) == 0;
}

