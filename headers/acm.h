#pragma once

//System Dependencies
#include<Windows.h>

//Header Files
#include "vector.h"
#include "curve.h"
#include "fractalise.h"

//Function Declarations
void acmInit(RootSys& rtsys);
DWORD WINAPI ACMThreadProc(LPVOID);
DWORD WINAPI ACMThreadIter(LPVOID);
vector<CUBF_PARAM> spline(vector<VECTOR> pline, double VECTOR::* cmpt);

//Global Variables
extern double alpha;
extern double beta;
extern double gamma;
extern double delta;
extern double dt;

//extern int ziplock;
extern int iterCount;
extern bool runACM;
extern bool runCalibrate;

extern int ITER;
extern int attractLow;
extern int attractHigh;
extern int repulseLow;
extern int repulseHigh;

extern volatile HANDLE acmMutex;