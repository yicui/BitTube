#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <cstddef> // for size_t

class Bitmap
{	
	class OutOfRange
	{};

public:
	Bitmap(size_t); // default constructor
	Bitmap(size_t, unsigned char *);// initialize from buf
	Bitmap(const Bitmap &); // copy constructor
	~Bitmap(); // destructor
	void operator = (const Bitmap &); // assignment operator

	// *this += b;
	void Union(const Bitmap &);
	// *this -= b;
	void Except(const Bitmap &);

	void Set(size_t);
	void UnSet(size_t);
	bool IsSet(size_t) const;

	// return the index of the first set bit
	size_t FirstSetBit(void) const;

	bool IsEmpty(void) const
	{
		return 0 == this->nset_;
	}

	bool IsFull(void) const
	{
		return this->nset_ == this->nbit_;
	}

	size_t NSet(void) // the number of set bits
	{
		return this->nset_;
	}

	size_t NBit(void) // the number of total bits
	{
		return this->nbit_;
	}

	size_t NBytes(void) // the number of bytes in buf
	{
		return this->nbytes_;
	}

	unsigned char * Buf(void) // the beginning position of the buf
	{
		return this->b_;
	}
private:
	void Recalculate(void); // recalculate the number of set bits
	unsigned char * b_;
	size_t nset_;
	size_t nbit_;
	size_t nbytes_;
};

#endif
