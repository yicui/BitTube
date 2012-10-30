

#include "Trace.h"
#include "Listener.h"
#include "Option.h"
#include "Tracker.h"
#include "Server.h"
#include "Peers.h"
#include "LocalServer.h"
#include "Stat.h"
#include <signal.h>
#include <stdlib.h>


void sigint_catch(int sig_no)
{
	if(SIGINT == sig_no)
	{
		Tracker::Instance()->Bye();
		signal(SIGINT,SIG_DFL);
		raise(SIGINT);
	}
}
/*
void clear_up(void)
{
	Tracker::Instance()->Bye();
}
*/


// change this parameter to adjust server downloading spead
// the larger the slower
const int server_step = 1;

int main(int argc, char ** argv)
{
	try
	{
		signal(SIGINT,sigint_catch);
		//signal(SIGABRT,sigint_catch);
		//atexit(clear_up);

		WSADATA wsadata;
		if(WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
			throw "initial socket environment error";

		
		Option::Instance()->parse_args(argc, argv);
		LocalServer::Instance()->WaitForRequest();
		Stat::Instance();
		

		time_t now;
		fd_set rfd, wfd, efd;
		int nfds;
		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		int i = 0;
		while(true)
		{
			++i;
			time(&now);
			FD_ZERO(&rfd);
			FD_ZERO(&wfd);
			FD_ZERO(&efd);

			Listener::Instance()->SetFd(now, rfd, wfd);
			Tracker::Instance()->SetFd(now, rfd, wfd, efd);
			if(i / server_step)
			{
				Server::Instance()->SetFd(now, rfd, wfd, efd);
			}
			Peers::Instance()->SetFd(now, rfd, wfd, efd);
			LocalServer::Instance()->SetFd(now, rfd, wfd, efd);

			nfds = select(0, &rfd, &wfd, &efd, &timeout);

			if(nfds > 0)
			{
				Listener::Instance()->CheckFd(rfd, wfd, nfds);
				Tracker::Instance()->CheckFd(rfd, wfd, efd, nfds);
				if(i / server_step)
				{
					Server::Instance()->CheckFd(rfd, wfd, efd, nfds);
					i = 0;
				}
				Peers::Instance()->CheckFd(rfd, wfd, efd, nfds);
				LocalServer::Instance()->CheckFd(rfd, wfd, efd, nfds);
			}
			else
			{
				//std::cout << "time out!" << std::endl;
			}

			if(!LocalServer::Instance()->Finished())
			{
				std::cout << "\rBytes from server : " << 
					Stat::Instance()->ByteFromServer() << " (" <<
					Stat::Instance()->ServerRate() << " KB/S)\t" <<
					"Bytes from peers : " << 
					Stat::Instance()->ByteFromPeers() << " (" <<
					Stat::Instance()->PeerRate() << " KB/S)";
			}
				
		}
	}
	catch(const char * str)
	{
		std::cout << str << std::endl;
	}


	WSACleanup();

	return 0;
}