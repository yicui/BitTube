
#include "Bitmap.h"
#include "Trace.h"
#include <cstring> // for memset() and memcpy()


Bitmap::Bitmap(size_t s) : b_(NULL), nset_(0), nbit_(s), nbytes_(s % 8 ? s / 8 + 1 : s / 8)
{
	Trace("Bitmap::Bitmap()");
	this->b_ = new unsigned char[this->nbytes_];
	memset(this->b_, 0, this->nbytes_);
}


Bitmap::Bitmap(size_t s, unsigned char * b) : b_(NULL), nset_(0), nbit_(s), nbytes_(s % 8 ? s / 8 + 1 : s / 8)
{
	Trace("Bitmap::Bitmap()");
	this->b_ = new unsigned char[this->nbytes_];
	memcpy(this->b_, b, this->nbytes_);

	// calculate how may bits have been set
	this->Recalculate();
}


Bitmap::Bitmap(const Bitmap & bm) : b_(new unsigned char[bm.nbytes_]), nset_(bm.nset_), nbit_(bm.nbit_), nbytes_(bm.nbytes_)
{
	Trace("Bitmap::Bitmap()");
	memcpy(this->b_, bm.b_, this->nbytes_);
}

Bitmap::~Bitmap()
{
	Trace("Bitmap::~Bitmap()");
	delete [] this->b_;
}

void Bitmap::operator = (const Bitmap & bm)
{
	Trace("Bitmap::operator = ()");
	if(this == &bm)
		return;

	unsigned char * pc = new unsigned char[bm.nbytes_];
	delete this->b_;
	this->b_ = pc;

	this->nset_ = bm.nset_;
	this->nbit_ = bm.nbit_;
	this->nbytes_ = bm.nbytes_;

	memcpy(this->b_, bm.b_, this->nbytes_);
}


void Bitmap::Union(const Bitmap & bm)
{
	Trace("Bitmap::Union()");
	if(this->nbit_ != bm.nbit_)
		throw OutOfRange();

	for(size_t i = 0; i < this->nbytes_; ++i)
		this->b_[i] |= bm.b_[i];
	this->Recalculate();
}


void Bitmap::Except(const Bitmap & bm)
{
	Trace("Bitmap::Except()");
	if(this->nbit_ != bm.nbit_)
		throw OutOfRange();

	for(size_t i = 0; i < this->nbytes_; ++i)
		this->b_[i] &= ~bm.b_[i];
	this->Recalculate();
}


void Bitmap::Set(size_t idx)
{
	Trace("Bitmap::Set()");
	if(idx >= this->nbit_)
		throw OutOfRange();

	size_t pos = idx / 8;
	
	unsigned char mask = static_cast<unsigned char>(1) << static_cast<unsigned char>(7 - idx % 8);
	if(this->b_[pos] & mask) // already set
		return;
	else
	{
		this->b_[pos] |= mask;
		++this->nset_;
	}
}


void Bitmap::UnSet(size_t idx)
{
	Trace("Bitmap::UnSet()");
	if(idx >= this->nbit_)
		throw OutOfRange();

	size_t pos = idx / 8;
	unsigned char mask = static_cast<unsigned char>(1) << static_cast<unsigned char>(7 - idx % 8);
	if(this->b_[pos] & mask) // already set
	{
		mask = ~mask;
		this->b_[pos] &= mask;
		--this->nset_;
	}
}


bool Bitmap::IsSet(size_t idx) const
{
	Trace("Bitmap::IsSet()");
	if(idx >= this->nbit_)
		throw OutOfRange();

	size_t pos = idx / 8;
	unsigned char mask = static_cast<unsigned char>(1) << static_cast<unsigned char>(7 - idx % 8);
	return 0 != (this->b_[pos] & mask);
}


void Bitmap::Recalculate(void)
{
	Trace("Bitmap::Recalculate()");
	this->nset_ = 0;
	for(size_t i = 0; i < this->nbytes_; ++i)
	{
		unsigned char temp = this->b_[i];
		while(temp)
		{
			++this->nset_;
			temp &= (temp - 1);
		}
	}
}



size_t Bitmap::FirstSetBit(void) const
{
	Trace("Bitmap::FirstSetBit()");

	if(0 == this->nset_)
		throw OutOfRange();

	for(size_t i = 0; i < this->nbytes_; ++i)
	{
		unsigned char temp = this->b_[i];
		if(temp)
		{
			if(temp >> 7)
				return i * 8;
			if(temp >> 6)
				return i * 8 + 1;
			if(temp >> 5)
				return i * 8 + 2;
			if(temp >> 4)
				return i * 8 + 3;
			if(temp >> 3)
				return i * 8 + 4;
			if(temp >> 2)
				return i * 8 + 5;
			if(temp >> 1)
				return i * 8 + 6;
			return i * 8 + 7;
		}
	}
}

