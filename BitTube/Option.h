

#ifndef __OPTION_H__
#define __OPTION_H__

#include <time.h>
#include <string>
#include <Winsock2.h>

class Option
{
public:
  
  static Option * Instance(void);

  //   for future use
  bool parse_args(int argc, char *argv[]);

//  void print_usage(void); for future use

  // the max number of one's neighbors
  int Max_Connected_Peers(void);

  // the port number I am listening at
  short Listen_Port(void);

  // the interval I should wait before send another request to tracker
  time_t Tracker_Interval(void);

  // the port number tracker is listening at
  // get from outside world
  short Tracker_Port(void);

  // the address of tracker ( could be IP address or domain address)
  // get from outside world
  const char * Tracker_Address(void);

  // the movie ID I am downloading
  // get from outside world
  const char * Movie_ID(void);


  void Movie_ID(const std::string & id);

  // the local address ( could be IP address or domain address)
  // get from outside world
  const char * Local_Addr(void);

  // the length of the file I am downloading
  // get from outside world
  size_t File_Size(void);


  // set file size
  void File_Size(size_t len);

  // the length of a piece
  size_t Piece_Size(void);

  // the address of the video content server
  // get from outside world
  const char * Server_Address(void);

  // the port number the video content server is listening at
  // get from outside world
  short Server_Port(void)
  {
	  return this->server_port_;
  }

  // the max number of requests a peer can send before get the corresponding response
  int Max_Concurrent_Request(void)
  {
	  return this->max_concurrent_request_;
  }

  sockaddr_in Myself(void)
  {
	  return this->myself;
  }

protected:
	Option(void);

private:
  
	int max_connected_peers_;
	short listen_port_;
	time_t tracker_interval_;
	short tracker_port_;
	std::string tracker_address_;
	std::string movie_id_;
	std::string local_addr_;
	size_t file_size_;
	size_t piece_size_;

	std::string server_address_;
	short server_port_;
	int max_concurrent_request_;
	sockaddr_in myself;

	static Option * instance_;
};

#endif

