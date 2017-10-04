//System Dependencies
#include<cmath>

//Header Files
#include "..\headers\vector.h"

using namespace std;

double VECTOR::dot()
{
	return x*x+y*y+z*z;
}

double VECTOR::abs()
{
	return sqrt(dot());
}

/*VECTOR VECTOR::perp1()
{
	return VECTOR(-y, x);
}

VECTOR VECTOR::perp2()
{
	return VECTOR(y, -x);
}*/

VECTOR VECTOR::operator+(const VECTOR& b) const
{
	return VECTOR(x+b.x, y+b.y, z+b.z);
}
	
VECTOR VECTOR::operator-(const VECTOR& b) const
{
	return VECTOR(x-b.x, y-b.y, z-b.z);
}

VECTOR VECTOR::operator*(const VECTOR& b) const
{
	return VECTOR(x*b.x, y*b.y, z*b.z);
}

VECTOR VECTOR::operator/(const VECTOR& b) const
{
	return VECTOR(x/b.x, y/b.y, z/b.z);
}

VECTOR operator*(double k, const VECTOR& a)
{
	return VECTOR(k*a.x, k*a.y, k*a.z);
}

VECTOR operator*(const VECTOR& a, double k)
{
	return VECTOR(k*a.x, k*a.y, k*a.z);
}

VECTOR operator/(const VECTOR& a, double k)
{
	return VECTOR(a.x/k, a.y/k, a.z/k);
}

VECTOR VECTOR::operator-() const
{
	return VECTOR(-x, -y, -z);
}

VECTOR& VECTOR::operator+=(const VECTOR& b)
{
	*this = *this+b;

	return *this;
}

VECTOR& VECTOR::operator-=(const VECTOR& b)
{
	*this = *this-b;

	return *this;
}

VECTOR& VECTOR::operator*=(const VECTOR& b)
{
	*this = *this*b;

	return *this;
}

VECTOR& VECTOR::operator/=(const VECTOR& b)
{
	*this = *this/b;

	return *this;
}

bool operator==(const VECTOR& a, const VECTOR& b)
{
	return a.x==b.x && a.y==b.y && a.z==b.z;
}

bool operator!=(const VECTOR& a, const VECTOR& b)
{
	return a.x!=b.x || a.y!=b.y || a.z!=b.z;
}

double dot(const VECTOR& a, const VECTOR& b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

VECTOR cross(const VECTOR& a, const VECTOR& b)
{
	return VECTOR(a.y*b.z-a.z*b.y, a.x*b.z-a.z*b.x, a.x*b.y-a.y*b.x);
}