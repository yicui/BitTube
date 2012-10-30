
#include "Content.h"
#include "Trace.h"
#include "Option.h"


Content * Content::instance_ = NULL;

Content * Content::Instance(void)
{
	Trace("Content::Instance()");

	if(NULL == Content::instance_)
		Content::instance_ = new Content;

	return Content::instance_;
}

Content::Content(void) :
file_size_(Option::Instance()->File_Size()),
piece_size_(Option::Instance()->Piece_Size()),
buf_(new unsigned char[Option::Instance()->File_Size()]),
bitmap_((Option::Instance()->File_Size() + Option::Instance()->Piece_Size() - 1) / Option::Instance()->Piece_Size()),
bitmap_request_(bitmap_)
{
	Trace("Content::Content()");
}

// return the start pointer of the bitmap buffer and the length of the buffer
// 0 : error
// otherwise : the length of the bitmap
size_t Content::Get_Bitmap(const std::string &, const unsigned char * &buf)
{
	Trace("Content::Get_Bitmap()");

	buf = this->bitmap_.Buf();
	return this->bitmap_.NBytes();
}

// copy the piece idx from buf to local buffer
// 0 : success
// -1 : error
int Content::Put_Piece(const std::string &, size_t idx, const unsigned char * buf)
{
	Trace("Content::Put_Piece()");
	////std::cout << idx << " ";

	if(this->bitmap_.IsSet(idx)) // I already have this piece
		return -1;

	this->bitmap_.Set(idx);
	if(idx == this->bitmap_.NBit() - 1) // idx is the last piece
		memcpy(this->buf_ + idx * this->piece_size_, buf, this->file_size_ - this->piece_size_ * (this->bitmap_.NBit() - 1));
	else
		memcpy(this->buf_ + idx * this->piece_size_, buf, this->piece_size_);
	//std::cout << idx << " ";
	return 0;
}


// get the pointer to the start position of piece idx
// 0 : success
// -1 : error
int Content::Get_Piece(const std::string &, size_t idx, const unsigned char * & buf)
{
	Trace("Content::Get_Piece()");

	if(!this->bitmap_.IsSet(idx)) // I don't have piece idx
		return -1;

	buf = this->buf_ + idx * this->piece_size_;
	return 0;
}


// some peer failed to download piece idx
// 0 : success
// -1 : error
int Content::Cancel_Piece(const std::string &, size_t idx)
{
	Trace("Content::Cancel_Piece()");

	if(!this->bitmap_request_.IsSet(idx))
		return -1;

	this->bitmap_request_.UnSet(idx);
	return 0;
}



// schedule a piece to download from a remote peer with bitmap.
// 0 : success, the scheduled piece # is stored in idx
// -1 : fail, no piece to download from this remote peer with bitmap
int Content::Fetch_Piece(const std::string &, const Bitmap & bitmap, size_t & idx)
{
	Trace("Content::Fetch_Piece()");

	Bitmap tbm(bitmap);
//	//std::cout << "tbm.Nset()" << tbm.NSet() << std::endl;
	tbm.Except(this->bitmap_request_);
//	//std::cout << "tbm.Nset()" << tbm.NSet() << std::endl;

	if(tbm.NSet() == 0)
		return -1;
	
	idx = tbm.FirstSetBit();
//	//std::cout << "idx " << idx << std::endl;
//	//std::cout << "tbm.Nset()" << tbm.NSet() << std::endl;
	this->bitmap_request_.Set(idx);
//	//std::cout << idx << std::endl;
	return 0;
}


bool Content::Full(void)
{
	Trace("Content::Full()");

	return this->bitmap_.NBit() == this->bitmap_.NSet();
}



size_t Content::Piece_Size(const std::string &, size_t idx)
{
	size_t s = (this->file_size_ + this->piece_size_ - 1) / this->piece_size_;
	if(idx < s - 1)
		return this->piece_size_;
	else if(idx == s - 1)
		return this->file_size_ - (s - 1) * this->piece_size_;
	else
		throw "wrong piece index";
}



size_t Content::Piece_Number(const std::string & movie_id)
{
	return (this->file_size_ + this->piece_size_ - 1) / this->piece_size_;
}

bool Content::DoIHave(size_t idx)
{
	return this->bitmap_.IsSet(idx);
}

