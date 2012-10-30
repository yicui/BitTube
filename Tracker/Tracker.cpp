#include <map>
#include <set>
#include <string>
#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


using namespace std;


map<string, set<pair<string, string> > > records;



const short port = 6969;
const char GET = 1;
const char BYE = 2;
const char HAVE = 4;

void Handle(int sk)
{
  char buf[1024];
  ssize_t r;
  r = recv(sk, buf, 1024, 0);

  if(r < 5) // error message
    return;

  char command = buf[0];
  size_t size = ntohl(*(unsigned long *)(buf + 1));

  if(r != 1 + 4 + size)
    {
      std::cout << "error message" << std::endl;
      return;
    }
  string str(buf + 1 + 4, size);
  istringstream is(str);
  string addr, port, movie_id;
  set<pair<string, string> > tmp;
  string reply;
  switch(command)
    {
    case GET:
      is >> movie_id >> addr >> port;
      std::cout << movie_id << " " << addr << " " << port << std::endl;


      // update info
      if(records.find(movie_id) == records.end())// no such movie's info
	{
	  tmp.insert(make_pair(addr, port));
	  records[movie_id] = tmp;
	}
      else // records[movie_id] is the info of movie_id
	{
	  records[movie_id].insert(make_pair(addr, port));
	}

      // send back the peer list of movie_id
      for(set<pair<string, string> >::iterator itor = records[movie_id].begin();
	  itor != records[movie_id].end();
	  ++itor)
	{
	  reply += itor->first;
	  reply += " ";
	  reply += itor->second;
	  reply += " ";
	}

      send(sk, reply.c_str(), reply.size(), 0);
      close(sk);
      break;


    case BYE:
      is >> addr >> port;
      std::cout << addr << " " << port << std::endl;

      // erase the record of this peer
      for(map<string, set<pair<string, string> > >::iterator itor = records.begin();
	  itor != records.end();
	  ++itor)
	{
	  itor->second.erase(make_pair(addr, port));
	}

      close(sk);
      break;

    case HAVE:
      std::cout << "for future use" << std::endl;
      break;
    default:
      std::cout << "error message" << std::endl;
    }
}


int main(void)
{
  struct sockaddr_in lis_addr;
  memset(&lis_addr,0, sizeof(sockaddr_in));
  lis_addr.sin_family = AF_INET;
  lis_addr.sin_addr.s_addr = INADDR_ANY;
  lis_addr.sin_port = htons(port);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if(-1 == sock)
    {
      std::cout << "socket() error" << std::endl;
      return -1;
    }

  if(bind(sock, (struct sockaddr*)&lis_addr, sizeof(struct sockaddr_in)) != 0)
    {
      std::cout << "bind() error" << std::endl;
      return -1;
    }
  

  if(listen(sock, 5) == -1)
    {
      std::cout << "listen() error" << std::endl;
      return -1;
    }

  int newsk;

  while((newsk = accept(sock, NULL, NULL)) != -1)
    {
      Handle(newsk);
    }
}
