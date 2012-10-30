

#include "Server.h"
#include "Trace.h"
#include "Option.h"
#include "Content.h"
#include "httpencode.h"
#include "Stat.h"


Server * Server::instance_ = NULL;


Server * Server::Instance(void)
{
	Trace("Server::Instance()");

	if(NULL == Server::instance_)
		Server::instance_ = new Server;


	return Server::instance_;
}


Server::Server(void) :
sock_(INVALID_SOCKET),
status_(SERVER_FREE),
bitmap_((Option::Instance()->File_Size() + Option::Instance()->Piece_Size() - 1) / Option::Instance()->Piece_Size()),
pending_queue_(-1)
{
	Trace("Server::Server()");

	//memset(this->bitmap_.Buf(), 0xff, this->bitmap_.NBytes());
	for(size_t i = 0; i < this->bitmap_.NBit(); ++i)
		this->bitmap_.Set(i);

	memset(&(this->addr_), 0, sizeof(sockaddr_in));
	this->addr_.sin_family = AF_INET;
	this->addr_.sin_port = htons(Option::Instance()->Server_Port());
	this->addr_.sin_addr.s_addr = inet_addr(Option::Instance()->Server_Address());

	

	if(this->addr_.sin_addr.s_addr == INADDR_NONE)// Server_Address() returns domain name
	{
		struct hostent * ph = gethostbyname(Option::Instance()->Server_Address());
		if(!ph || ph->h_addrtype != AF_INET)
		{
			memset(&(this->addr_), 0, sizeof(struct sockaddr_in));
			throw "invalid server address";
		}
////std::cout << Option::Instance()->Server_Address() << Option::Instance()->Server_Port() << std::endl;
		memcpy(&(this->addr_.sin_addr), ph->h_addr_list[0], sizeof(struct in_addr));
	}
}


SOCKET Server::SetFd(const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp)
{
	Trace("Server::SetFd()");

	if(Content::Instance()->Full())
		return INVALID_SOCKET;

	if(SERVER_FREE == this->status_)
	{
		if(this->Connect() < 0)
		{
			//std::cout << "connect to server error!" << std::endl;
			return INVALID_SOCKET;
		}


		if(SERVER_READY == this->status_)
		{
			FD_SET(this->sock_, &rfdp);
			//FD_SET(this->sock_, &wfdp);
		}
		else if(SERVER_CONNECTING == this->status_)// SERVER_CONNECTING
		{
			FD_SET(this->sock_, &efdp);
			FD_SET(this->sock_, &wfdp);
		}
		else // SERVER_FREE
		{
			return INVALID_SOCKET;
		}
	}
	else if(SERVER_CONNECTING == this->status_)
	{
		FD_SET(this->sock_, &efdp);
		FD_SET(this->sock_, &wfdp);
	}
	else // SERVER_READY == this->status_
	{
		if(-1 == this->pending_queue_)
		{
			FD_SET(this->sock_, &wfdp);
		}
		if(-1 != this->pending_queue_)
		{
			FD_SET(this->sock_, &rfdp);
		}
	}

	return this->sock_;
}

void Server::CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds)
{
	Trace("Server::CheckFd()");

	if(SERVER_FREE == this->status_)
	{
		return;
	}
	else if(SERVER_CONNECTING == this->status_)
	{
		if(FD_ISSET(this->sock_, &wfdp))// connect success
		{
			FD_CLR(this->sock_, &wfdp);
			--nfds;

			if(0 != this->SendRequest())// send request failed
			{
				this->Reset();
			}
			else
			{
				this->status_ = SERVER_READY;
			}
		}
		else if(FD_ISSET(this->sock_, &efdp))// connect fail
		{
			FD_CLR(this->sock_, &efdp);
			--nfds;
			this->Reset();
			//std::cout << "connect to server failed!" << std::endl;
		}
	}
	else // SERVER_READY == this->status_
	{
		if(FD_ISSET(this->sock_, &rfdp))
		{
			FD_CLR(this->sock_, &rfdp);
			--nfds;
			this->CheckResponse();
		}

		if(FD_ISSET(this->sock_, &wfdp))
		{	
			FD_CLR(this->sock_, &wfdp);
			--nfds;
			if(0 != this->SendRequest())
			{
				this->Reset();
			}
		}
	}
}



/*
	non-block connect to tracker
	1. non-block connect to tracker
	2. if connect finished, call SendRequest(), and set this->status_ = SERVER_READY
	3. if connect in progress, this->status_ = SERVER_CONNECTING
	retval :
	0 : success
	-1 : failed
*/
int Server::Connect(void)
{
	Trace("Server::Connect()");

	//std::cout << "Connect" << std::endl;
	this->sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == this->sock_)
	{
		throw "socket() error";
	}

	unsigned long val = 1;
	if(ioctlsocket(this->sock_, FIONBIO, &val) < 0)
	{
		closesocket(this->sock_);
		throw "set sock_ to non-block mode error!";
	}

	int r = connect(this->sock_, (struct sockaddr *)&(this->addr_), sizeof(struct sockaddr));
	////std::cout << r << std::endl;
	////std::cout << WSAGetLastError() << std::endl;
	if(r < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
		r = -2;

	if(0 == r)// success
	{
		if(0 != this->SendRequest())// send request failed
		{
			this->Reset();
			return -1;
		}
		else
		{
			this->status_ = SERVER_READY;
			return 0;
		}
	}
	else if(-1 == r)// failed
	{
		this->Reset();
		return -1;
	}
	else// if(-2 == r)// in progress
	{
		this->status_ = SERVER_CONNECTING;
		return 0;
	}
}



/*
	send request to server
	0 : success
	-1 : fail
*/
int Server::SendRequest(void)
{
	Trace("Server::SendRequest()");


	const char * fmt = "GET /video_files/%s.flv HTTP/1.1\r\nHost:%s\r\nConnection: Keep-Alive\r\nRange:bytes=%d-%d\r\n\r\n";
	char buf[1024];
	size_t idx;
	
//	while(this->pending_queue_.size() < Option::Instance()->Max_Server_Concurrent())
//	{
		if(-1 == Content::Instance()->Fetch_Piece(Option::Instance()->Movie_ID(), this->bitmap_, idx))
			return 0;

		////std::cout << idx << std::endl;
		size_t range_end;
		if((idx + 1) * Option::Instance()->Piece_Size() > Option::Instance()->File_Size())
			range_end = Option::Instance()->File_Size() - 1;
		else
			range_end = (idx + 1) * Option::Instance()->Piece_Size() - 1;

		
		if(1024 < _snprintf(buf, 1024, fmt,
		Option::Instance()->Movie_ID(),
		Option::Instance()->Server_Address(),
		idx * Option::Instance()->Piece_Size(),
		range_end))
		{
			//std::cout << "_snprintf error!" << std::endl;
			return 0;
		}

		////std::cout << idx << std::endl;
		int r = send(this->sock_, buf, int(strlen((char *)buf)), 0);
		if(strlen((char *)buf) != r)
		{
			//std::cout << "send() error!" << std::endl;
			return -1;
		}

//		this->pending_queue_.insert(idx);
		this->pending_queue_ = idx;
//	}
	

	return 0;
}

void Server::Reset(void)
{
	Trace("Server::Reset()");

	if(INVALID_SOCKET != this->sock_)
	{
		closesocket(this->sock_);
		this->sock_ = INVALID_SOCKET;
	}

	this->response_buffer_.Reset();
	this->status_ = SERVER_FREE;

	if(-1 != this->pending_queue_)
	{
		Content::Instance()->Cancel_Piece(Option::Instance()->Movie_ID(), this->pending_queue_);
		this->pending_queue_ = -1;
	}
	/*
	for(std::set<size_t>::iterator itor = this->pending_queue_.begin();
		itor != this->pending_queue_.end();
		++itor)
	{
		Content::Instance()->Cancel_Piece(Option::Instance()->Movie_ID, *itor);
	}
	this->pending_queue_.clear();
	*/
}



void Server::CheckResponse(void)
{
	Trace("Server::CheckResponse()");


	ssize_t r;

	r = this->response_buffer_.FeedIn(this->sock_);
	////std::cout << std::string(this->response_buffer_.BasePointer(), this->response_buffer_.Count()) << std::endl;

	if(-1 == r)// read error
	{		
		this->Reset();
		//std::cout << WSAGetLastError() << std::endl;
		//std::cout << "read error" << std::endl;
		return;
	}

	
	
	
	if(-3 == r)// no more buffer
	{
		this->Reset();
		//std::cout << "no more buffer" << std::endl;
		return;
	}


	if(0 == this->response_buffer_.Count())
	{
		this->Reset();
		//std::cout << "zero buffer" << std::endl;
		return;
	}


	// -2 == r || r > 0

	
	char *pdata;
	size_t q, hlen, dlen, code;


	q = this->response_buffer_.Count();
	hlen = Http_split(this->response_buffer_.BasePointer(), q, &pdata,&dlen);

	
	if(0 == hlen)
	{
		//std::cout << "No head found!" << std::endl;
		this->Reset();
		return;
	}

	code = Http_reponse_code(this->response_buffer_.BasePointer(), hlen);
	
	
	if(200 != code && 206 != code)
	{
		//std::cout << "Response code is not 200 or 206" << std::endl;
		this->Reset();
		return;
	}

	if(NULL == pdata)
	{
		//std::cout << "Null data" << std::endl;
		this->Reset();
		return;
	}

	char length[256];
	if( Http_get_header(this->response_buffer_.BasePointer(), hlen, "Content-Length", length) < 0 )
	{
		//std::cout << "No Content-Length" << std::endl;
		this->Reset();
		return;
	}

	int len = atoi(length);

	if(hlen + 4 + len > this->response_buffer_.Count())// we have not got the entire piece
	{
	//	//std::cout << "not enough" << std::endl;
		return;
	}
	else // we already got the entire piece
	{//[pdata, pdata+ len]
		Content::Instance()->Put_Piece(Option::Instance()->Movie_ID(), this->pending_queue_, (unsigned char *)pdata);
		Stat::Instance()->ByteFromServer(Content::Instance()->Piece_Size(Option::Instance()->Movie_ID(),
			this->pending_queue_));
		this->pending_queue_ = -1;
		this->response_buffer_.PickUp(hlen + 4 + len);
		//std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
	}

	if(-2 == r)
	{
	//	//std::cout << "Done" << std::endl;
		this->Reset();
	}
	
}




