#pragma once

//Definitions
#undef UNICODE

//System Dependencies
#include<Windows.h>
#include<string>

using namespace std;

//Header Files
#include "imagestack.h"

//Global variables
//extern int rtOverheadDisp;
//extern int rtAngleLenMes;
extern string csvFileName;
extern HANDLE consoleThread;

//Function Declarations
DWORD WINAPI console(LPVOID lpParam);
void statsOutput(_3DSize stack, bool printCSV = false, bool updThrld = true, bool updClss = true);
void sendConsoleCommand(string str);