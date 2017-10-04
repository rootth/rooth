#pragma once

//Header Files
#include"../headers/fractalise.h"

//Global Varible Definitions
extern unsigned rtClssTrshld;

//Function Definitions
void rootTraits(RootSys& rtsys, bool updateClassification);
void classifyRoots(RootSys* rtsys);
unsigned rtcount(const RootSys* rtsys);