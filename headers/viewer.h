#pragma once

//Definitions
#undef UNICODE

//System Dependencies
#include<Windows.h>
#include<vector>
#include<stack>

using namespace std;

//Header Files
#include "size.h"
#include "triangle.h"
#include "distancemap.h"
#include "fractalise.h"

//Strcuture Definitions

struct DISPLAY
{
	bool spin;
	bool highlightSeminals;
	bool crvFitSkl;
	bool snakePoints;
	bool spline;
	bool curvature;

	struct DSP_CTRL_PTS
	{
		bool top;
		bool front;
		bool left;

	} ctrlPts;

	enum DSP_ROOT_SYS {SKEL, WALL, MESH} rootSys;
};

struct ROOTINDEX
{
	int level;
	int number;
	int lateral;
	int length;
	int segment;
};

struct COLOUR
{
	unsigned short red;
	unsigned short green;
	unsigned short blue;
};

//Global Variables
extern int  glutWnd;
extern bool closeViewer;

extern vector<Triangle> mesh;
extern vector<_3DCoor> rootWall;
extern volatile HANDLE meshMutex;
extern volatile HANDLE skelMutex;

extern DISPLAY display;
extern ROOTINDEX rootIndex;

extern int tchInd;
extern Region* pRegChoice;
extern Region::Skeleton *pskl, *currSkel, *tchSkel;
extern RootSys *prtsys, *currRoot, *latRoot, *tchRoot;

extern int connLinesLen;

extern bool renderMovie;
extern char videoPath[256];
extern COLOUR background;
extern unsigned framedelay, framerate;
extern _2DSize windowSize;

//Function Declarations
DWORD WINAPI init(LPVOID lpParam);
DWORD WINAPI CtrlThread(LPVOID lpParam);
void drawWall();
void drawSkeleton();
void updateTitle(int data=0);
void wheel(int num, int dir, int x, int y);
void resize(int width, int height);