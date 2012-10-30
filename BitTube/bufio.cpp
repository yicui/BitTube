#include "./bufio.h"

#include <string.h>
#include <errno.h>

//#include "btrequest.h"

#define DEF_BUF_SIZ 36864  /* 36 KB */
#define INCREAST_SIZ 32768  /*  32 KB */
#define MAX_BUF_SIZ 135168  /* 132 KB */

#define _left_buffer_size (n - p)

BufIo::BufIo()
{
  f_socket_remote_closed = 0;
  b = new char[DEF_BUF_SIZ];
#ifndef WINDOWS
  if( !b ) throw 9;
#endif
  p = 0;
  n = DEF_BUF_SIZ;
}



// increase the buffer to n + INCREAST_SIZ if n + INCREAST_SIZ < MAX_BUF-SIZ.
// return value:
// 0 :  success
// -1: otherwise
ssize_t BufIo::_realloc_buffer()
{
  char *tbuf;

  if( ( n + INCREAST_SIZ ) >= MAX_BUF_SIZ ) return -1; // buffer too long

  tbuf = new char[n + INCREAST_SIZ];
#ifndef WINDOWS
  if( !tbuf ) return -1;
#endif

  if(p) memcpy(tbuf, b, p);
  delete []b;
  b = tbuf;
  n += INCREAST_SIZ;

  return 0;
}

// retval
// successful return bytes sended. otherwise -1;
//non-block write data to socket
//return value:
// -1 : write error
// otherwise : how many bytes have been sended
ssize_t BufIo::_SEND(SOCKET sk,  char *buf, size_t len)
{
  ssize_t r;
  size_t t = 0;
  for(; len;)
  {
    r = SEND(sk,buf,len);
    if(r < 0)
    {
#ifndef WINDOWS
      if(errno == EINTR) continue;// interupted by signal
#endif
//  if would block, return the size has been sended
      //return (EWOULDBLOCK == errno) ? (ssize_t)t : -1;// would block or error
	  return (WSAEWOULDBLOCK == WSAGetLastError()) ? (ssize_t)t : -1;
    }else if( 0 == r )// when will this happen?
    {
      return t;			// impossible???
    }else// has sended r bytes data
    {
      buf += r;
      len -= r;
      t += r;
    }
  }
  return (ssize_t)t;
}


//non-block read up to len bytes data to buf
//return value:
// -1 : read error
// otherwise : how many bytes have been read
// if the remote peer closed the connection, f_socket_remote_closed = 1;
ssize_t BufIo::_RECV(SOCKET sk, char *buf,size_t len)
{
  ssize_t r;
  size_t t = 0;
  for(; len;){
    r = RECV(sk,(char*)buf,len);
    if(r < 0){
#ifndef WINDOWS
      if(errno == EINTR) continue;
#endif
  //    return (EWOULDBLOCK == errno) ? (ssize_t)t : -1;
	  return (WSAEWOULDBLOCK == WSAGetLastError()) ? (ssize_t)t : -1;
    }else if( 0 == r ){//connection closed by remote.
      f_socket_remote_closed = 1;
      return t;		
    }else{
      buf += r;
      len -= r;
      t += r;
    }
  }
  return (ssize_t)t;
}


// put len bytes data to underlying buf
  // if needed, write data already in underlying buf to sk first
  // return value :
  // <0 : error
  // == 0 : success
ssize_t BufIo::Put(SOCKET sk, char *buf,size_t len)
{
  ssize_t r;
  if( _left_buffer_size < len ){ //no enough space
    r = FlushOut(sk);
    if( r < 0 ) return r;
    for( ; _left_buffer_size < len; ) // still no enough space
      if(_realloc_buffer() < 0) return -3;
  }
  memcpy(b + p, buf, len);
  p += len;
  return 0;
}


// read data from sk to buf until there is no more data
// return value :
// -1 : read error
// -2 : connection is closed by the remote peer
// -3 : no more buf
// otherwise : the number of data in buf after Feedln()
ssize_t BufIo::FeedIn(SOCKET sk)
{
  ssize_t r;

  if(!_left_buffer_size)
    if(_realloc_buffer() < 0) return (ssize_t) -3;
  
  r = _RECV(sk, b + p, _left_buffer_size);
  if( r < 0 ) return -1;
  else{
    if( r ) p += r;
    if( f_socket_remote_closed ) return -2; // connection closed by remote
  }
  return (ssize_t) p;
}


 // almost the same as put, 
  // except try to write all data in buf to sk at the end
   // return value :
 // >= 0 left bytes in buffer
// < 0 failed
ssize_t BufIo::PutFlush(SOCKET sk, char *buf,size_t len)
{
  if( _left_buffer_size < len && p){
    if( FlushOut(sk) < 0) return -1;
  }

  for(; _left_buffer_size < len; )
    if( _realloc_buffer() < 0) return -3;

  memcpy(b + p, buf, len);
  p += len;
  return FlushOut(sk);
}


// write data in buf to sk
// retval
// >= 0 left bytes in buffer
// < 0 failed
ssize_t BufIo::FlushOut(SOCKET sk)
{
  ssize_t r;
  if( !p ) return 0;		// no data to be send
  
  r = _SEND(sk,b,p);
  if( r < 0 ) return r;
  else if( r > 0){
    p -= r;
    if( p ) memmove(b, b + r, p);
  }
  return (ssize_t)p;
}


// remove the first len bytes data from the buf
// return value :
// -1 : if the data in buf is less than len
// 0 : otherwise
ssize_t BufIo::PickUp(size_t len)
{
  if( p < len ) return -1;
  p -= len;
  if( p ) memmove(b, b + len, p);
  return 0;
}
