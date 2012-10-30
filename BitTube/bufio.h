#ifndef BUFIO_H
#define BUFIO_H

//#include <sys/types.h>
//#include "def.h"

typedef int ssize_t;
typedef int socklen_t;
typedef unsigned __int64 u_int64_t;
typedef unsigned __int8 u_int8_t;

#define PATH_SP '\\'
#define MAXPATHLEN 1024
#define MAXHOSTNAMELEN 256

//#define mkdir(path,mode) _mkdir((path))

#define snprintf _snprintf
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define ioctl ioctlsocket

#define RECV(fd,buf,len) recv((fd),(buf),(len),0)
#define SEND(fd,buf,len) send((fd),(buf),(len),0)
#define EWOULDBLOCK	WSAEWOULDBLOCK
#define EINPROGRESS	WSAEINPROGRESS
#define CLOSE_SOCKET(sk) closesocket((sk))

#include <Winsock2.h>


class BufIo
{
 private:
  char *b; // point to the beginning of the buf
  size_t p; // the number of bytes left in the buf
  size_t n; // the length of the buf, default is DEF_BUF_SIZ

  int f_socket_remote_closed;

// increase the buffer to n + INCREAST_SIZ if n + INCREAST_SIZ < MAX_BUF-SIZ.
// return value:
// 0 :  success
// -1: otherwise
  ssize_t _realloc_buffer();


//non-block write data to socket
//return value:
// -1 : write error
// otherwise : how many bytes have been sended
  ssize_t _SEND(SOCKET socket,char *buf,size_t len);

//non-block read up to len bytes data to buf
//return value:
// -1 : read error
// otherwise : how many bytes have been read
// if the remote peer closed the connection, f_socket_remote_closed = 1;
  ssize_t _RECV(SOCKET socket,char *buf,size_t len);
  
 public:

	 bool Remote_Closed(void)
	 {
		 return this->f_socket_remote_closed;
	 }

 //allocate DEF_BUF_SIZ bytes buf to b
  BufIo();
 // delete buf
  ~BufIo() { if(b){ delete []b; b = (char*) 0;} }


// discard all data in buf
// f_socket_remote_closed = 0;
  void Reset(){ p = 0; f_socket_remote_closed = 0;}

// release the buf
  void Close(){
    if( b ){ delete []b; b = (char*) 0; }
    p = n = 0;
  }

  size_t Count() const { return p; } //缓存中现有字节数
  size_t LeftSize() const { return (n - p); }// how many space left in buf


// remove the first len bytes data from the buf
// return value :
// -1 : if the data in buf is less than len
// 0 : otherwise
  ssize_t PickUp(size_t len); //移除缓存中前len个字节


// read data from sk to buf until there is no more data
// return value :
// -1 : read error
// -2 : connection is closed by the remote peer
// -3 : no more buf
// otherwise : the number of data in buf after Feedln()
  ssize_t FeedIn(SOCKET sk); //从sk读数据到缓存直到暂时无数据可读或缓冲区满


// write data in buf to sk
// retval
// >= 0 left bytes in buffer
// < 0 failed
  ssize_t FlushOut(SOCKET sk); //将缓存中数据写到socket


  // put len bytes data to underlying buf
  // if needed, write data already in underlying buf to sk first
  // return value :
  // <0 : error
  // == 0 : success
  ssize_t Put(SOCKET sk,char *buf,size_t len); //将buf内容添加到缓存


  // almost the same as put, 
  // except try to write all data in buf to sk at the end
   // return value :
 // >= 0 left bytes in buffer
// < 0 failed
  ssize_t PutFlush(SOCKET sk,char *buf,size_t len);

// return the beginning of the whole buf
  char *BasePointer(){ return b; }
// return the place of the beginning of the free buf
  char *CurrentPointer() { return ( b + p); }
};

#endif
