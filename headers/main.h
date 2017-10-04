#pragma once

//Macros
#undef UNICODE

//System Dependencies
#include<Windows.h>

//Header Files
#include "resource.h"
#include "imagestack.h"

//Global Variables
extern char* argv;
extern int   argc;
extern HWND hMainWnd;
extern _3DSize imgStack;
extern ImageStack *imgStackTop, *imgStackFront, *imgStackLeft;
extern const char title[];
extern unsigned char* regressionImg;
extern HICON icon;

extern struct MENUFLAGS
{
	bool mse;
	bool undetect;
	bool fill;

} menuFlags;

extern struct MENUPARAM
{
	bool autoSmthCoeff;
	
	unsigned smthCoeff;
	unsigned permDist;
	unsigned K;
	unsigned morph;
	unsigned prune;
	unsigned sideCtrlPtsThrld;
	unsigned res;

} menuParam;

//Function Definitions
void analyse();