

#include "LocalServer.h"
#include "Trace.h"
#include "Content.h"
#include "Option.h"
#include <string>
#include <sstream>

LocalServer * LocalServer::instance_ = NULL;


LocalServer * LocalServer::Instance(void)
{
	Trace("LocalServer::Instance");

	if(NULL == LocalServer::instance_)
		LocalServer::instance_ = new LocalServer;

	return LocalServer::instance_;
}


LocalServer::LocalServer(void) :
sock_(INVALID_SOCKET),
lis_sock_(INVALID_SOCKET),
nextidx_(0)
{
	Trace("LocalServer::LocalServer");

	memset(&(this->addr_), 0, sizeof(sockaddr_in));
	this->addr_.sin_family = AF_INET;
	this->addr_.sin_port = htons(8081);
	this->addr_.sin_addr.s_addr = inet_addr("127.0.0.1");

	this->lis_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == this->lis_sock_)
	{
		throw "socket() error";
	}

	if(bind(this->lis_sock_,(struct sockaddr*)&(this->addr_),sizeof(struct sockaddr_in)) != 0)
	{
		////std::cout << WSAGetLastError() << std::endl;
		throw "could not bind on the specific port";
	}

	if(listen(this->lis_sock_, 5) == -1)
	{
		closesocket(this->lis_sock_);
		throw "listen() error";
	}
}



bool LocalServer::WaitForRequest(void)
{
	Trace("LocalServer::WaitForRequest");


	this->sock_ = accept(this->lis_sock_, NULL, NULL);
	if(INVALID_SOCKET == this->sock_)
	{
		closesocket(this->sock_);
		return false;
	}

	return this->GetRequest();
}




bool LocalServer::GetRequest(void)
{
	char buf[1024];
	int bytesRecv;
	do
	{
		if((bytesRecv = recv(this->sock_, buf, 1024, 0)) <= 0)
		{
			//std::cout << "connection closed or error" << std::endl;
			closesocket(this->sock_);
			return false;
		}
		buf[bytesRecv] = 0;
		//std::cout << buf << std::endl;
	}
	while(!this->CheckRequest(buf, bytesRecv));

	return true;
}

// return:
// true : received a request for video file
// false : other requests
bool LocalServer::CheckRequest(const char * buf, int len)
{
	std::string str(buf, len);
	if(str.find("crossdomain") != std::string::npos)// request for crossdomain.xml
	{
		this->SendXMLHead(201);
		this->SendCrossdomainFile();
		return false;
	}
	else if(str.find("client_info") != std::string::npos)
	{
		this->SendPlainHead(strlen("my_text=ok"));
		this->SendClientInfo();
		return false;
	}
	else if(str.find(".torrent") != std::string::npos)
	{
		size_t pos = str.find(".torrent");
		size_t i = pos;
		while(str[i] != '%')
			--i;
		i += 3;

		std::string id = str.substr(i, pos - i);
		
		this->GetFileLength(id);
		unsigned long val = 1;
		if(ioctlsocket(this->sock_, FIONBIO, &val) < 0)
		{
			closesocket(this->sock_);
			throw "set sock_ to non-block mode error!";
		}
		return true;
	}
	else
		return false;
}

void LocalServer::SendClientInfo(void)
{
	send(this->sock_, "my_text=ok", strlen("my_text=ok"), 0);
//	//std::cout << strlen("my_text=ok") << std::endl;
}

void LocalServer::SendXMLHead(int len)
{
	std::string buf;
	char length[32];
	_snprintf(length, 32, "%d\r\n", len);

	buf += "HTTP/1.1 200 OK\r\n";
	buf += "Content-Length: ";
	buf += length;
	buf += "Connection: keep-alive\r\n";
	buf += "Content-Type: text/xml\r\n\r\n";


	send(this->sock_, buf.c_str(), buf.size(), 0);
//	//std::cout << buf << std::endl;
}

void LocalServer::SendPlainHead(int len)
{
	std::string buf;
	char length[32];
	_snprintf(length, 32, "%d\r\n", len);

	buf += "HTTP/1.1 200 OK\r\n";
	buf += "Content-Length: ";
	buf += length;
	buf += "Connection: keep-alive\r\n";
	buf += "Content-Type: text/plain; charset=UTF-8\r\n\r\n";


	send(this->sock_, buf.c_str(), buf.size(), 0);
//	//std::cout << buf << std::endl;
}

void LocalServer::SendFLVHead(int len)
{
	std::string buf;
	char length[32];
	_snprintf(length, 32, "%d\r\n", len);

	buf += "HTTP/1.1 200 OK\r\n";
	buf += "Content-Length: ";
	buf += length;
	buf += "Connection: keep-alive\r\n";
	buf += "Content-Type: video/x-flv\r\n\r\n";


	send(this->sock_, buf.c_str(), buf.size(), 0);
//	//std::cout << buf << std::endl;
}


void LocalServer::SendCrossdomainFile(void)
{
	const char * pf = "<\?xml version=\"1.0\"\?>\n<!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\">\n<cross-domain-policy>\n   <allow-access-from domain=\"*\" />\n</cross-domain-policy>";
	send(this->sock_, pf, strlen(pf), 0);
//	//std::cout << strlen(pf) << std::endl;
}

void LocalServer::GetFileLength(const std::string & id)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Option::Instance()->Server_Port());
	addr.sin_addr.s_addr = inet_addr(Option::Instance()->Server_Address());

	

	if(addr.sin_addr.s_addr == INADDR_NONE)// Server_Address() returns domain name
	{
		struct hostent * ph = gethostbyname(Option::Instance()->Server_Address());
		if(!ph || ph->h_addrtype != AF_INET)
		{
			memset(&(addr), 0, sizeof(struct sockaddr_in));
			throw "invalid server address";
		}
//////std::cout << Option::Instance()->Server_Address() << Option::Instance()->Server_Port() << std::endl;
		memcpy(&(addr.sin_addr), ph->h_addr_list[0], sizeof(struct in_addr));
	}


	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == sock)
	{
		throw "socket() error";
	}

	if(connect(sock, (struct sockaddr *)&(addr), sizeof(struct sockaddr)) < 0)
		throw "connect to video server error";


	const char * fmt = "HEAD /video_files/%s.flv HTTP/1.1\r\nHost:%s\r\n\r\n";
	char buf[1024];
	int len = _snprintf(buf, 1024, fmt, id.c_str(), Option::Instance()->Server_Address());
	send(sock, buf, len, 0);

	//std::cout << buf << std::endl;
	len = recv(sock, buf, 1024, 0);
	buf[len] = 0;
	//std::cout << buf << std::endl;
	std::string str(buf, len);
	size_t pos;
	if((pos = str.find("Content-Length")) == std::string::npos)
		throw "get file length error";

	pos += 15;
	std::string s = str.substr(pos);
	std::istringstream is(s);
	size_t l;
	is >> l;
	//std::cout << "len " << l << std::endl;
	Option::Instance()->Movie_ID(id);
	Option::Instance()->File_Size(l);

	this->SendFLVHead(l);
}


SOCKET LocalServer::SetFd(const time_t &, fd_set &, fd_set & wfdp, fd_set &)
{
	Trace("LocalServer::SetFd");

	if(this->nextidx_ < Content::Instance()->Piece_Number(Option::Instance()->Movie_ID()) &&
		Content::Instance()->DoIHave(this->nextidx_))
	{
		FD_SET(this->sock_, &wfdp);
		return this->sock_;
	}

	if(this->sending_buffer_.Count() > 0)
	{
		FD_SET(this->sock_, &wfdp);
		return this->sock_;
	}

	return INVALID_SOCKET;
}


void LocalServer::CheckFd(fd_set &, fd_set & wfdp, fd_set &, int & nfds)
{
	Trace("LocalServer::CheckFd");


	if(!FD_ISSET(this->sock_, &wfdp))
		return;

	FD_CLR(this->sock_, &wfdp);
	--nfds;

	if(this->sending_buffer_.Count() > 0)
	{
		this->sending_buffer_.FlushOut(this->sock_);
		return;
	}

	while(this->nextidx_ < Content::Instance()->Piece_Number(Option::Instance()->Movie_ID()) &&
		Content::Instance()->DoIHave(this->nextidx_))
	{
		++(this->nextidx_);
		if(this->SendPiece(this->nextidx_ - 1))
			break;
	}
}


bool LocalServer::SendPiece(size_t idx)
{
	const unsigned char * buf;
	if(-1 == Content::Instance()->Get_Piece(Option::Instance()->Movie_ID(),
		idx, buf))
		throw "get piece error";
	//std::cout << idx << " " << std::endl;
	return this->sending_buffer_.PutFlush(this->sock_, (char *)buf,
		Content::Instance()->Piece_Size(Option::Instance()->Movie_ID(), idx)) > 0;
}



bool LocalServer::Finished(void)
{
	return this->nextidx_ == Content::Instance()->Piece_Number(Option::Instance()->Movie_ID());
}

