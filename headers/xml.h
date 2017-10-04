#pragma once

//System Dependencies
#include<fstream>

//Header Files
#include "fractalise.h"

using namespace std;

//Global Variables
extern int SPLINE_DEN;

//Function Declarations
RootSys* importRootSystem(fstream& file);
void exportRootSystem(RootSys& rtsys);
void exportSkeletonStack(ImageStack* imgStack, char* path);