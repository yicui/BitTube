
#include "Trace.h"
#include "Peers.h"
#include "Peer.h"
#include "Option.h"

Peers * Peers::instance_ = NULL;


Peers * Peers::Instance(void)
{
	Trace("Peers::Instace()");

	if(NULL == Peers::instance_)
	{
		Peers::instance_ = new Peers;
	}
	return Peers::instance_;
}


Peers::Peers(void)
{
	Trace("Peers::Peers()");
}


size_t Peers::size(void)
{
	Trace("Peers::size()");
	return this->peerlist_.size();
}


void Peers::NewPeer(struct sockaddr_in addr, SOCKET sk)
{
	Trace("Peers::NewPeer()");

	if(this->peerlist_.size() >= Option::Instance()->Max_Connected_Peers())
	{
		if(INVALID_SOCKET != sk)
			closesocket(sk);
		return;
	}

	for(std::list<Peer *>::iterator itor = this->peerlist_.begin();
		itor != this->peerlist_.end();
		++itor)
	{
		if((*itor)->IsMe(addr))
		{
			if(INVALID_SOCKET != sk)
				closesocket(sk);
			//std::cout << " I already connected to you" << std::endl;
			return;
		}
	}

	/*
	sockaddr_in myself;
	memset(&myself, 0, sizeof(sockaddr_in));
	myself.sin_family = AF)INET;
	myself.sin_port = htons(Option::Instance()->Listen_Port());
	myself.sin_addr.s_addr = inet_addr(Option::Instance()->Local_Addr());

	if(myself.sin_addr.s_addr == INADDR_NONE)
	{
		struct hostent * ph = gethostbyname(Option::Instance()->Local_Addr());
		if(!ph || ph->h_addrtype != AF_INET)
		{
			throw "invalid local address";
		}
		memcpy(&(myself.sin_addr), ph->h_addr_list[0], sizeof(struct in_addr));
	}
*/

	//if(memcmp(&(Option::Instance()->Myself()), &addr, sizeof(struct sockaddr_in)) == 0)
	if(memcmp(&(Option::Instance()->Myself().sin_addr), &(addr.sin_addr), sizeof(struct in_addr)) == 0)
	{
		closesocket(sk);
		//std::cout << "Don't connect to myself" << std::endl;
		return;
	}


	//std::cout << "add a peer " << std::endl;
	this->peerlist_.push_back(new Peer(addr, sk));
}

	
void Peers::Remove(sockaddr_in addr)
{
	Trace("Peers::Remove()");

	/*
	for(std::list<Peer *>::iterator itor = this->peerlist_.begin();
		itor != this->peerlist_.end();
		++itor)
	{
		if((*itor)->IsMe(addr))
		{
			delete *itor;
			this->peerlist_.erase(itor);
			break;
		}
	}
	*/
	//std::cout << "removed a peer" << std::endl;
	this->removed_.push_back(addr);
}


SOCKET Peers::SetFd(const time_t & pnow, fd_set & rfdp, fd_set & wfdp, fd_set & efdp)
{
	Trace("Peers::SetFd()");

	for(std::list<Peer *>::iterator itor = this->peerlist_.begin();
		itor != this->peerlist_.end();
		++itor)
	{
		(*itor)->SetFd(pnow, rfdp, wfdp, efdp);
	}

	return INVALID_SOCKET;
}

void Peers::CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds)
{
	Trace("Peers::CheckFd()");

	for(std::list<Peer *>::iterator itor = this->peerlist_.begin();
		itor != this->peerlist_.end();
		++itor)
	{
		(*itor)->CheckFd(rfdp, wfdp, efdp, nfds);
	}

	for(std::vector<sockaddr_in>::iterator itor = this->removed_.begin();
		itor != this->removed_.end();
		++itor)
	{
		for(std::list<Peer *>::iterator it = this->peerlist_.begin();
		it != this->peerlist_.end();
		++it)
		{
			if((*it)->IsMe(*itor))
			{
				this->peerlist_.erase(it);
				break;
			}
		}
	}

	this->removed_.clear();
}

