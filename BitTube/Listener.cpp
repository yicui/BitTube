

#include "Listener.h"
#include "Trace.h"
#include "Option.h"
#include "Peers.h"

Listener * Listener::instance_ = NULL;

Listener * Listener::Instance(void)
{
	Trace("Listener::Instace()");

	if(NULL == Listener::instance_)
	{
		Listener::instance_ = new Listener;
	}
	return Listener::instance_;
}


// open a listening socket
Listener::Listener(void)
{
	Trace("Listener::Listener()");


	struct sockaddr_in lis_addr;
	memset(&lis_addr,0, sizeof(sockaddr_in));
	lis_addr.sin_family = AF_INET;
	lis_addr.sin_addr.s_addr = INADDR_ANY;
	lis_addr.sin_port = htons(Option::Instance()->Listen_Port());

	this->listen_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(INVALID_SOCKET == this->listen_sock_)
	{
		//std::cout << WSAGetLastError() << std::endl;
		throw "socket() error";
	}

	if(bind(this->listen_sock_,(struct sockaddr*)&lis_addr,sizeof(struct sockaddr_in)) != 0)
	{
		//std::cout << WSAGetLastError() << std::endl;
		throw "could not bind on the specific port";
	}
	

	if(listen(this->listen_sock_, 5) == -1)
	{
		closesocket(this->listen_sock_);
		throw "listen() error";
	}

	unsigned long val = 1;
	if(ioctlsocket(this->listen_sock_, FIONBIO, &val) < 0)
	{
		closesocket(this->listen_sock_);
		throw "could not set listening socket to non-block mode";
	}
}


/*
	if # of connected peers < max_connected_peers,
		put it into rddp
*/
SOCKET Listener::SetFd(const time_t &, fd_set & rfdp, fd_set &)
{
	Trace("Listener::SetFd");

	if(INVALID_SOCKET != this->listen_sock_ && 
		Peers::Instance()->size() < Option::Instance()->Max_Connected_Peers())
	{
		FD_SET(this->listen_sock_, &rfdp);
		return this->listen_sock_;
	}

	return INVALID_SOCKET;
}


void Listener::CheckFd(fd_set & rfdp, fd_set & wfdp, int & nfds)
{
	Trace("Listener::CheckFd");

	if( FD_ISSET(this->listen_sock_, &rfdp))
	{
		FD_CLR(this->listen_sock_, &rfdp);
		--nfds;
		Accepter();
	}
}

void Listener::Accepter(void)
{
	Trace("Listener::Accepter");

	SOCKET newsk;
	int addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in addr;
	newsk = accept(this->listen_sock_, (struct sockaddr *) & addr, & addrlen);

	if(INVALID_SOCKET == newsk ||
		AF_INET != addr.sin_family ||
		addrlen != sizeof(struct sockaddr_in))
	{//std::cout << "accept fail " << std::endl;
		closesocket(newsk);
	}
	else
	{
		//std::cout << "accept success" << std::endl;
		Peers::Instance()->NewPeer(addr, newsk);
	}
}

 

