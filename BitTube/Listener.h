

#ifndef __LISTENER_H__
#define __LISTENER_H__


#include <Winsock2.h>

class Listener
{
public:
	static Listener * Instance(void);
	SOCKET SetFd(const time_t & pnow, fd_set & rfdp, fd_set & wfdp);
	void CheckFd(fd_set & rfdp, fd_set & wfdp, int & nfds);
	void Accepter(void);

protected:
	Listener(void);

private:
	SOCKET listen_sock_;

	static Listener * instance_;
};



#endif

