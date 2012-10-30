

#ifndef __SERVER_H__
#define __SERVER_H__


//#include <set>
#include <Winsock2.h>
#include "Bitmap.h"
#include "bufio.h"


class Server
{
public:
	enum ServerMode {SERVER_FREE, SERVER_CONNECTING, SERVER_READY};

	static Server * Instance(void);

	SOCKET SetFd(const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp);

	void CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds);


protected:
	Server(void);

private:

	void CheckResponse(void);

	// non-block connect to server
	int Connect(void);

	// send request to server
	int SendRequest(void);

	void Reset(void);

	SOCKET sock_;

	static Server * instance_;

	ServerMode status_;

	struct sockaddr_in addr_;

	Bitmap bitmap_;

	//std::set<size_t> pending_queue_;
	int pending_queue_;

	BufIo response_buffer_;
};


#endif

