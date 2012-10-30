

#ifndef __TRACKER_H__
#define __TRACKER_H__

#include <Winsock2.h>
#include "bufio.h"

class Tracker
{
public:
	enum TrackerMode {TRACKER_FREE, TRACKER_CONNECTING, TRACKER_READY};

	static Tracker * Instance(void);

	SOCKET SetFd(const time_t & pnow, fd_set & rfdp, fd_set & wfdp, fd_set & efdp);

	void CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds);

	void Bye(void);

	

protected:
	Tracker(void);

private:
	void CheckResponse(void);

	// non-block connect to tracker
	int Connect(void);

	// send request to tracker
	int SendRequest(void);

	void Reset(void);

	SOCKET sock_;

	static Tracker * instance_;

	time_t last_time_;

	TrackerMode status_;

	struct sockaddr_in addr_;

	BufIo response_buffer_;
};



#endif

