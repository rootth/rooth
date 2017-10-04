#pragma once

//Header Files
#include "size.h"

//Class Declarations
class VECTOR
{
 public:

	double x;
	double y;
	double z;

	//Constructors
	VECTOR(): x(0), y(0), z(0){}
	VECTOR(double x, double y, double z): x(x), y(y), z(z){}
	VECTOR(_3DCoor c): x(c.x), y(c.y), z(c.z){}
	VECTOR(_3DCoorT<float> c): x(c.x), y(c.y), z(c.z){}
	
	double dot();
	double abs();
	//VECTOR perp1();//Perpendicular Vector 1
	//VECTOR perp2();//Perpendicular Vector 1
	VECTOR operator+(const VECTOR& b) const;
	VECTOR operator-(const VECTOR& b) const;
	VECTOR operator*(const VECTOR& b) const;
	VECTOR operator/(const VECTOR& b) const;
	VECTOR operator-() const;

	VECTOR& operator+=(const VECTOR& b);
	VECTOR& operator-=(const VECTOR& b);
	VECTOR& operator*=(const VECTOR& b);
	VECTOR& operator/=(const VECTOR& b);

	friend VECTOR operator*(double k, const VECTOR& a);
	friend VECTOR operator*(const VECTOR& a, double k);
	friend VECTOR operator/(const VECTOR& a, double k);
	friend bool operator==(const VECTOR& a, const VECTOR& b);
	friend bool operator!=(const VECTOR& a, const VECTOR& b);
	friend double dot(const VECTOR& a, const VECTOR& b);
	friend VECTOR cross(const VECTOR& a, const VECTOR& b);
};