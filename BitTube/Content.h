

#ifndef __CONTENT_H__
#define __CONTENT_H__

#include "Bitmap.h"
#include <string>


class Content
{
public:
	static Content * Instance(void);

	// return the start pointer of the bitmap buffer and the length of the buffer
	// 0 : error
	// otherwise : the length of the bitmap
	size_t Get_Bitmap(const std::string & movie_id, const unsigned char * &buf);

	// copy the piece idx from buf to local buffer
	// 0 : success
	// -1 : error
	int Put_Piece(const std::string & movie_id, size_t idx, const unsigned char * buf);

	// get the pointer to the start position of piece idx
	// 0 : success
	// -1 : error
	int Get_Piece(const std::string & movie_id, size_t idx, const unsigned char * & buf);

	// some peer failed to download piece idx
	// 0 : success
	// -1 : error
	int Cancel_Piece(const std::string & movie_id, size_t idx);

	// schedule a piece to download from a remote peer with bitmap.
	// 0 : success, the scheduled piece # is stored in idx
	// -1 : fail, no piece to download from this remote peer with bitmap
	int Fetch_Piece(const std::string & movie_id, const Bitmap & bitmap, size_t & idx);


	// if I have got all the pieces
	bool Full(void);

	// return the size of piece idx of movie_id
	size_t Piece_Size(const std::string & movie_id, size_t idx);

	// return the number of pieces
	size_t Piece_Number(const std::string & movie_id);

	// do I have piece idx
	bool DoIHave(size_t idx);

protected:
	Content(void);
private:
	static Content * instance_;

	// the size of the video file
	size_t file_size_;

	// the size of a piece
	size_t piece_size_;

	// the buffer contains video file
	unsigned char * buf_;

	// the bitmap of the pieces I already have
	Bitmap bitmap_;

	// the bitmap of the pieces I already have or I have sent a request for
	Bitmap bitmap_request_;
};


#endif


