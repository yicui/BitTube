
#include "Option.h"
#include "Trace.h"
#include <iostream>
using namespace std;

Option * Option::instance_ = NULL;



Option * Option::Instance(void)
{
	Trace("Option::Instace()");

	if(NULL == Option::instance_)
	{
		Option::instance_ = new Option;
	}
	return Option::instance_;
}

Option::Option(void) :
max_connected_peers_(10),
listen_port_(10234),
tracker_interval_(15),
tracker_port_(6969),
tracker_address_("vanetsim.vuse.vanderbilt.edu"),
//tracker_address_("129.59.88.178"),
movie_id_("xm6cscjXsnqW59fy1ga"),
//local_addr_("192.168.0.1"),
file_size_(2078952),
piece_size_(16 * 1024),// 16K
server_address_("www.vandyvideo.com"),
//server_address_("129.59.129.204"),
server_port_(80),
max_concurrent_request_(5)
{
	//we can extend this method to configure parameters from command line
	Trace("Option::Option");

	


	// get local address
//	this->local_addr_ = "129.59.89.129";
//	this->local_addr_ = "127.0.0.1";

	char host_name[256];
	gethostname(host_name,sizeof(host_name)); 
	struct hostent *hp;
	struct in_addr sa;
	char *buf;

	hp = gethostbyname(host_name);

	if (hp != NULL)
	{
		for (int i = 0; hp->h_addr_list[i]; i++)
		{
			memcpy (&sa, hp->h_addr_list[i],hp->h_length);

			buf = inet_ntoa(sa);
			//cout << "The host IP is:" << buf << endl;
			this->local_addr_ = buf;
			//cout << this->local_addr_ << std::endl;
		}
	}



	memset(&myself, 0, sizeof(sockaddr_in));
	myself.sin_family = AF_INET;
	myself.sin_port = htons(Listen_Port());
	myself.sin_addr.s_addr = inet_addr(Local_Addr());

	if(myself.sin_addr.s_addr == INADDR_NONE)
	{
		struct hostent * ph = gethostbyname(Option::Instance()->Local_Addr());
		if(!ph || ph->h_addrtype != AF_INET)
		{
			throw "invalid local address";
		}
		memcpy(&(myself.sin_addr), ph->h_addr_list[0], sizeof(struct in_addr));
	}

	
}

int Option::Max_Connected_Peers(void)
{
	Trace("Option::Max_Connected_Peers");

	return this->max_connected_peers_;
}

short Option::Listen_Port(void)
{
	Trace("Option::Listen_Port");

	return this->listen_port_;
}

time_t Option::Tracker_Interval(void)
{
	Trace("Option::Tracker_Interval()");

	return this->tracker_interval_;
}


short Option::Tracker_Port(void)
{
	Trace("Option::Tracker_Port()");

	return this->tracker_port_;
}

const char * Option::Tracker_Address(void)
{
	Trace("Option::Tracker_Address()");
	
	return this->tracker_address_.c_str();
}

const char * Option::Movie_ID(void)
{
	Trace("Option::Movie_ID()");
	
	return this->movie_id_.c_str();
}


void Option::Movie_ID(const std::string & id)
{
	Trace("Option::Movie_ID()");

	this->movie_id_ = id;
}


const char * Option::Local_Addr(void)
{
	Trace("Option::Local_Addr()");

	return this->local_addr_.c_str();
}

size_t Option::File_Size(void)
{
	Trace("Option::File_Size()");
	return this->file_size_;
}

void Option::File_Size(size_t len)
{
	Trace("Option::File_Size()");
	this->file_size_ = len;
}

size_t Option::Piece_Size(void)
{
	Trace("Option::Piece_Size()");
	return this->piece_size_;
}


const char * Option::Server_Address(void)
{
	Trace("Option::Server_Address()");
	
	return this->server_address_.c_str();
}

bool Option::parse_args(int argc, char ** argv)
{
	Trace("Option::parse_args()");
	return true;
}