
#ifndef __PEERS_H__
#define __PEERS_H__


#include <Winsock2.h>
#include <list>
#include <vector>


class Peer;

class Peers
{
public:
	static Peers * Instance(void);

	SOCKET SetFd(const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp);

	void CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds);

	// return the number of peers
	size_t size(void);

	// can be called by both listener and peers
	void NewPeer(struct sockaddr_in addr, SOCKET sk);

	// remove the peer with address addr
	void Remove(sockaddr_in addr);

protected:
	Peers(void);
private:
	static Peers * instance_;

	std::vector<sockaddr_in> removed_;

	std::list<Peer *> peerlist_;
};

#endif

