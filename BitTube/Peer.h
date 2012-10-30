

#ifndef __PEER_H__
#define __PEER_H__

#include <set>
#include <string>
#include <Winsock2.h>
#include "Bitmap.h"
#include "bufio.h"


#include "PeerDisconnectedState.h"
#include "PeerConnectingState.h"
#include "PeerHandshakingState.h"
#include "PeerReadyState.h"



const char MOVIE_ID = 1;
const char _BITMAP = 2; // BITMAP conflict with predefined name
const char HAVE = 4;
const char REQUEST = 8;
const char BYE = 16;
const char PIECE = 32;



class Peer
{
public:
	friend class PeerDisconnectedState;
	friend class PeerConnectingState;
	friend class PeerHandshakingState;
	friend class PeerReadyState;

	SOCKET SetFd(const time_t &, fd_set & rfdp, fd_set & wfdp, fd_set & efdp);

	void CheckFd(fd_set & rfdp, fd_set & wfdp, fd_set & efdp, int & nfds);

	Peer(sockaddr_in addr, SOCKET sk);

	bool IsMe(sockaddr_in addr);
protected:
private:

	enum State{DISCONNECTED, CONNECTING, HANDSHAKING, READY};

	/*
		Put the corresponding message in send_buffer_
		0 : success
		-1 : fail
	*/

	// tell the remote peer which movie I am interested in
	int Send_MovieID(void);

	// send my bitmap of the movie remote peer interested in 
	int Send_Bitmap(void);

	int Send_Have(size_t idx);
	int Send_Request(size_t idx);
	int Send_Bye(void);
	int Send_Piece(size_t idx);

	void Fill(char * buf, char c, size_t size);

	// clear up and remove *this peer
	void Reset(void);


	void ChangeState(State state);

	// 0 : success
	// -1 : error
	int DecodeMessage(void);

	// the bitmap of the remote peer
	Bitmap bitmap_;

	// the buffer for outgoing data
	BufIo send_buffer_;

	// the buffer for incoming data
	BufIo receive_buffer_;

	// the piece idx I have sent request to remote peer but I haven't receive the piece
	std::set<size_t> pending_queue_;


	// the state of current peer
	PeerState * state_;

	// the TCP connection handle
	SOCKET sock_;

	// the movie id remote peer interested in
	std::string movie_id_;


	// the address of remote peer
	sockaddr_in addr_;


	/*
		the static state objects
	*/
	static PeerDisconnectedState disconnected_state_;
	static PeerConnectingState connecting_state_;
	static PeerHandshakingState handshaking_state_;
	static PeerReadyState ready_state_;
};


#endif


