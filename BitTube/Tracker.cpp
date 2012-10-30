

#include "Trace.h"
#include "Tracker.h"
#include "Option.h"
#include "Peers.h"
#include "httpencode.h"
#include <iostream>
#include <string>
#include <sstream>


const char GET = 1;
const char BYE = 2;
Tracker * Tracker::instance_ = NULL;


Tracker * Tracker::Instance(void)
{
	Trace("Tracker::Instance()");

	if(NULL == Tracker::instance_)
		Tracker::instance_ = new Tracker;

	return Tracker::instance_;
}

Tracker::Tracker(void) :
sock_(INVALID_SOCKET),
last_time_(0),
status_(TRACKER_FREE)
{
	Trace("Tracker::Tracker()");

	memset(&(this->addr_), 0, sizeof(sockaddr_in));
	this->addr_.sin_family = AF_INET;
	this->addr_.sin_port = htons(Option::Instance()->Tracker_Port());
	this->addr_.sin_addr.s_addr = inet_addr(Option::Instance()->Tracker_Address());

	

	if(this->addr_.sin_addr.s_addr == INADDR_NONE)// Tracker_Address() returns domain name
	{
		struct hostent * ph = gethostbyname(Option::Instance()->Tracker_Address());
		if(!ph || ph->h_addrtype != AF_INET)
		{
			memset(&(this->addr_), 0, sizeof(struct sockaddr_in));
			throw "invalid tracker address";
		}
////std::cout << Option::Instance()->Tracker_Address() << Option::Instance()->Tracker_Port() << std::endl;
		memcpy(&(this->addr_.sin_addr), ph->h_addr_list[0], sizeof(struct in_addr));
	}
}



// if m_status == T_CONNECTING : put m_sock in both rdset and wrset
// if m_status == T_READY : put m_sock in rdset
// if m_status == T_FREE : connect the tracker by calling Conenct(), and put m_sock in proper descriptor set
// retval : 
// INVALID_SOCKET : error
// this->sock_ : success
SOCKET Tracker::SetFd(const time_t & now, fd_set & rfdp, fd_set & wfdp, fd_set & efdp)
{
	Trace("Tracker::SetFd()");

	if(TRACKER_FREE == this->status_)
	{
		if(now - this->last_time_ >= Option::Instance()->Tracker_Interval() &&
			Peers::Instance()->size() < Option::Instance()->Max_Connected_Peers())
		{
			if(this->Connect() < 0)
			{
				//std::cout << "connect to tracker error!" << std::endl;
				return INVALID_SOCKET;
			}


			if(TRACKER_READY == this->status_)
			{
				FD_SET(this->sock_, &rfdp);
			}
			else if(TRACKER_CONNECTING == this->status_)// TRACKER_CONNECTING
			{
				FD_SET(this->sock_, &efdp);
				FD_SET(this->sock_, &wfdp);
			}
			else // TRACKER_FREE
			{
				return INVALID_SOCKET;
			}
		}
	}
	else if(TRACKER_CONNECTING == this->status_)
	{
		FD_SET(this->sock_, &efdp);
		FD_SET(this->sock_, &wfdp);
	}
	else// TRACKER_READY
	{
		FD_SET(this->sock_, &rfdp);
	}

	return this->sock_;
}

/*
*/
void Tracker::CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds)
{
	Trace("Tracker::CheckFd()");

	if(TRACKER_FREE == this->status_)
	{
		return;
	}
	else if(TRACKER_CONNECTING == this->status_)
	{////std::cout << 1 << std::endl;
		//success is reported in the writefds set and failure is reported in the exceptfds set. 
		if(FD_ISSET(this->sock_, &wfdp))// connect success
		{////std::cout << 2 << std::endl;
			FD_CLR(this->sock_, &wfdp);
			--nfds;

			if(0 != this->SendRequest())// send request failed
			{
				closesocket(this->sock_);
				this->status_ = TRACKER_FREE;
			}
			else
			{
				this->status_ = TRACKER_READY;
			}
		}
		else if(FD_ISSET(this->sock_, &efdp))// connect fail
		{////std::cout << 3 << std::endl;
			FD_CLR(this->sock_, &efdp);
			--nfds;
			this->status_ = TRACKER_FREE;
			closesocket(this->sock_);
			//std::cout << "connect to tracker failed!" << std::endl;
		}
	}
	else// TRACKER_READY == this->status_
	{
		if(FD_ISSET(this->sock_, &rfdp))
		{
			FD_CLR(this->sock_, &rfdp);
			--nfds;
			this->CheckResponse();
		}
	}
}


/*
	non-block connect to tracker
	1. update m_last_timestamp
	2. non-block connect to tracker
	3. if connect finished, call SendRequest(), and set m_status = TRACKER_READY
	4. if connect in progress, m_status = TRACKER_CONNECTING
	retval :
	0 : success
	-1 : failed
*/
int Tracker::Connect(void)
{
	Trace("Tracker::Connect()");


	time(&(this->last_time_));

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

	int r = connect(this->sock_, (struct sockaddr *)&(this->addr_), sizeof(this->addr_));
	////std::cout << r << std::endl;
	////std::cout << WSAGetLastError() << std::endl;
	if(r < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
		r = -2;

	if(0 == r)// success
	{
		if(0 != this->SendRequest())// send request failed
		{
			closesocket(this->sock_);
			this->status_ = TRACKER_FREE;
			return -1;
		}
		else
		{
			this->status_ = TRACKER_READY;
			return 0;
		}
	}
	else if(-1 == r)// failed
	{
		closesocket(this->sock_);
		this->status_ = TRACKER_FREE;
		return -1;
	}
	else// if(-2 == r)// in progress
	{
		this->status_ = TRACKER_CONNECTING;
		return 0;
	}

}

/*
	send request to tracker
	0 : success
	-1 : fail
*/
int Tracker::SendRequest(void)
{
	Trace("Tracker::SendRequest()");
	//   1     4        ....
	//----------------------------------
	//| GET | size | movie_id addr port|
	//-----------------------------------

	const char * fmt = "%s %s %d";
	char buf[256];
	buf[0] = GET;
	size_t len;

	if(256 - 1 - 4 < (len = _snprintf(buf + 1 + 4, 256 - 1 - 4, fmt,
		Option::Instance()->Movie_ID(),
		Option::Instance()->Local_Addr(),
		Option::Instance()->Listen_Port())))
	{
		//std::cout << "_snprintf error!" << std::endl;
		return -1;
	}

	len = htonl(len);
	memcpy(buf + 1, &len, sizeof(len));

	
	int r = send(this->sock_, buf, 1 + 4 + ntohl(len), 0);
	if(1 + 4 + ntohl(len) != r)
	{
		//std::cout << "send() error!" << std::endl;
		return -1;
	}

	return 0;
}

void Tracker::Reset(void)
{
	Trace("Tracker::Reset()");

	closesocket(this->sock_);
	this->response_buffer_.Reset();
	time(&this->last_time_);
	this->status_ = TRACKER_FREE;
}

void Tracker::CheckResponse(void)
{
	Trace("Tracker::CheckResponse()");


	ssize_t r;

	r = this->response_buffer_.FeedIn(this->sock_);
	////std::cout << std::string(this->response_buffer_.BasePointer(), this->response_buffer_.Count()) << std::endl;

	if( r > 0 )
		return;

	if(-1 == r || -3 == r)// read error or no more buffer
	{
		if(INVALID_SOCKET != this->sock_)
		{
			closesocket(this->sock_);
			this->sock_ = INVALID_SOCKET;
		}
		
		this->Reset();

		return;
	}

	
	// -2 == r
	// the tracker close the connection

	if(0 == this->response_buffer_.Count())
	{
		this->Reset();
		return;
	}

/*	
	char *pdata;
	size_t q, hlen, dlen;
	q = this->response_buffer_.Count();
	hlen = Http_split(this->response_buffer_.BasePointer(), q, &pdata,&dlen);

	
	if(0 == hlen)
	{
		//std::cout << "No head found!" << std::endl;
		this->Reset();
		return;
	}

	r = Http_reponse_code(this->response_buffer_.BasePointer(), hlen);

	
	if(200 != r)
	{
		//std::cout << "Response code is not 200" << std::endl;
		this->Reset();
		return;
	}
	

	if(NULL == pdata)
	{
		this->Reset();
		return;
	}
*/
//	std::string str(pdata, dlen);
	std::string str(this->response_buffer_.BasePointer(), this->response_buffer_.Count());
	std::istringstream is(str);
	short port;
	struct sockaddr_in addr;
	////std::cout << str << std::endl;
	while(is >> str >> port)
	{
		memset(&addr,0, sizeof(sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(str.c_str());
		addr.sin_port = htons(port);

		if(addr.sin_addr.s_addr == INADDR_NONE)// Tracker returns domain name
		{
			struct hostent * ph = gethostbyname(str.c_str());
			if(!ph || ph->h_addrtype != AF_INET)
			{
				this->Reset();
				//std::cout << "invalid address" << std::endl;
				return;
			}

			memcpy(&(addr.sin_addr), ph->h_addr_list[0], sizeof(struct in_addr));
		}

		//std::cout << str << " " << port << std::endl;
		Peers::Instance()->NewPeer(addr, INVALID_SOCKET);
	}

	this->Reset();
}


void Tracker::Bye(void)
{
	Trace("Tracker::Bye()");
	//   1     4        ....
	//----------------------------------
	//| BYE | size | addr port|
	//----------------------------------


	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == this->sock_)
	{
		throw "socket() error";
	}

	int r = connect(sock, (struct sockaddr *)&(this->addr_), sizeof(struct sockaddr));

	if(0 == r) // connect success
	{
		const char * fmt = "%s %d";
		char buf[256];
		buf[0] = BYE;
		size_t len;

		if(256 - 1 - 4 < (len = _snprintf(buf + 1 + 4, 256 - 1 - 4, fmt,
		Option::Instance()->Local_Addr(),
		Option::Instance()->Listen_Port())))
		{
			//std::cout << "_snprintf error!" << std::endl;
			return;
		}

		len = htonl(len);
		memcpy(buf + 1, &len, sizeof(len));

		send(sock, buf, 1 + 4 + ntohl(len), 0);

		closesocket(sock);
	}
}

