#pragma once

//Class Definitions
class VECTOR;

//Structure Definitions
struct _2DSize
{
	//Default Constructor
	_2DSize(): height(0), width(0){}
	
	//Constructor
	_2DSize(unsigned height, unsigned width): height(height), width(width){}

	//Operators
	/*_2DSize& operator+(const _2DSize& c)
	{
		this->height += c.height;
		this->width  += c.width;

		return *this;
	}

	_2DSize& operator-(const _2DSize& c)
	{
		this->height -= c.height;
		this->width  -= c.width;

		return *this;
	}*/
	
	//Members
	unsigned height;
	unsigned width;
};

struct _3DSize: public _2DSize
{
	//Default Constructor
	_3DSize(): depth(0){}

	//Constructors
	_3DSize(unsigned depth, _2DSize& size): depth(depth), _2DSize(size){}
	_3DSize(unsigned depth, unsigned height, unsigned width): depth(depth), _2DSize(height, width){}

	//Operators
	/*_3DSize& operator+(const _3DSize& c)
	{
		_2DSize::operator+(c);
		this->depth += c.depth;

		return *this;
	}

	_3DSize& operator-(const _3DSize& c)
	{
		_2DSize::operator-(c);
		this->depth -= c.depth;

		return *this;
	}*/

	//Members
	unsigned depth;
};

struct _2DBase
{
	//Default Constructor
	_2DBase():baseX(0), baseY(0){};
	
	//Members
	unsigned short baseX;
	unsigned short baseY;
};

template<typename T> struct _2DCoorT
{
	//Default Constructor
	_2DCoorT(): x(0), y(0){}

	//Constructor
	_2DCoorT(T x, T y):x(x), y(y){}

	//Operators
	_2DCoorT operator+(const _2DSize& s) const
	{
		return _2DCoorT(x+s.height, y+s.width);
	}

	_2DCoorT operator-(const _2DSize& s) const
	{
		return _2DCoorT(x-s.height, y-s.width);
	}

	_2DSize operator+(const _2DCoorT& c) const
	{
		return _2DSize(unsigned(x+c.x), unsigned(y+c.y));
	}

	_2DSize operator-(const _2DCoorT& c) const
	{
		return _2DSize(unsigned(x-c.x), unsigned(y-c.y));
	}

	T x;
	T y;
};

template<typename T> struct _3DCoorT: public _2DCoorT<T>
{
	//Default Constructor
	_3DCoorT(): z(0){}

	//Constructors
	_3DCoorT(_2DCoorT& c, T z): _2DCoorT(c), z(z){}
	_3DCoorT(T x, T y, T z): _2DCoorT(x, y), z(z){}
	_3DCoorT(VECTOR& v):_2DCoorT(T(v.x), T(v.y)), z(T(v.z)){}

	//Operators
	_3DCoorT operator+(const _3DSize& s) const
	{
		return _3DCoorT(_2DCoorT::operator+(s), z+s.depth);
	}

	_3DCoorT operator-(const _3DSize& s) const
	{
		return _3DCoorT(_2DCoorT::operator-(s), z-s.depth);
	}

	_3DSize operator+(const _3DCoorT& c) const
	{
		return _3DSize(unsigned(z+c.z), _2DCoorT::operator+(c));
	}

	_3DSize operator-(const _3DCoorT& c) const
	{
		return _3DSize(unsigned(z-c.z), _2DCoorT::operator-(c));
	}

	//Members
	T z;
};

typedef _2DCoorT<unsigned short> _2DCoor;
typedef _3DCoorT<unsigned short> _3DCoor;