
#ifndef __LOCALSERVER_H__
#define __LOCALSERVER_H__

#include <Winsock2.h>
#include <string>
#include "bufio.h"


class LocalServer
{
public:
	static LocalServer * Instance(void);

	/*
		wait for http request from flash player
		true : success
		false : failed
	*/
	bool WaitForRequest(void);

	SOCKET SetFd(const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp);

	void CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds);


	bool Finished(void);

protected:
	LocalServer(void);

private:

	// get and check GET request from flash player
	// true : success
	// false : failed
	bool GetRequest(void);

	// return:
	// true : received a request for video file
	// false : other requests
	bool CheckRequest(const char * buf, int len);


	bool SendPiece(size_t idx);


	void SendXMLHead(int len);

	void SendPlainHead(int len);

	void SendFLVHead(int len);

	void SendClientInfo(void);

	void SendCrossdomainFile(void);

	void GetFileLength(const std::string & id);

	static LocalServer * instance_;

	// the socket handle to flash player
	SOCKET sock_;

	// the listening socket handle
	SOCKET lis_sock_;

	// the next piece idx I am gonna to send to flash player
	size_t nextidx_;

	// the outgoing buffer
	BufIo sending_buffer_;


	// the local http server address
	sockaddr_in addr_;
};

#endif


