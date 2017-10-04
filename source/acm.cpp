//Macros
#undef  UNICODE
#define FREEGLUT_LIB_PRAGMAS 0//prevent freeglut.h from linking library

//System Dependencies
#include<Windows.h>
#include<CommCtrl.h>
#include<vector>
#include<cmath>

#include<iostream>

//Header Files
#include "..\headers\GL\freeglut.h"
#include "..\headers\viewer.h"
#include "..\headers\acm.h"
#include "..\headers\main.h"
#include "..\headers\dialog.h"
#include "..\headers\fractalise.h"
#include "..\headers\console.h"

//Global Variables
double alpha = 15;
double beta	 = 15;
double gamma = 1;
double delta = 40;
double dt  = 0.01;

int iterCount = 0;
bool runACM = false;
bool runCalibrate = false;

int ITER = 150;
int attractLow  = 1;
int attractHigh = 2;
int repulseLow  = 0;
int repulseHigh = 100;

//int ziplock;

volatile HANDLE acmMutex = NULL;

//Static Global Variables
static const int CTRL_TO_SNK_PTS_RATIO = 40;

//Function Definitions
//void CALLBACK TimeProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
vector<CUBF_PARAM> spline(vector<VECTOR> pline, double VECTOR::* cmpt);

void acmInit(RootSys& rtsys)
{
	VECTOR**& M = rtsys.M;

	//Initialise Active Contour Model with a straight line
	//connecting the origin of the roo and the root tip
	int nOfSPs = (int)rtsys.size()/CTRL_TO_SNK_PTS_RATIO;

	if(nOfSPs>=3)//ensure there are enough points to avoid array out of bound errors
	{
		WaitForSingleObject(acmMutex, INFINITE);

			//Reset Snake points and matricies
			if(M)
			{
				for(int i=0; i<rtsys.acmpts.size(); i++)
					if(M[i]) delete [] M[i];
				delete [] M, M = NULL;
			}

			rtsys.acmpts.clear();
			rtsys.curve.clear();

			//rtsys.acmpts.push_back(rtsys[0]);
			//for(int i=1; i<rtsys.size()-1; i+=(int)rtsys.size()/100)
			for(int i=0; i<=nOfSPs; i++)
				rtsys.acmpts.push_back(double(i)/(nOfSPs?nOfSPs:1)*(VECTOR(rtsys[rtsys.size()-1])-VECTOR(rtsys[0]))+VECTOR(rtsys[0]));
			//rtsys.acmpts.push_back(rtsys[rtsys.size()-1]);
	
			//Allocate matrix
			M = new VECTOR*[rtsys.acmpts.size()];

			for(int i=0; i<rtsys.acmpts.size(); i++)
				M[i] = new VECTOR[rtsys.acmpts.size()+1];

		ReleaseMutex(acmMutex);

		//Calculate the smoothing spline connecting the points
		vector<CUBF_PARAM> xpars = spline(rtsys.acmpts, &VECTOR::x);
		vector<CUBF_PARAM> ypars = spline(rtsys.acmpts, &VECTOR::y);
		vector<CUBF_PARAM> zpars = spline(rtsys.acmpts, &VECTOR::z);

		rtsys.curve.clear();
		for(int i=0; i<rtsys.acmpts.size()-1; i++)
			rtsys.curve.push_back(PARAM_CURVE(xpars[i], ypars[i], zpars[i]));
	}

	//clear connecting lines
	connLinesLen = 50;
	rtsys.parSnkPt = VECTOR();

	//ziplock  = (int)rtsys.acmpts.size();

	//MMRESULT timerID = NULL;
	//if(!timerID) timerID = timeSetEvent(40, 40, TimeProc, (DWORD_PTR)&rtsys, TIME_PERIODIC);

	for(int i=0; i<rtsys.latRoots.size(); i++)
		if(rtsys.latRoots[i]->size()>menuParam.prune)//DEBUG
			acmInit(*rtsys.latRoots[i]);
}

vector<CUBF_PARAM> spline(vector<VECTOR> pline, double VECTOR::* cmpt)
{
	vector<CUBF_PARAM> cubf;

	double* c = new double[pline.size()-1];
	double* d = new double[pline.size()];
	double* x = new double[pline.size()];
	
	c[0] = 0.5;
	d[0] = 3*(pline[1].*cmpt-pline[0].*cmpt)/2;

	int i=1;
	for(i; i<pline.size()-1; i++)
	{
		c[i] = 1/(4-c[i-1]);
		d[i] = (3*(pline[i+1].*cmpt-pline[i-1].*cmpt)-d[i-1])/(4-c[i-1]);
	}

	x[i] = d[i] = (3*(pline[i].*cmpt-pline[i-1].*cmpt)-d[i-1])/(4-c[i-1]);

	for(i--; i>=0; i--)
		x[i] = d[i]-c[i]*d[i+1];

	for(i=0; i<pline.size()-1; i++)
		cubf.push_back(CUBF_PARAM(2*(pline[i].*cmpt-pline[i+1].*cmpt)+d[i]+d[i+1],
								  3*(pline[i+1].*cmpt-pline[i].*cmpt)-2*d[i]-d[i+1],
								  d[i],
								  pline[i].*cmpt));
	delete [] c;
	delete [] d;
	delete [] x;

	return cubf;
}

double acmIter(RootSys& rtsys)
{
	vector<VECTOR>& points	   = rtsys.acmpts;
	vector<PARAM_CURVE>& curve = rtsys.curve;
	VECTOR**& M				   = rtsys.M;

	if(!M) return 0;

	//Initialise Matrix
	double p = beta*dt;
	double q = -alpha*dt-4*beta*dt;
	double r = 1+2*alpha*dt+6*beta*dt;

	M[0][0] = M[points.size()-1][points.size()-1] = VECTOR(1, 1, 1);

	M[1][0] = M[points.size()-2][points.size()-1] = VECTOR(p+q, p+q, p+q);
	M[1][1] = M[points.size()-2][points.size()-2] = VECTOR(r, r, r);
	M[1][2] = M[points.size()-2][points.size()-3] = VECTOR(p+q, p+q, p+q);
					
	for(int i=2; i<points.size()-2; i++)
	{
		M[i][i]   = VECTOR(r, r, r);

		M[i][i-1] = VECTOR(q, q, q);
		M[i][i+1] = VECTOR(q, q, q);
					
		M[i][i+2] = VECTOR(p, p, p);
		M[i][i-2] = VECTOR(p, p, p);
	}

	M[0][points.size()] = points[0];
	M[points.size()-1][points.size()] = points[points.size()-1];

	const int FORCE_REACH = 1;//10;
	const int FORCE_TRSLD = 5;//10;

	for(int i=1; i<(points.size()+1)/2; i++)
	{
		VECTOR f, r, v;

		//Attraction to control points
		for(int s=0; s<rtsys.regs.size(); s++)
			for(unsigned c=0; c<rtsys.regs[s]->centroids.size(); c++)
			{
				v = VECTOR(_3DCoor(rtsys.regs[s]->centroids[c], rtsys.regs[s]->d))-points[i];
				v = (v.abs()>10?v/(FORCE_REACH*dot(v, v)):v/v.abs()/FORCE_TRSLD);

				f = f + v;
			}

		for(unsigned s=0; s<rtsys.sideCtrlPtsFront.size(); s++)
		{
			v = VECTOR(rtsys.sideCtrlPtsFront[s])-points[i];
			v = (v.abs()>10?v/(FORCE_REACH*dot(v, v)):v/v.abs()/FORCE_TRSLD);

			f = f + v;
		}

		for(unsigned s=0; s<rtsys.sideCtrlPtsLeft.size(); s++)
		{
			v = VECTOR(rtsys.sideCtrlPtsLeft[s])-points[i];
			v = (v.abs()>10?v/(FORCE_REACH*dot(v, v)):v/v.abs()/FORCE_TRSLD);

			f = f + v;
		}

		//Internal repulsion between snake points
		for(int j=0; j<points.size(); j++)
			if(i!=j)
			{
				v = points[i]-points[j];
				v = v/dot(v, v);

				r = r + v;
			}

		//if(i<ziplock)//External Force
			M[i][points.size()] = points[i] + gamma*f + delta*r;
		//else
		//	M[i][points.size()] = points[i];

		f = r = VECTOR();//reset forces

		for(int s=0; s<rtsys.regs.size(); s++)
			for(unsigned c=0; c<rtsys.regs[s]->centroids.size(); c++)
			{
				v = VECTOR(_3DCoor(rtsys.regs[s]->centroids[c], rtsys.regs[s]->d))-points[points.size()-1-i];
				v = (v.abs()>10?v/(FORCE_REACH*dot(v, v)):v/v.abs()/FORCE_TRSLD);

				f = f + v;
			}

		for(unsigned s=0; s<rtsys.sideCtrlPtsFront.size(); s++)
		{
			v = VECTOR(rtsys.sideCtrlPtsFront[s])-points[points.size()-1-i];
			v = (v.abs()>10?v/(FORCE_REACH*dot(v, v)):v/v.abs()/FORCE_TRSLD);

			f = f + v;
		}

		for(unsigned s=0; s<rtsys.sideCtrlPtsLeft.size(); s++)
		{
			v = VECTOR(rtsys.sideCtrlPtsLeft[s])-points[points.size()-1-i];
			v = (v.abs()>10?v/(FORCE_REACH*dot(v, v)):v/v.abs()/FORCE_TRSLD);

			f = f + v;
		}

		for(int j=0; j<points.size(); j++)
			if(points.size()-1-i!=j)
			{
				v = points[points.size()-1-i]-points[j];
				v = v/dot(v, v);

				r = r + v;
			}

		//if(i<ziplock)//External Force
			M[points.size()-1-i][points.size()] = points[points.size()-1-i] + gamma*f + delta*r;
		//else
		//	M[points.size()-1-i][points.size()] = points[points.size()-1-i];
	}

	//Elementary Form
	for(int i=0; i<points.size(); i++)
	{
		for(int j=(int)points.size(); j>=i; j--)
			M[i][j] /= M[i][i];

		for(int j=i+1; j<points.size(); j++)
		{
			if(M[j][i].x)
			{
				for(int k=(int)points.size(); k>=i; k--)
					M[j][k].x /= M[j][i].x;

				for(int k=i; k<points.size()+1; k++)
					M[j][k].x = M[j][k].x-M[i][k].x;
			}

			if(M[j][i].y)
			{
				for(int k=(int)points.size(); k>=i; k--)
					M[j][k].y /= M[j][i].y;

				for(int k=i; k<points.size()+1; k++)
					M[j][k].y = M[j][k].y-M[i][k].y;
			}

			if(M[j][i].z)
			{
				for(int k=(int)points.size(); k>=i; k--)
					M[j][k].z /= M[j][i].z;

				for(int k=i; k<points.size()+1; k++)
					M[j][k].z = M[j][k].z-M[i][k].z;
			}
		}
	}

	//Echelon Form
	for(int i=(int)points.size()-1; i>0; i--)
		for(int j=i-1; j>=0; j--)
			for(int k=(int)points.size(); k>=0; k--)
				M[j][k] += -M[j][i]*M[i][k];

	//Update value of new points
	WaitForSingleObject(acmMutex, INFINITE);

		for(int i=0; i<points.size(); i++)
			points[i] = M[i][points.size()];
		
	ReleaseMutex(acmMutex);
	
	//Calculate the smoothing spline connecting the points
	vector<CUBF_PARAM> xpars = spline(points, &VECTOR::x);
	vector<CUBF_PARAM> ypars = spline(points, &VECTOR::y);
	vector<CUBF_PARAM> zpars = spline(points, &VECTOR::z);

	WaitForSingleObject(acmMutex, INFINITE);

		curve.clear();
		for(int i=0; i<points.size()-1; i++)
			curve.push_back(PARAM_CURVE(xpars[i], ypars[i], zpars[i]));

	ReleaseMutex(acmMutex);

	double mse = 0;

	for(int i=0; i<rtsys.latRoots.size(); i++)
		if(rtsys.latRoots[i]->size()>menuParam.prune)//DEBUG
		{
			//Compute Connecting Lines Origin Snake Point
			double minDist = 1e100;
			double dist;

			for(int p=0; p<rtsys.acmpts.size(); p++)
				if((dist=(VECTOR((*rtsys.latRoots[i])[0])-rtsys.acmpts[p]).abs())<minDist)
				{
					minDist = dist;
					rtsys.latRoots[i]->parSnkPt = rtsys.acmpts[p];
				}

			mse += acmIter(*rtsys.latRoots[i]);
		}

	for(int a=0; a<rtsys.acmpts.size(); a++)
	{
		double dist, minDist=1e100;

		for(int r=0; r<rtsys.regs.size(); r++)
			for(int c=0; c<rtsys.regs[r]->centroids.size(); c++)
				if((dist=(rtsys.acmpts[a]-_3DCoor(rtsys.regs[r]->centroids[c], rtsys.regs[r]->d)).dot())<minDist)
					minDist = dist;

		for(unsigned s=0; s<rtsys.sideCtrlPtsFront.size(); s++)
			if((dist=(rtsys.acmpts[a]-rtsys.sideCtrlPtsFront[s]).dot())<minDist)
				minDist = dist;

		for(unsigned s=0; s<rtsys.sideCtrlPtsLeft.size(); s++)
			if((dist=(rtsys.acmpts[a]-rtsys.sideCtrlPtsLeft[s]).dot())<minDist)
				minDist = dist;

		mse += minDist;
	}

	return mse;
}

//void CALLBACK TimeProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
DWORD WINAPI ACMThreadProc(LPVOID lpParam)
{
	RootSys& rtsys = *(RootSys*)lpParam;

	double minMSE = 1e100;
	double optGamma, optDelta;

	char str[256] = {};

	cout<<endl<<endl;
	cout<<"Calibrating ACM Attracion and Repulsion parameters"<<endl<<"Search Space:"<<endl;
	cout<<"   Attraction Force = ("<<attractLow<<" - "<<attractHigh<<")"<<endl;
	cout<<"   Repulsion  Force = ("<<repulseLow<<" - "<<repulseHigh<<")"<<endl;
	cout<<"   Iterations per ACM run = "<<ITER<<endl<<endl;
	
	for(int g=attractLow; g<=attractHigh; g++)
		for(int d=repulseLow; d<=repulseHigh; d+=10)
		{
			gamma = g;
			delta = d;

			acmInit(rtsys);

			double minmse = 1e100;

			//Refresh the Force and Repulsion coefficients in the Control Dialog Status Bar
			wsprintf(str, "Attraction = %i, Repulsion = %i", (int)g, (int)d);
			SendMessage(GetDlgItem(hCtrlDlg, IDS_STATUSBAR), SB_SETTEXT, MAKEWPARAM(0, 0), (LPARAM)str);

			for(iterCount=0; iterCount<ITER; iterCount++)
			{
				double mse = sqrt(acmIter(rtsys));

				if(mse<minmse) minmse = mse;

				//Refresh the Iter Counter in the Control Dialog Status Bar
				wsprintf(str, "Iteration = %i", iterCount);
				SendMessage(GetDlgItem(hCtrlDlg, IDS_STATUSBAR), SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM)str);

				if(!runCalibrate)
				{
					cout<<endl<<endl<<'>';
					return 0;
				}
			}

			if(minmse<minMSE)
			{
				minMSE = minmse;

				optGamma = g;
				optDelta = d;
			}

			cout<<gamma<<", "<<delta<<" - "<<minmse<<endl;
		}

	gamma = optGamma;
	delta = optDelta;

	acmInit(rtsys);
	for(iterCount=0; iterCount<ITER; iterCount++) acmIter(rtsys);

	connLinesLen = -1;

	cout<<endl<<endl;
	cout<<"Minimum MSE = "<<minMSE<<endl;
	cout<<"Optimal Force = "<<optGamma<<endl;
	cout<<"Optimal Repulsion = "<<optDelta<<endl;
	cout<<endl<<'>';

	//Set Calibrated values for
	//attraction and repulsion in the Ctrl Dialog

	wsprintf(str, "%i", int(gamma));
	SetDlgItemText(hCtrlDlg, IDC_ATTRACT, str);

	wsprintf(str, "%i", int(delta));
	SetDlgItemText(hCtrlDlg, IDC_REPULSE, str);

	statsOutput(imgStack, false, !buttons.classification, !buttons.classification);
	sendConsoleCommand("print");

	EnableMenuItem(GetMenu(hMainWnd), ID_FILE_CLOSE, MF_BYCOMMAND + MF_ENABLED);

	buttons.classification = true;

	buttons.start = true;
	buttons.stop  = true;
	buttons._auto = true;
	buttons.reset = true;

	buttons.acmParam = true;

	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

	if(glutWnd) glutTimerFunc(50, updateTitle, 0);

	return 0;
}

DWORD WINAPI ACMThreadIter(LPVOID lpParam)
{
	RootSys& rtsys = *(RootSys*)lpParam;

	while(runACM)
	{
		acmIter(rtsys);

		iterCount++;

		//Refresh the Iter Counter in the Control Dialog Status Bar
		char str[256] = {};
		wsprintf(str, "Iteration = %i", iterCount);
		SendMessage(GetDlgItem(hCtrlDlg, IDS_STATUSBAR), SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM)str);
	}

	connLinesLen = -1;

	statsOutput(imgStack, false, !buttons.classification, !buttons.classification);
	sendConsoleCommand("print");

	buttons.classification = true;

	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

	return 0;
}