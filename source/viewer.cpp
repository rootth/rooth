//Macros
#pragma comment(lib, "lib/freeglut.lib")
#define NOMINMAX
#undef  UNICODE

//System Dependencies
#include<Windows.h>
#include<sstream>
#include<iomanip>

using namespace std;

//Header Files
#include "../headers/GL/freeglut.h"
#include "../headers/viewer.h"
#include "../headers/dialog.h"
#include "../headers/fractalise.h"
#include "../headers/console.h"
#include "../headers/vector.h"
#include "../headers/acm.h"

//#include "../headers/debug.h"

//Global Variables
int  glutWnd     = NULL;
bool closeViewer = false;

vector<Triangle> mesh;
vector<_3DCoor> rootWall;
HANDLE volatile meshMutex = NULL;
HANDLE volatile skelMutex = NULL;

DISPLAY display = {true, true, true};
ROOTINDEX rootIndex = {};

int tchInd = 0;
Region* pRegChoice = NULL;
Region::Skeleton *pskl = NULL, *currSkel = NULL, *tchSkel = NULL;
RootSys *prtsys = NULL, *currRoot = NULL, *latRoot = NULL, *tchRoot = NULL;

int connLinesLen = 50;

bool renderMovie    = false;
char videoPath[256] = {};
COLOUR background   = {};
unsigned framedelay = 100;
unsigned framerate  = 60;
_2DSize windowSize(800, 800);

//Static Global Variables
static const char* glutTitle = "Root System Viewer";

static float light_global_on[]     = {1, 1, 1, 1};
static float light_global_dimmed[] = {0.25, 0.25, 0.25, 1};

static int x = 0, y = 0, z = 0;
static float rotX = 0, rotY = 0, rotZ = 0;
static float scale = 1;
static float transx = 0;
static float transy = 0;

static const int STEP = 2;

static struct
{
	int left;
	int right;

	int x, y;

} mouseState = {GLUT_UP, GLUT_UP};

static unsigned k=0;
static double curveCoeff = 0;

//Function Definitions
static void drawTouchingRoots(RootSys& rtsys, int level);
static void drawRootSys(RootSys& rtsys, int level);
static void drawRootSpline(RootSys& rtsys, int level);
static void drawRootSnkPts(RootSys& rtsys);
static void drawRootCtrlPts(RootSys& rtsys);

//Output Rendered root system to a video file
//(sequence of *.bmp images)
void record()
{
	if(!rootSys) return;

	static char* screenshot		   = NULL;
	static BITMAPFILEHEADER header = {};
	static BITMAPINFOHEADER info   = {};
	static _2DSize size;

	//Initiate BMP Header sturcutures for movie rendering
	if(size.height!=windowSize.height || size.width!=windowSize.width)
	{
		if(!screenshot) delete screenshot;
		screenshot = new char[4*windowSize.height*windowSize.width];

		header.bfType = 'MB';
		header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 3*windowSize.height*windowSize.width+windowSize.height*((4-3*windowSize.width%4)%4);
		header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		info.biSize = sizeof(BITMAPINFOHEADER);
		info.biHeight = windowSize.height;
		info.biWidth = windowSize.width;
		info.biPlanes = 1;
		info.biBitCount = 24;
		info.biCompression = BI_RGB;
		info.biSizeImage = 3*windowSize.height*windowSize.width+windowSize.height*((4-3*windowSize.width%4)%4);
	}

	static int frameCounter = 0;
	stringstream bmpFileName;
	bmpFileName<<videoPath<<"\\frame"<<setw(4)<<setfill('0')<<frameCounter++<<".bmp";

	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, windowSize.width, windowSize.height, GL_RGBA, GL_UNSIGNED_BYTE, screenshot);

	int fileSize;
	HANDLE hFile = CreateFile(bmpFileName.str().c_str(), GENERIC_WRITE, NULL, NULL, CREATE_NEW, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(hFile!=INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, &header, sizeof(BITMAPFILEHEADER), (LPDWORD)&fileSize, NULL);
		WriteFile(hFile, &info, sizeof(BITMAPINFOHEADER), (LPDWORD)&fileSize, NULL);

		unsigned char* line = new unsigned char[3*windowSize.width+((4-3*windowSize.width%4)%4)];
		memset(line, 0, 3*windowSize.width+((4-3*windowSize.width%4)%4));

		for(int h=0; h<(int)windowSize.height; h++)
		{
			for(int w=0; w<(int)windowSize.width; w++)
			{
				line[3*w]   = screenshot[4*(h*windowSize.width+w)+2];
				line[3*w+1] = screenshot[4*(h*windowSize.width+w)+1];
				line[3*w+2] = screenshot[4*(h*windowSize.width+w)];
			}
								
			WriteFile(hFile, line, 3*windowSize.width+((4-3*windowSize.width%4)%4), (LPDWORD)&fileSize, NULL);
		}

		delete line;

		CloseHandle(hFile);
	}
}

void drawWall()
{
	glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
	glClearColor(background.red/255.f, background.green/255.f, background.blue/255.f, 1.f);
	glMatrixMode(GL_MODELVIEW);

	glColor3f(0, 1, 0);

	if(display.rootSys == DISPLAY::MESH)
	{
		//Rotate light
		glLoadIdentity();

		glTranslatef(0, 0, -5);
		glRotatef(2*rotZ, 0, 1, 0);
		glRotatef(rotY+90, 1, 0, 0);

		float light_position[] = {-1, 1, 1, 0};
	
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_global_dimmed);
	
		glLoadIdentity();

		glTranslatef(0, 0, -5);
		//glRotatef(rotZ-90, 0, 1, 0);
		glRotatef(rotY+90, 1, 0, 0);
		glRotatef(-rotX, 0, 1, 0);
		glTranslatef(transx, 0, 0);
		glRotatef(-rotZ+90, 0, 0, 1);
		glTranslatef(0, 0, transy);
		glScalef(scale, scale, scale);
		
		WaitForSingleObject(meshMutex, INFINITE);

			for(unsigned i=0; i<mesh.size(); i++)
			{
				glBegin(GL_TRIANGLES);

					glNormal3f(float(mesh[i].normal.x), float(mesh[i].normal.y), float(mesh[i].normal.z));

					glVertex3f((mesh[i].a.x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
							   (mesh[i].a.y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
							   (mesh[i].a.z-imgStack.depth/2.f)/(imgStack.depth/2.f));

					glVertex3f((mesh[i].b.x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
							   (mesh[i].b.y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
							   (mesh[i].b.z-imgStack.depth/2.f)/(imgStack.depth/2.f));

					glVertex3f((mesh[i].c.x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
							   (mesh[i].c.y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
							   (mesh[i].c.z-imgStack.depth/2.f)/(imgStack.depth/2.f));

				glEnd();
			}

		ReleaseMutex(meshMutex);
	}
	else
	{
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_global_on);
		glLoadIdentity();

		glTranslatef(0, 0, -5);
		glRotatef(rotY+90, 1, 0, 0);
		glRotatef(-rotX, 0, 1, 0);
		glTranslatef(transx, 0, 0);
		glRotatef(-rotZ, 0, 0, 1);
		glTranslatef(0, 0, transy);
		glScalef(scale, scale, scale);

		glBegin(GL_POINTS);

			for(unsigned i=0; i<rootWall.size(); i++)
				glVertex3f(-(rootWall[i].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
						    (rootWall[i].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
							(rootWall[i].z-imgStack.depth/2.f)/(imgStack.depth/2.f));

		glEnd();
	}

	//Mark the location of root tips (red for downward growing and blue for upward growing ones)
	/*glBegin(GL_POINTS);
		for(unsigned d=0; d<imgStack.depth; d++)
			for(int r=0; r<rootsTop[d].size(); r++)	
				if(rootsTop[d][r].skl.isTip || rootsTop[d][r].skl.isDownward)// && rootsTop[d][r].skl.is3DTip)
				{
					
					for(int x=-10; x<=10; x++)
						for(int y=-10; y<=10; y++)
						{
							if(rootsTop[d][r].skl.isDownward)
								glColor3f(0, 0, 1);
							else
								glColor3f(1, 0, 0);

							glVertex3f(-(rootsTop[d][r].centroids[0].x+x-imgStack.width/2.0)/(imgStack.width/2.0),
				  						(rootsTop[d][r].centroids[0].y+y-imgStack.height/2.0)/(imgStack.height/2.0),
										(d+10-imgStack.depth/2.0)/(imgStack.depth/2.0));

							glVertex3f(-(rootsTop[d][r].centroids[0].x+x-imgStack.width/2.0)/(imgStack.width/2.0),
				  						(rootsTop[d][r].centroids[0].y+y-imgStack.height/2.0)/(imgStack.height/2.0),
										(d-10-imgStack.depth/2.0)/(imgStack.depth/2.0));
						}

					for(int y=-10; y<=10; y++)
						for(int z=-10; z<=10; z++)
						{
							if(rootsTop[d][r].skl.isDownward)
								glColor3f(0, 0, 1);
							else
								glColor3f(1, 0, 0);

							glVertex3f(-(rootsTop[d][r].centroids[0].x+10-imgStack.width/2.0)/(imgStack.width/2.0),
				  						(rootsTop[d][r].centroids[0].y+y-imgStack.height/2.0)/(imgStack.height/2.0),
										(d+z-imgStack.depth/2.0)/(imgStack.depth/2.0));

							glVertex3f(-(rootsTop[d][r].centroids[0].x-10-imgStack.width/2.0)/(imgStack.width/2.0),
				  						(rootsTop[d][r].centroids[0].y+y-imgStack.height/2.0)/(imgStack.height/2.0),
										(d-z-imgStack.depth/2.0)/(imgStack.depth/2.0));
						}

					for(int x=-10; x<=10; x++)
						for(int z=-10; z<=10; z++)
						{
							if(rootsTop[d][r].skl.isDownward)
								glColor3f(0, 0, 1);
							else
								glColor3f(1, 0, 0);

							glVertex3f(-(rootsTop[d][r].centroids[0].x+x-imgStack.width/2.0)/(imgStack.width/2.0),
				  						(rootsTop[d][r].centroids[0].y+10-imgStack.height/2.0)/(imgStack.height/2.0),
										(d+z-imgStack.depth/2.0)/(imgStack.depth/2.0));

							glVertex3f(-(rootsTop[d][r].centroids[0].x+x-imgStack.width/2.0)/(imgStack.width/2.0),
				  						(rootsTop[d][r].centroids[0].y-10-imgStack.height/2.0)/(imgStack.height/2.0),
										(d+z-imgStack.depth/2.0)/(imgStack.depth/2.0));
						}
				}
	glEnd();*/

	if(renderMovie) record();
	
	glutSwapBuffers();
}

void drawSkeleton()
{	
	glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT);
	glClearColor(background.red/255.f, background.green/255.f, background.blue/255.f, 1.f);
	glMatrixMode(GL_MODELVIEW);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_global_on);
	
	glLoadIdentity();

	glTranslatef(0, 0, -5);
	glRotatef(rotY+90, 1, 0, 0);
	glRotatef(-rotX, 0, 1, 0);
	glTranslatef(transx, 0, 0);
	glRotatef(-rotZ, 0, 0, 1);
	glTranslatef(0, 0, transy);
	glScalef(scale, scale, scale);

	vector<Region>* roots = rootsTop;

	if(menuFlags.mse)
	{
		glColor3f(0, 0, 1);
		glBegin(GL_POINTS);
			for(unsigned d=1; d<imgStack.depth-1; d++)
				for(unsigned r=0; r<roots[d].size(); r++)
					if(roots[d][r].skl.length)
					{
						glColor3f(0, 0, 1);
						glVertex3f(-(roots[d][r].skl.x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
								    (roots[d][r].skl.y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
								    (d-imgStack.depth/2.f)/(imgStack.depth/2.f));

						if(roots[d][r].skl.k == k || k == menuParam.K)
						{
							glColor3f(0, 1, 0);
							for(int i=0; i<roots[d][r].skl.mse.size(); i++)
								glVertex3f(-(roots[d][r].skl.mse[i].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
									 	    (roots[d][r].skl.mse[i].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
										    (d+(i<roots[d][r].skl.mse.size()/2?0:1)-imgStack.depth/2.f)/(imgStack.depth/2.f));
						}
					}
		glEnd();
	}
	else
	{
		if(rootSys)
		{
			WaitForSingleObject(skelMutex, INFINITE);
			glBegin(GL_POINTS);

			prtsys   = NULL;
			currSkel = NULL;
			currRoot = NULL;
			latRoot  = NULL;

			if(display.crvFitSkl){drawTouchingRoots(*rootSys, 0); drawRootSys(*rootSys, 0);}
			if(display.ctrlPts.top || display.ctrlPts.front || display.ctrlPts.left) drawRootCtrlPts(*rootSys);
			if(display.snakePoints) drawRootSnkPts(*rootSys);
			if(display.spline) drawRootSpline(*rootSys, 0);
			
			glEnd();
			ReleaseMutex(skelMutex);
		}
	}

	if(menuFlags.undetect)
	{
		glBegin(GL_POINTS);
			glColor3f(1, 0, 0);

			for(unsigned d=1; d<imgStack.depth-1; d++)
				for(unsigned r=0; r<roots[d].size(); r++)	
					if(!roots[d][r].skl.length)
						glVertex3f(-(roots[d][r].centroids[0].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
								    (roots[d][r].centroids[0].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
								    (d-imgStack.depth/2.f)/(imgStack.depth/2.f));
		glEnd();
	}

	if(renderMovie)
	{
		glLoadIdentity();

		glTranslatef(-1.25, 1, -5);

		glColor3f(1, 1, 1);
		glRasterPos2f(0.1f, 0.1f);

		char str[256] = {};
		wsprintf(str, "Root Number - %i", rootIndex.number);
		if(rootIndex.number) glutBitmapString(GLUT_BITMAP_HELVETICA_12, (unsigned char*)str);

		static unsigned count = 0;
		
		if(count++>framerate)
		{
			count = 0;
			rootIndex.number++;
		}

		record();
	}

	glutSwapBuffers();
}

//Draw Touching roots alternative paths
void drawTouchingRoots(RootSys& rtsys, int level)
{
	static int rootNum;
	static int rootLen;

	if(!level) rootNum = 0;
	rootLen = 0;

	if(rootIndex.number>=0)
	{
		if(rtsys.size()>menuParam.prune)//DEBUG
			rootNum++;
		else
			return;
	}

	bool dspTchPath = true;
	RootSys* prs = NULL;
	Region::Skeleton* pskel;

	for(int i=0; i<rtsys.size(); i++)
	{
		if(level >= rootIndex.level)
			if(rtsys[i].nbrSeg.size()>1)
				if(++rootLen >= rootIndex.length) dspTchPath = false;

		if(rootNum == rootIndex.number && rtsys[i].pskl)
		{
			for(int p=0; p<rtsys[i].pskl->parent.size(); p++)
				if(rtsys[i].pskl->parent[p] && rtsys[i].pskl->parent[p]->skl.prtsys)
					if(rtsys[i].pskl->parent[p]->skl.prtsys != rtsys[i].pskl->prtsys && rtsys[i].pskl->parent[p]->skl.prtsys->size()>menuParam.prune)
						prs = (prs?(pskel = &rtsys[i].pskl->parent[p]->skl)->prtsys:(RootSys*)-1);

			for(int p=0; p<rtsys[i].pskl->path.size(); p++)
				if(rtsys[i].pskl->path[p] && rtsys[i].pskl->path[p]->skl.prtsys)
					if(rtsys[i].pskl->path[p]->skl.prtsys != rtsys[i].pskl->prtsys && rtsys[i].pskl->path[p]->skl.prtsys->size()>menuParam.prune)
						prs = (prs?(pskel = &rtsys[i].pskl->path[p]->skl)->prtsys:(RootSys*)-1);
		}
	}

	if(prs && prs!=(RootSys*)-1)//skip first branch point
		if(dspTchPath && rootIndex.length-rootLen-1>=0 && prs->latRoots.size())
		{
			int rlts = -1;

			for(int t=0; t<=rootIndex.length-rootLen-1; t++)
			{
				if(rlts<int(prs->latRoots.size())-1) rlts++;
				while(rlts<int(prs->latRoots.size())-1 && prs->latRoots[rlts]->size()<=menuParam.prune) rlts++;
			}

			tchRoot = prs;
			tchSkel = pskel;
			tchInd  = rlts;

			glColor3f(0.9f,0.5f,0.15f);

			for(int r=0; r<prs->latRoots[rlts]->size(); r++)
				glVertex3f(-((*prs->latRoots[rlts])[r].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
							((*prs->latRoots[rlts])[r].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
							((*prs->latRoots[rlts])[r].z-imgStack.depth/2.f)/(imgStack.depth/2.f));
		}
		else tchRoot = NULL;

	for(int i=0; i<rtsys.latRoots.size(); i++)
		drawTouchingRoots(*rtsys.latRoots[i], level+1);
}

void drawRootSys(RootSys& rtsys, int level)
{
	static int rootNum;
	static int rootLen;
	static int rootSeg;

	//if(level >= ::level) return;
	
	if(!level) rootNum = 0;
	rootLen = 0;
	rootSeg = 0;
	
	if(rootIndex.number>=0)
	{
		if(rtsys.size()>menuParam.prune)//DEBUG
			rootNum++;
		else
			return;//goto drawNext;
	}
	else
	{
		if(rtsys.size()>menuParam.prune)//DEBUG
		//if(rtsys.is3DTip)
			glColor3f(0, 0, 1);
		else
			glColor3f(0, 1, 0);
	}

	if(rootNum == rootIndex.number)
	{
		if(rtsys.pInitReg) currSkel = &rtsys.pInitReg->skl;

		if(level < rootIndex.level)
		{
			if(rtsys.latRoots.size())
			{
				int rlts = -1;

				for(int l=0; l<=rootIndex.lateral; l++)
				{
					if(rlts<int(rtsys.latRoots.size())-1) rlts++;
					while(rlts<int(rtsys.latRoots.size())-1 && rtsys.latRoots[rlts]->size()<=menuParam.prune) rlts++;
				}

				glColor3f(1, 0, 1);
			
				for(int i=0; i<rtsys.latRoots[rlts]->size(); i++)
					glVertex3f(-((*rtsys.latRoots[rlts])[i].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
								((*rtsys.latRoots[rlts])[i].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
								((*rtsys.latRoots[rlts])[i].z-imgStack.depth/2.f)/(imgStack.depth/2.f));

				currRoot = NULL;
				latRoot  = rtsys.latRoots[rlts];
			}
		}
		else
		{
			currRoot = &rtsys;
			latRoot  = NULL;
		}
	}

	for(int i=0; i<rtsys.size(); i++)
	{
		if(level >= rootIndex.level)
		{
			if(rtsys[i].nbrSeg.size()>1)
				if(rootLen++ == rootIndex.length-1)
					if(rootNum == rootIndex.number)
					{
						if(rootIndex.segment>=rtsys[i].nbrSeg.size())
							rootIndex.segment = (int)rtsys[i].nbrSeg.size()-1;

						prtsys = &rtsys;
						pskl = rtsys[i].pskl;
						pRegChoice = rtsys[i].nbrSeg[rootIndex.segment].reg;

						for(int n=0; n<rtsys[i].nbrSeg.size(); n++)	
						{
							if(rootSeg++ == rootIndex.segment)
								glColor3f(1, 1, 0);//next selected root segment
							else
								glColor3f(0, 0, 1);//other available options (root segments where the root path could branch out)

							for(int m=0; m<rtsys[i].nbrSeg[n].size(); m++)
								glVertex3f(-(rtsys[i].nbrSeg[n][m].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
											(rtsys[i].nbrSeg[n][m].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
											(rtsys[i].nbrSeg[n][m].z-imgStack.depth/2.f)/(imgStack.depth/2.f));
						}
					}
		}

		if(level >= rootIndex.level)
			if(rootNum == rootIndex.number)
				if(rootLen<rootIndex.length)
					glColor3f(0, 1, 0);//traced segments of the currently selected root
				else
					glColor3f(1, 0, 0);//remaining segmnents of the currently selected root
			else
				glColor3f(0.25, 0.25, 0.25);//all remaining roots
		else
			if(rootNum == rootIndex.number)
				glColor3d(0, 1, 1);
			else
				glColor3d(0.75, 0.75, 0.75);//parent root of higher levels

		glVertex3f(-(rtsys[i].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
					(rtsys[i].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
					(rtsys[i].z-imgStack.depth/2.f)/(imgStack.depth/2.f));
	}

//drawNext:
	for(int i=0; i<rtsys.latRoots.size(); i++)
		drawRootSys(*rtsys.latRoots[i], level+1);
}

void drawRootSnkPts(RootSys& rtsys)
{
	const int CB_SZ = 2;
	
	WaitForSingleObject(acmMutex, INFINITE);
				
		for(int p=0; p<(rtsys.acmpts.size()+1)/2; p++)
		{
			//if(p<ziplock)
				glColor3f(1, 0, 1);
			//else
			//	glColor3f(0.5f, 0.5f, 0.5f);

			for(int x=-CB_SZ; x<=CB_SZ; x++)
				for(int y=-CB_SZ; y<=CB_SZ; y++)
					for(int z=-CB_SZ; z<=CB_SZ; z++)
					{
						glVertex3f(-(float(rtsys.acmpts[p].x+x)-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
									(float(rtsys.acmpts[p].y+y)-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
									(float(rtsys.acmpts[p].z+z)-imgStack.depth/2.f)/(imgStack.depth/2.f));

						glVertex3f(-(float(rtsys.acmpts[rtsys.acmpts.size()-1-p].x+x)-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
									(float(rtsys.acmpts[rtsys.acmpts.size()-1-p].y+y)-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
									(float(rtsys.acmpts[rtsys.acmpts.size()-1-p].z+z)-imgStack.depth/2.f)/(imgStack.depth/2.f));
					}
		}

	ReleaseMutex(acmMutex);

	for(int i=0; i<rtsys.latRoots.size(); i++)
		if(rtsys.latRoots[i]->size()>menuParam.prune || (!rtsys.size() && rtsys.latRoots[i]->acmpts.size()))//DEBUG
			drawRootSnkPts(*rtsys.latRoots[i]);
}

void drawRootSpline(RootSys& rtsys, int level)
{
	static int rootNum;
	const int SPLINE_DEN = 250;

	if(!level) rootNum = 0;
	if(rootIndex.number>=0) rootNum++;

	if(rootNum==rootIndex.number && level>=rootIndex.level) currRoot = &rtsys;

	double maxk = 0;
	double mink = numeric_limits<double>::max();

	WaitForSingleObject(acmMutex, INFINITE);

		if(display.curvature)
		{
			for(int c=0; c<rtsys.curve.size(); c++)
			{
				double ax = rtsys.curve[c].xp.a;
				double bx = rtsys.curve[c].xp.b;
				double cx = rtsys.curve[c].xp.c;

				double ay = rtsys.curve[c].yp.a;
				double by = rtsys.curve[c].yp.b;
				double cy = rtsys.curve[c].yp.c;

				double az = rtsys.curve[c].zp.a;
				double bz = rtsys.curve[c].zp.b;
				double cz = rtsys.curve[c].zp.c;
				
				for(double t=0; t<1.0f; t+=1.0f/SPLINE_DEN)
				{
					float x = float(rtsys.curve[c].xp.a*t*t*t + rtsys.curve[c].xp.b*t*t + rtsys.curve[c].xp.c*t + rtsys.curve[c].xp.d);
					float y = float(rtsys.curve[c].yp.a*t*t*t + rtsys.curve[c].yp.b*t*t + rtsys.curve[c].yp.c*t + rtsys.curve[c].yp.d);
					float z = float(rtsys.curve[c].zp.a*t*t*t + rtsys.curve[c].zp.b*t*t + rtsys.curve[c].zp.c*t + rtsys.curve[c].zp.d);

					double dx  = 3*ax*t*t + 2*bx*t + cx;
					double ddx = 6*ax*t   + 2*bx;

					double dy  = 3*ay*t*t + 2*by*t + cy;
					double ddy = 6*ay*t   + 2*by;

					double dz  = 3*az*t*t + 2*bz*t + cz;
					double ddz = 6*az*t   + 2*bz;

					double k = abs(log(sqrt((pow(ddz*dy-ddy*dz, 2)+pow(ddx*dz-ddz*dx, 2)+(ddy*dx-ddx*dy, 2))/pow(dx*dx+dy*dy+dz*dz,3))));

					if(k>maxk) maxk = k;
					if(k<mink) mink = k;
				}
			}

			maxk -= curveCoeff*(maxk-mink);
		}

		//Draw Connecting Lines
		if(!display.curvature && rtsys.parSnkPt.x>0 && rtsys.parSnkPt.y>0 && rtsys.parSnkPt.z>0 && rtsys.acmpts.size())
		{
			VECTOR v = rtsys.parSnkPt-rtsys.acmpts[0];

			if(connLinesLen == -1 || v.abs()<connLinesLen)
			{
				if(rootNum == rootIndex.number)
					glColor3f(1, 0, 0);
				else
					if(display.highlightSeminals && (rtsys.type == RootSys::PRIMARY || rtsys.type == RootSys::SEMINAL))
						glColor3d(0, 1, 1);
					else
						glColor3f(1, 1, 0);
				
				for(int l=0; l<v.abs(); l++)
				{
					VECTOR point = rtsys.parSnkPt-double(l)/int(v.abs())*v;

					glVertex3f(-(float(point.x)-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
								(float(point.y)-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
								(float(point.z)-imgStack.depth/2.f)/(imgStack.depth/2.f));
				}
			}
		}

		for(int c=0; c<rtsys.curve.size(); c++)
		{
			double ax = rtsys.curve[c].xp.a;
			double bx = rtsys.curve[c].xp.b;
			double cx = rtsys.curve[c].xp.c;

			double ay = rtsys.curve[c].yp.a;
			double by = rtsys.curve[c].yp.b;
			double cy = rtsys.curve[c].yp.c;

			double az = rtsys.curve[c].zp.a;
			double bz = rtsys.curve[c].zp.b;
			double cz = rtsys.curve[c].zp.c;

			for(double t=0; t<1.0f; t+=1.0f/SPLINE_DEN)
			{
				float x = float(rtsys.curve[c].xp.a*t*t*t + rtsys.curve[c].xp.b*t*t + rtsys.curve[c].xp.c*t + rtsys.curve[c].xp.d);
				float y = float(rtsys.curve[c].yp.a*t*t*t + rtsys.curve[c].yp.b*t*t + rtsys.curve[c].yp.c*t + rtsys.curve[c].yp.d);
				float z = float(rtsys.curve[c].zp.a*t*t*t + rtsys.curve[c].zp.b*t*t + rtsys.curve[c].zp.c*t + rtsys.curve[c].zp.d);
				
				if(display.curvature)
				{
					double dx  = 3*ax*t*t + 2*bx*t + cx;
					double ddx = 6*ax*t   + 2*bx;

					double dy  = 3*ay*t*t + 2*by*t + cy;
					double ddy = 6*ay*t   + 2*by;

					double dz  = 3*az*t*t + 2*bz*t + cz;
					double ddz = 6*az*t   + 2*bz;

					double k = abs(log(sqrt((pow(ddz*dy-ddy*dz, 2)+pow(ddx*dz-ddz*dx, 2)+(ddy*dx-ddx*dy, 2))/pow(dx*dx+dy*dy+dz*dz,3))));
					float colour = float((maxk-k)/(maxk-mink));

					glColor3f(0, 0, colour);
				}
				else
					if(rootNum == rootIndex.number)
						glColor3f(1, 0, 0);
					else
						if(display.highlightSeminals && rtsys.type == RootSys::PRIMARY)
							glColor3d(0, 1, 0);
						else if(display.highlightSeminals && rtsys.type == RootSys::SEMINAL)
							glColor3d(0, 1, 1);
						else
							glColor3f(1, 1, 0);

				glVertex3f(-(x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
							(y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
							(z-imgStack.depth/2.f)/(imgStack.depth/2.f));
			}
		}
	
	ReleaseMutex(acmMutex);

	for(int i=0; i<rtsys.latRoots.size(); i++)
		if(rtsys.latRoots[i]->size()>menuParam.prune || (!rtsys.size() && rtsys.latRoots[i]->acmpts.size()))//DEBUG
			drawRootSpline(*rtsys.latRoots[i], level+1);
}

void drawRootCtrlPts(RootSys& rtsys)
{
	if(display.ctrlPts.top)
	{
		glColor3f(1, 0, 0);

		for(int r=0; r<rtsys.regs.size(); r++)
			for(unsigned c=0; c<rtsys.regs[r]->centroids.size(); c++)
				glVertex3f(-(rtsys.regs[r]->centroids[c].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
							(rtsys.regs[r]->centroids[c].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
							(rtsys.regs[r]->d-imgStack.depth/2.f)/(imgStack.depth/2.f));
	}

	if(display.ctrlPts.front)
	{
		glColor3f(0, 1, 0);

		for(unsigned c=0; c<rtsys.sideCtrlPtsFront.size(); c++)
			glVertex3f(-(rtsys.sideCtrlPtsFront[c].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
						(rtsys.sideCtrlPtsFront[c].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
						(rtsys.sideCtrlPtsFront[c].z-imgStack.depth/2.f)/(imgStack.depth/2.f));
	}

	if(display.ctrlPts.left)
	{
		glColor3f(0, 0, 1);

		for(unsigned c=0; c<rtsys.sideCtrlPtsLeft.size(); c++)
			glVertex3f(-(rtsys.sideCtrlPtsLeft[c].x-imgStack.width/2.f)/(imgStack.width/2.f)*(float(imgStack.width)/imgStack.depth),
						(rtsys.sideCtrlPtsLeft[c].y-imgStack.height/2.f)/(imgStack.height/2.f)*(float(imgStack.height)/imgStack.depth),
						(rtsys.sideCtrlPtsLeft[c].z-imgStack.depth/2.f)/(imgStack.depth/2.f));
	}

	for(int i=0; i<rtsys.latRoots.size(); i++)
		if(rtsys.latRoots[i]->size()>menuParam.prune)//DEBUG
			drawRootCtrlPts(*rtsys.latRoots[i]);
}

void updateTitle(int data)
{
	if(!glutWnd) return;
	
	stringstream s;

	if(rootIndex.number>0)
	{
		s<<"Root Number - "<<rootIndex.number;
		
		if(latRoot)
			s<<"; Lateral Root - "<<rootIndex.lateral;
		else
			s<<"; Segment - "<<rootIndex.length<<"; Branch Number - "<<rootIndex.segment;

		s<<"; Order - "<<rootIndex.level;
	}
	else s<<glutTitle;

	if(buttons.classification && currRoot)
	{
		if(currRoot->type == RootSys::PRIMARY)
			CheckRadioButton(hCtrlDlg, IDC_PRIMARY, IDC_LATERAL, IDC_PRIMARY);
		else if(currRoot->type == RootSys::SEMINAL)
			CheckRadioButton(hCtrlDlg, IDC_PRIMARY, IDC_LATERAL, IDC_SEMINAL);
		else if(currRoot->type == RootSys::LATERAL)
			CheckRadioButton(hCtrlDlg, IDC_PRIMARY, IDC_LATERAL, IDC_LATERAL);
		else
			goto unchk;
	}
	else
	{
unchk:	CheckDlgButton(hCtrlDlg, IDC_PRIMARY, false);
		CheckDlgButton(hCtrlDlg, IDC_SEMINAL, false);
		CheckDlgButton(hCtrlDlg, IDC_LATERAL, false);
	}

	//wsprintfA(str, "%s: Root Number - %i; Level - %i     ", glutTitle, rootToShow, level);

	glutSetWindowTitle(s.str().c_str());
}

void rotate(int data = 0)
{
	if(closeViewer) glutLeaveMainLoop(), closeViewer = false;

	if(mouseState.left==GLUT_UP && mouseState.right==GLUT_UP && display.spin) rotZ+=STEP;
	
	glutPostRedisplay();
	glutTimerFunc(renderMovie?framedelay:20, rotate, 0);
}

void move(int x, int y)
{
	if(mouseState.left==GLUT_DOWN)
	{
		if(glutGetModifiers()==GLUT_ACTIVE_CTRL)
		{
			transx -= 0.01f*(::z-x);
			transy -= 0.01f*(::y-y);
		}
		else
		{
			rotZ-=(::z-x)/2;
		}

		::z = x;
	}
	else if(mouseState.right==GLUT_DOWN)
	{
		if(glutGetModifiers()!=GLUT_ACTIVE_CTRL) rotX-=(::x-x)/2;
	}

	if(mouseState.right==GLUT_DOWN)
	{
		if(glutGetModifiers()==GLUT_ACTIVE_CTRL)
		{
			if(scale-0.01f*(::y-y)/2>0) scale -= 0.01f*(::y-y)/2;
		}
		else
		{
			rotY-=(::y-y)/2;
		}
		
		::x = x;
	}

	::y = y;

	glutPostRedisplay();
}

void click(int button, int state, int x, int y)
{
	if(button==GLUT_LEFT_BUTTON)
	{
		mouseState.left = state;

		::z = x;
	}

	if(button==GLUT_RIGHT_BUTTON)
	{
		mouseState.right = state;

		::x = x;
	}

	::y = y;

	if(state == GLUT_DOWN)
	{
		mouseState.x = x;
		mouseState.y = y;
	}
	else
		glutWarpPointer(mouseState.x, mouseState.y);

	glutSetCursor(mouseState.left==GLUT_DOWN||mouseState.right==GLUT_DOWN?GLUT_CURSOR_NONE:GLUT_CURSOR_LEFT_ARROW);
}

void wheel(int num, int dir, int x, int y)
{
	if(!glutWnd) return;
	
	WaitForSingleObject(skelMutex, INFINITE);
	
		if(dir>0)
		{
			latRoot?rootIndex.lateral++:rootIndex.length++;
		}
		else if(dir<0)
		{
			if(currRoot)
			{
				if(rootIndex.length>0) rootIndex.length--;
			}
			else if(latRoot)
			{
				if(rootIndex.lateral>0) rootIndex.lateral--;
			}
		}

	ReleaseMutex(skelMutex);

	updateTitle();
}

void charKeyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case '+': if(curveCoeff<1) curveCoeff+=0.01; if(k<menuParam.K) k++;
		break;

		case '-': if(curveCoeff>0) curveCoeff-=0.01; if(k) k--;
		break;

		case ' ': CheckDlgButton(hCtrlDlg, IDC_PAUSE, display.spin = !display.spin);
		break;

		case '*': display.curvature =! display.curvature;
		break;

		/*case '5':
			
			if(dspRootSys == MESH)
			{
				dspRootSys = WALL;
				CheckRadioButton(hCtrlDlg, IDC_MESH, IDC_SKEL, IDC_WALL);
			}
			else if(dspRootSys == WALL)
			{
				dspRootSys = MESH;
				CheckRadioButton(hCtrlDlg, IDC_MESH, IDC_SKEL, IDC_MESH);
			}

		break;*/

		case 127:
		{	
			if(display.crvFitSkl || display.spline)
				CtrlProc(hCtrlDlg, WM_COMMAND, LOWORD(IDC_DELETE), NULL); 
			else
				popupWarning("Display the Curve-Fitted Skeleton or ACM Spline\nin order to delete/disconnect the root system layout");
			
			break;
		}
		case '\r':
		{
			if(display.crvFitSkl)
				CtrlProc(hCtrlDlg, WM_COMMAND, LOWORD(IDC_CHANGE), NULL); 
			else
				popupWarning("Display the Curve-Fitted Skeleton in order\nto change the root");

			break;
		}
		case 'z':
		{
			if(display.crvFitSkl)
				CtrlProc(hCtrlDlg, WM_COMMAND, LOWORD(IDC_UNDO), NULL);
			else
				popupWarning("Display the Curve-Fitted Skeleton in order\nto undo the root system layout");

			break;
		}
	}
}

void keyboard(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_UP:

			if(display.rootSys == DISPLAY::WALL) display.rootSys = DISPLAY::MESH;
			if(display.rootSys == DISPLAY::SKEL) display.rootSys = DISPLAY::WALL;
			
			if(display.rootSys == DISPLAY::MESH)
				CheckRadioButton(hCtrlDlg, IDC_MESH, IDC_SKEL, IDC_MESH);
			else if(display.rootSys == DISPLAY::WALL)
				CheckRadioButton(hCtrlDlg, IDC_MESH, IDC_SKEL, IDC_WALL);

			glutDisplayFunc(drawWall);
			break;

		case GLUT_KEY_DOWN:
			
			if(display.rootSys == DISPLAY::WALL) display.rootSys = DISPLAY::SKEL;
			if(display.rootSys == DISPLAY::MESH) display.rootSys = DISPLAY::WALL;
			
			if(display.rootSys == DISPLAY::WALL)
			{
				CheckRadioButton(hCtrlDlg, IDC_MESH, IDC_SKEL, IDC_WALL);

				glutDisplayFunc(drawWall);
			}
			else if(display.rootSys == DISPLAY::SKEL)
			{
				CheckRadioButton(hCtrlDlg, IDC_MESH, IDC_SKEL, IDC_SKEL);

				glutDisplayFunc(drawSkeleton);
			}
			break;

		case GLUT_KEY_RIGHT:
		{
			rootIndex.segment++;
			goto updTitle;

		case GLUT_KEY_LEFT:

			if(rootIndex.segment>0) rootIndex.segment--;
			goto updTitle;

		case GLUT_KEY_PAGE_UP:

			rootIndex.level++;
			goto updTitle;

		case GLUT_KEY_PAGE_DOWN:

			if(rootIndex.level) rootIndex.level--;
			goto updTitle;

		case GLUT_KEY_HOME:

			rootIndex.number++;
			goto updTitle;

		case GLUT_KEY_END:

			if(rootIndex.number>0) rootIndex.number--;

updTitle:	if(glutWnd) glutTimerFunc(50, updateTitle, 0);
			break;
		}

		case GLUT_KEY_INSERT:
		{
			static int oldRootToShow = 0;

			if(rootIndex.number == -1)
				rootIndex.number = oldRootToShow;
			else
			{
				oldRootToShow = rootIndex.number;
				rootIndex.number = -1;
			}

			break;
		}
	}
}

void resize(int width, int height)
{
	windowSize = _2DSize(height, width);

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	gluPerspective(30.0, double(width)/height, 1.0, 10.0);
}

DWORD WINAPI init(LPVOID lpParam)
{
	glutInit(&argc, &argv);
	glutInitDisplayMode(GLUT_DOUBLE + GLUT_RGB + GLUT_DEPTH);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(windowSize.width, windowSize.height);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	
	glutWnd  = glutCreateWindow(glutTitle);

	EnableMenuItem(GetMenu(hMainWnd), ID_TOOLS_VIEWER, MF_BYCOMMAND + MF_DISABLED);

	glutDisplayFunc(display.rootSys==DISPLAY::SKEL?drawSkeleton:drawWall);
	glutMotionFunc(move);
	glutMouseFunc(click);
	glutKeyboardFunc(charKeyboard);
	glutSpecialFunc(keyboard);
	glutMouseWheelFunc(wheel);
	glutReshapeFunc(resize);

	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHT0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float twosides = 1;
	float light_ambient[]  = {0, 0, 0, 0};
	float light_diffuse[]  = {1, 1, 1, 1};
	float light_specular[] = {1, 1, 1, 1};
	float light_position[] = {-1, 1, 1, 0};

	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, &twosides);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_global_on);
	
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	rotate();
	updateTitle();
	glutMainLoop();

	glutWnd = NULL;
	EnableMenuItem(GetMenu(hMainWnd), ID_TOOLS_VIEWER, MF_BYCOMMAND + MF_ENABLED);

	return 0;
}