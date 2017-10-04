//Definitions
//#define _USE_MATH_DEFINES

//System Dependencies
#include<Windows.h>
#include<Commctrl.h>
#include<cmath>

//Header Files
#include "../headers/fractalise.h"
#include "../headers/viewer.h"
#include "../headers/traits.h"
#include "../headers/dialog.h"
#include "../headers/main.h"
//#include "../headers/debug.h"

//Global variables
vector<RootSys*> rootSysLst;
RootSys* rootSys = NULL;
stack<ManualMerge> manMrgRts;

//Function Definitions
static unsigned rtlen(RootSys& rtsys)
{
	for(int i=0; i<rtsys.latRoots.size(); i++)
		rtsys.latRoots.length += rtlen(*rtsys.latRoots[i]);

	rtsys.rootWidthCurve /= rtsys.numOfXSec;
	//rtsys.rootWidth = int(rtsys.rootWidth/((double)rtsys.size()/rtsys.numOfXSec));
	
	return (unsigned)rtsys.size() + rtsys.latRoots.length;	
}

static void initprs(RootSys* rtsys)
{
	for(int r=0; r<rtsys->size(); r++)
		if((*rtsys)[r].pskl) (*rtsys)[r].pskl->prtsys = rtsys;

	for(int i=0; i<rtsys->latRoots.size(); i++)
		initprs(rtsys->latRoots[i]);
	
	return;
}

/*static void rmvFlsPst3DTips(RootSys& rtsys)
{
	for(int i=0; i<rtsys.latRoots.size(); i++)
		if(!rtsys.latRoots[i].is3DTip)
			rtsys.latRoots.erase(rtsys.latRoots.begin()+i--);//i-- is used to go back one element since the current one is removed
		else
			rmvFlsPst3DTips(rtsys.latRoots[i]);
}*/

static void cmbSideCtrlPts(RootSys& rtsys, ImageStack& stack, int range, int& count)
{
	rtsys.sideCtrlPtsFront.clear();
	rtsys.sideCtrlPtsLeft.clear();
	
	for(unsigned r=0; r<rtsys.regs.size(); r++)
		for(unsigned v=0; v<rtsys.regs[r]->size(); v++)
			stack[rtsys.regs[r]->d][(*rtsys.regs[r])[v].x][(*rtsys.regs[r])[v].y] = 0xFF;

	for(unsigned d=1; d<imgStack.height-1; d++)
		for(unsigned r=0; r<rootsFront[d].size(); r++)
			for(unsigned v=0; v<rootsFront[d][r].size(); v++)
				if(stack[rootsFront[d][r][v].x][d][rootsFront[d][r][v].y])
				{
					for(unsigned c=0; c<rootsFront[d][r].centroids.size(); c++)
						rtsys.sideCtrlPtsFront.push_back(_3DCoor(d, rootsFront[d][r].centroids[c].y, rootsFront[d][r].centroids[c].x));

					//continue to next region
					v = (unsigned)rootsFront[d][r].size();
				}

	for(unsigned d=1; d<imgStack.width-1; d++)
		for(unsigned r=0; r<rootsLeft[d].size(); r++)
			for(unsigned v=0; v<rootsLeft[d][r].size(); v++)
				if(stack[rootsLeft[d][r][v].x][rootsLeft[d][r][v].y][d])
				{
					for(unsigned c=0; c<rootsLeft[d][r].centroids.size(); c++)
						rtsys.sideCtrlPtsLeft.push_back(_3DCoor(rootsLeft[d][r].centroids[c].y, d, rootsLeft[d][r].centroids[c].x));

					//continue to next region
					v = (unsigned)rootsLeft[d][r].size();
				}

	for(unsigned r=0; r<rtsys.regs.size(); r++)
		for(unsigned v=0; v<rtsys.regs[r]->size(); v++)
			stack[rtsys.regs[r]->d][(*rtsys.regs[r])[v].x][(*rtsys.regs[r])[v].y] = 0;

	for(unsigned l=0; l<rtsys.latRoots.size(); l++)
		if(rtsys.latRoots[l]->size()>menuParam.prune)//DEBUG
			cmbSideCtrlPts(*rtsys.latRoots[l], stack, range, count);

	SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSMERGE), PBM_SETPOS, int(SCROLL_RANGE*(double(range- --count)/range)), 0);
}

DWORD WINAPI cmbCtrlPtsThread(LPVOID lpParam)
{
	RootSys& rtsys = *(RootSys*)lpParam;

	int count = rtcount(&rtsys);
	ImageStack tmpImageStack(*(_3DSize*)imgStackTop);

	buttons.merge = false;
	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);
	
	cmbSideCtrlPts(rtsys, tmpImageStack, count, --count);
	
	merged = true;
	buttons.merge = true;
	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

	return 0;
}

static RootSys* rootPath(Region* reg, _3DSize& stack)
{
	RootSys* rtsys = new RootSys;
	Region::Skeleton* skl = &reg->skl;

	//Fit an ellipse around every cross section
	double maxVariance = 0;

	for(unsigned i=0; i<reg->wall.size(); i++)
		for(unsigned j=i+1; j<reg->wall.size(); j++)
		{
			double diffX = (double)reg->wall[i].x - (double)reg->wall[j].x;
			double diffY = (double)reg->wall[i].y - (double)reg->wall[j].y;

			double dist = sqrt(diffX*diffX+diffY*diffY);

			if(maxVariance<dist)
			{
				maxVariance = dist;
				reg->pA = reg->wall[i];
				reg->pB = reg->wall[j];
			}
		}
	
	maxVariance = 0;

	for(unsigned i=0; i<reg->wall.size(); i++)
	{
		double a = sqrt(double((reg->pA.x-reg->pB.x)*(reg->pA.x-reg->pB.x)+(reg->pA.y-reg->pB.y)*(reg->pA.y-reg->pB.y)));
		double b = sqrt(double((reg->pB.x-reg->wall[i].x)*(reg->pB.x-reg->wall[i].x)+(reg->pB.y-reg->wall[i].y)*(reg->pB.y-reg->wall[i].y)));
		double c = sqrt(double((reg->pA.x-reg->wall[i].x)*(reg->pA.x-reg->wall[i].x)+(reg->pA.y-reg->wall[i].y)*(reg->pA.y-reg->wall[i].y)));

		double A = 0.25*sqrt((a+b-c)*(a-b+c)*(-a+b+c)*(a+b+c));
		
		double dist = 2*A/a;

		if(maxVariance<dist)
		{
			maxVariance = dist;
			reg->pC = reg->wall[i];
		}
	}

	//rtsys.is3DTip	 = skl->is3DTip;
	rtsys->numOfXSec = 1;
	rtsys->rootWidthCurve = (int)(2*maxVariance);//int(reg->wall.size()/M_PI);
	rtsys->rootVolume = (int)reg->size();

	unsigned maxLen = 0;
	RootSys* maxLenRtSys = NULL;
	vector<RootSys*> lstOfRtSys;

	while(true)
	{
		vector<Region*> lstOfReg;
		vector<RootSkelVox::NbrSeg> nbrSeg;
		//vector<vector<_3DCoor>> nbrSeg;

		skl->visited = true;

		for(unsigned p=0; p<skl->path.size(); p++)
			if(!skl->path[p]->skl.visited)
				lstOfReg.push_back(skl->path[p]);

		for(unsigned p=0; p<skl->parent.size(); p++)
			if(!skl->parent[p]->skl.visited)
				lstOfReg.push_back(skl->parent[p]);

		if(lstOfReg.size()>1)
		{
			//If the user has selected the correct branch, use it instead and add it first to the list
			if(skl->nbrSegChoice)//>=0)
				for(int r=0; r<lstOfReg.size(); r++)
					if(lstOfReg[r]==skl->nbrSegChoice)
						lstOfRtSys.push_back(rootPath(lstOfReg[r/*skl->nbrSegChoice*/], stack));

			//Enumerate through all remaining branches not selected by the user
			//if non has been selected then nbrSegChoice is -1 and the for loop
			//will go through all root branches
			for(int r=0; r<lstOfReg.size(); r++)
				//if the next branch is unvisited then call recursively rootPath() to construct a subtree
				if(lstOfReg[r]!=skl->nbrSegChoice && !skl->latRootSkip.count(lstOfReg[r]) && !lstOfReg[r]->skl.visited)
				{
					RootSys* rs = rootPath(lstOfReg[r], stack);

					rs->pInitReg = lstOfReg[r];
					lstOfRtSys.push_back(rs);

					if(rs->size()>menuParam.prune)
					{
						vector<_3DCoor> v;

						for(int i = 0; i<rs->size(); i++)
						{
							//only mark the roots up to the next segment
							if((*rs)[i].nbrSeg.size()>1) break;

							v.push_back((*rs)[i]);
						}

						nbrSeg.push_back(RootSkelVox::NbrSeg(v, lstOfReg[r]));
					}
				}
				else//if the branch is already visisted it means there's a loop - highlight only the next segment
				{
					Region::Skeleton *s = &lstOfReg[r]->skl, *prevS = skl;
					vector<_3DCoor> v;

					while(true)
					{
						for(int l=0; l<s->line.size(); l++)
							v.push_back(_3DCoor(s->line[l], s->z));
						v.push_back(*s);
						
						prevS = s;
						if(s->parent.size()==1 && s->path.size()==1)
						{
							if(&s->path[0]->skl != prevS)
								s = &s->path[0]->skl;
							if(&s->parent[0]->skl != prevS)
								s = &s->parent[0]->skl;
						}
						else
							break;
					}

					nbrSeg.push_back(RootSkelVox::NbrSeg(v, lstOfReg[r]));
				}
			
			//if the user has selected the next branch choose it as
			//maxLenRtSys even if it doesn't have the highest length
			if(skl->nbrSegChoice)//>=0)
				maxLenRtSys = lstOfRtSys[0];
		}

		for(int l=(int)skl->line.size()-1; l>=0; l--)
			rtsys->push_back(_3DCoor(skl->line[l], skl->z));
		rtsys->push_back(RootSkelVox(skl, nbrSeg));
		rtsys->regs.push_back(reg);

		if(lstOfReg.size()==1)
			skl = &(reg=lstOfReg[0])->skl;
		else
			break;
	}

	//if the user hasn't set the next branch (i.e. maxLenRtSys is NULL)
	//then select the branch with the longest length
	if(!maxLenRtSys)
		for(unsigned s=0; s<lstOfRtSys.size(); s++)
			if(maxLen<lstOfRtSys[s]->size())
			{
				maxLen = (unsigned)lstOfRtSys[s]->size();
				maxLenRtSys = lstOfRtSys[s];
			}

	if(maxLenRtSys)
	{
		(*rtsys)[rtsys->size()-1].pNextReg = maxLenRtSys->pInitReg;
		maxLenRtSys->pInitReg = NULL;
		
		maxLenRtSys->numOfXSec += rtsys->numOfXSec;
		maxLenRtSys->rootWidthCurve += rtsys->rootWidthCurve;
		maxLenRtSys->rootVolume += rtsys->rootVolume;
		//rtsys.is3DTip = lstOfRtSys[s].is3DTip;

		maxLenRtSys->insert(maxLenRtSys->/*end*/begin(), rtsys->begin(), rtsys->end());
		maxLenRtSys->latRoots.insert(maxLenRtSys->latRoots.end(), rtsys->latRoots.begin(), rtsys->latRoots.end());
		maxLenRtSys->regs.insert(maxLenRtSys->regs.end(), rtsys->regs.begin(), rtsys->regs.end());

		rtsys->latRoots.clear();
		delete rtsys;

		rtsys = maxLenRtSys;
	}

	for(unsigned s=0; s<lstOfRtSys.size(); s++)
		if(rtsys != lstOfRtSys[s])
		{
			lstOfRtSys[s]->pParReg = reg;
			rtsys->latRoots.push_back(lstOfRtSys[s]);
		}

	return rtsys;
}

static DWORD WINAPI rootPathThread(void* param)
{
	ImageStack& stack = *(ImageStack*)param;
	vector<Region>* roots = rootsTop;//DEBUG set to rootsLeft/rootFront for multi-sweep version
	
	WaitForSingleObject(skelMutex, INFINITE);

		//Clear regions 'visited' state to ensure aggregate() is a re-entrant function
		for(unsigned d=1; d<stack.depth-1; d++)
			for(int r=0; r<roots[d].size(); r++)
			{
				roots[d][r].skl.visited = false;
				roots[d][r].skl.prtsys  = NULL;
			}
		
		for(int i=0; i<rootSysLst.size(); i++)
			delete rootSysLst[i];
		rootSysLst.clear();
		rootSys = NULL;

		for(unsigned d=1; d<stack.depth-1; d++)
			if(roots[d].size())
			{
				for(int r=0; r<roots[d].size(); r++)
				{
					Region* reg = &roots[d][r];
					RootSys* rs = rootPath(reg, *imgStackTop);

					rs->pInitReg = reg;
					rootSysLst.push_back(rs);
				}

				break;
			}

		for(int r=0; r<rootSysLst.size(); r++) initprs(rootSysLst[r]);

		for(int i=0; i<manMrgRts.size(); i++)
		{
			if(!manMrgRts._Get_container()[i].pRootSkl || ! manMrgRts._Get_container()[i].pRootSkl->prtsys || !manMrgRts._Get_container()[i].pTchSkl) continue;

			manMrgRts._Get_container()[i].pRootSkl->prtsys->insert
				(manMrgRts._Get_container()[i].pRootSkl->prtsys->end(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->end());

			manMrgRts._Get_container()[i].pRootSkl->prtsys->latRoots.insert
				(manMrgRts._Get_container()[i].pRootSkl->prtsys->latRoots.begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->latRoots.begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->latRoots.end());

			manMrgRts._Get_container()[i].pRootSkl->prtsys->regs.insert
				(manMrgRts._Get_container()[i].pRootSkl->prtsys->regs.begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->regs.begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->regs.end());

			manMrgRts._Get_container()[i].pRootSkl->prtsys->sideCtrlPtsFront.insert
				(manMrgRts._Get_container()[i].pRootSkl->prtsys->sideCtrlPtsFront.begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->sideCtrlPtsFront.begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->sideCtrlPtsFront.end());

			manMrgRts._Get_container()[i].pRootSkl->prtsys->sideCtrlPtsLeft.insert
				(manMrgRts._Get_container()[i].pRootSkl->prtsys->sideCtrlPtsLeft.begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->sideCtrlPtsLeft.begin(),
				 manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->sideCtrlPtsLeft.end());

			manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts]->latRoots.clear();
			delete manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots[manMrgRts._Get_container()[i].rlts];

			manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots.erase
				(manMrgRts._Get_container()[i].pTchSkl->prtsys->latRoots.begin() + manMrgRts._Get_container()[i].rlts);

			for(int r=0; r<manMrgRts._Get_container()[i].pRootSkl->prtsys->size(); r++)
				if((*manMrgRts._Get_container()[i].pRootSkl->prtsys)[r].pskl)
					(*manMrgRts._Get_container()[i].pRootSkl->prtsys)[r].pskl->prtsys = manMrgRts._Get_container()[i].pRootSkl->prtsys;
		}

		unsigned maxRtSysLen = 0, maxRtSysInd = 0;

		for(int r=0; r<rootSysLst.size(); r++)
		{			
			unsigned rtsyslen;

			if((rtsyslen = rtlen(*rootSysLst[r]))>maxRtSysLen)
			{
				maxRtSysLen = rtsyslen;
				maxRtSysInd = r;
			}
		}

		if(rootSysLst.size()) rootSys = rootSysLst[maxRtSysInd];

	ReleaseMutex(skelMutex);

	return 0;
}

//MIGHT NOT NEED THE IMAGE STACK VARIABLE
//AS A FUNCTION PARAMETER
void aggregate(_3DSize& stack)
{
	if(!skelMutex) skelMutex = CreateMutex(NULL, false, NULL);

	WaitForSingleObject(CreateThread(NULL, 16777216, rootPathThread, (LPVOID)&stack, 0, NULL), INFINITE);

	//rmvFlsPst3DTips(rootSys);

	/*ImageStack tmpImageStack(*(_3DSize*)imgStackTop);

	cmbSideCtrlPts(rootSys, tmpImageStack);*/

	//DEBUG
	//level = 10;//rootSys.latRoots.depth;
}