//Macros
#pragma once
#pragma warning(disable: 4351)//disable waring that rootAngle array is initialised in the contructor of RooSys

//System Dependencies
#include<vector>
#include<stack>

using namespace std;

//Header Files
#include "imagestack.h"
#include "distancemap.h"
#include "vector.h"
#include "curve.h"

//#include "debug.h"

//Class Definitions
class RootSkelVox: public _3DCoor
{
 public:
	class NbrSeg: public vector<_3DCoor>
	{
	 public:
		NbrSeg(vector<_3DCoor> v, Region* r):vector<_3DCoor>(v), reg(r){}
		
		Region* reg;
	};	

	//RootSkelVox():pskl(), pNextReg(){}
	//RootSkelVox(_2DCoor& c, unsigned short z):_3DCoor(c, z), pskl(), pNextReg(){}
	RootSkelVox(_3DCoor& c):_3DCoor(c), pskl(), pNextReg(){}
	RootSkelVox(Region::Skeleton* pskel, vector<NbrSeg>& ns):_3DCoor(*pskel), nbrSeg(ns), pskl(pskel), pNextReg(){}

	vector<NbrSeg> nbrSeg;
	Region::Skeleton* pskl;
	Region* pNextReg;
};

class RootSys: public vector<RootSkelVox>
{
 public:
	//Default Constructor
	RootSys():pInitReg(), pParReg(), rootVolume(), numOfXSec(), rootWidthCurve(), rootWidth(), rootLen(), rootAngle(), M(), type(UNCLSSD){}//, is3DTip(false){}
	
	//Destructor
	~RootSys(){for(int i=0; i<latRoots.size(); i++) delete latRoots[i];}

	//Member Variables
	//bool is3DTip;
	Region* pInitReg;
	Region* pParReg;
	int rootVolume, numOfXSec, rootWidthCurve;
	double rootWidth, rootLen, rootAngle[4];
	class LatRoots: public vector<RootSys*>
	{
	 public:
		
		LatRoots():depth(0), length(0), numOfTips(0){}
		
		int depth;
		int length;
		int numOfTips;

	} latRoots;

	//Active Contour Model Points
	VECTOR parSnkPt;
	vector<VECTOR> acmpts;
	vector<PARAM_CURVE> curve;
	VECTOR** M;

	//Control Points from all sweeps
	vector<Region*> regs;
	vector<_3DCoor> sideCtrlPtsFront, sideCtrlPtsLeft;

	//Root Classification
	enum {UNCLSSD, PRIMARY, SEMINAL, LATERAL} type;
};

class ManualMerge
{
public:
	ManualMerge(Region::Skeleton* rskl, Region::Skeleton* tchskl, int rlts): pRootSkl(rskl), pTchSkl(tchskl), rlts(rlts){}

	Region::Skeleton* pRootSkl;
	Region::Skeleton* pTchSkl;
	int rlts;
};

//Global Extrernal Variables
extern vector<RootSys*> rootSysLst;
extern RootSys* rootSys;
extern stack<ManualMerge> manMrgRts;

//Function Declarations
void aggregate(_3DSize& stack);
DWORD WINAPI cmbCtrlPtsThread(LPVOID lpParam);