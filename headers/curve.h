#pragma once

//Strcuture Definitions
struct CUBF_PARAM
{
	double a;
	double b;
	double c;
	double d;

	CUBF_PARAM(double a, double b, double c, double d): a(a), b(b), c(c), d(d){}
};

struct PARAM_CURVE
{
	CUBF_PARAM xp;
	CUBF_PARAM yp;
	CUBF_PARAM zp;

	PARAM_CURVE(CUBF_PARAM x, CUBF_PARAM y, CUBF_PARAM z): xp(x), yp(y), zp(z){};
};