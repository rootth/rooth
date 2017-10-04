//Header Files
#include "../headers/size.h"
#include "../headers/vector.h"

struct Triangle
{
	//Constructor
	Triangle::Triangle(_3DCoorT<float>& a, _3DCoorT<float>& b, _3DCoorT<float>& c): a(a), b(b), c(c)
	{
		VECTOR n = cross(VECTOR(b)-VECTOR(a), VECTOR(c)-VECTOR(a));
		normal = n/n.abs();
	}

	//Members
	_3DCoorT<float> a;
	_3DCoorT<float> b;
	_3DCoorT<float> c;
	_3DCoorT<float> normal;
};