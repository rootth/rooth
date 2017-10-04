#pragma once

//System Dependencies
#include<vector>

using namespace std;

//Header Files
#include "imagestack.h"

//Global Variables
extern double minCoeff;
extern unsigned long long minMSE;

//Function Declarations
void skeletonise(ImageStack& stack);