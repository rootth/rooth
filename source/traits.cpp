//Macros
#define _USE_MATH_DEFINES

//System Dependencies
#include<algorithm>
#include<vector>
#include<cmath>

//Header Files
#include "../headers/main.h"
#include "../headers/traits.h"
#include "../headers/fractalise.h"

using namespace std;

//Global Variables
unsigned rtClssTrshld = 0;//in mm

//Global Static Variables
static vector<unsigned> lstOfRtLen;

void rootTraits(RootSys& rtsys, bool updateClassification)
{	
	//Verticle Angle Measurement
	/*unsigned r;
	for(r=0; r<rtsys.size(); r++)
		if(rtsys[r].z-rtsys[0].z > int(1000/res))//1 cm depth before measuring the root angle
			break;
	r--; //subtract 1 to avoid array index out of range (the for loop will make r equal to the size of the array if the root is shorter then 1cm)*/

	/*double dx = (int)rtsys[rtsys.size()/2].x-(int)rtsys[0].x;
	double dy = (int)rtsys[rtsys.size()/2].y-(int)rtsys[0].y;
	double dz = (int)rtsys[rtsys.size()/2].z-(int)rtsys[0].z;
	double dxdy = sqrt(dx*dx+dy*dy);*/
	/*double dx = (int)rtsys[rtsys.size()-1].x-(int)rtsys[0].x;
	double dy = (int)rtsys[rtsys.size()-1].y-(int)rtsys[0].y;
	double dz = (int)rtsys[rtsys.size()-1].z-(int)rtsys[0].z;
	double distxyz = sqrt(dx*dx+dy*dy+dz*dz);*/
	
	//ensure dxdy>1 to avoid division by 0
	//double angleVer = atan(dxdy>1?dz/dxdy:0)*180/M_PI;
	//rtsys.rootAngle = (distxyz>0?acos(dz/distxyz)*180/M_PI:0);

	if(updateClassification && rtsys.size()) rtsys.type = (menuParam.res*rtsys.size()/1000>rtClssTrshld?RootSys::SEMINAL:RootSys::LATERAL);

	//Length calculated using the splines
	const double dt = 0.0001;
	double length = 0;

	for(int s=0; s<rtsys.curve.size(); s++)
		for(double t=0; t<1; t+=dt)
			length += sqrt(pow(3*rtsys.curve[s].xp.a*t*t+2*rtsys.curve[s].xp.b*t+rtsys.curve[s].xp.c,2)+
						   pow(3*rtsys.curve[s].yp.a*t*t+2*rtsys.curve[s].yp.b*t+rtsys.curve[s].yp.c,2)+
						   pow(3*rtsys.curve[s].zp.a*t*t+2*rtsys.curve[s].zp.b*t+rtsys.curve[s].zp.c,2))*dt;

	int angleAtLen = 0;
	double len = 0;

	for(int s=0; s<rtsys.curve.size(); s++)
		for(double t=0; t<1; t+=dt)
		{
			VECTOR coor(rtsys.curve[s].xp.a*t*t*t+rtsys.curve[s].xp.b*t*t+rtsys.curve[s].xp.c*t+rtsys.curve[s].xp.d,
						rtsys.curve[s].yp.a*t*t*t+rtsys.curve[s].yp.b*t*t+rtsys.curve[s].yp.c*t+rtsys.curve[s].yp.d,
						rtsys.curve[s].zp.a*t*t*t+rtsys.curve[s].zp.b*t*t+rtsys.curve[s].zp.c*t+rtsys.curve[s].zp.d);

			len += sqrt(pow(3*rtsys.curve[s].xp.a*t*t+2*rtsys.curve[s].xp.b*t+rtsys.curve[s].xp.c,2)+
						pow(3*rtsys.curve[s].yp.a*t*t+2*rtsys.curve[s].yp.b*t+rtsys.curve[s].yp.c,2)+
						pow(3*rtsys.curve[s].zp.a*t*t+2*rtsys.curve[s].zp.b*t+rtsys.curve[s].zp.c,2))*dt;

			if(len>=0.1*length && angleAtLen==0)//Root Angle at 10% of root length
			{
				VECTOR d = coor-rtsys.acmpts[0];

				rtsys.rootAngle[angleAtLen] = (d.abs()>0?180*acos(d.z/d.abs())/M_PI:0);
				angleAtLen++;
			}

			if(len>=0.25*length && angleAtLen==1)//Root Angle at 25% of root length
			{
				VECTOR d = coor-rtsys.acmpts[0];

				rtsys.rootAngle[angleAtLen] = (d.abs()>0?180*acos(d.z/d.abs())/M_PI:0);
				angleAtLen++;
			}

			if(len>=0.5*length && angleAtLen==2)//Root Angle at 50% of root length
			{
				VECTOR d = coor-rtsys.acmpts[0];

				rtsys.rootAngle[angleAtLen] = (d.abs()>0?180*acos(d.z/d.abs())/M_PI:0);
				angleAtLen++;
			}

			if(len>=length && angleAtLen==3)//Root Angle at 100% of root length
			{
				VECTOR d = coor-rtsys.acmpts[0];

				rtsys.rootAngle[angleAtLen] = (d.abs()>0?180*acos(d.z/d.abs())/M_PI:0);
				angleAtLen++;
			}
		}

	if(rtsys.parSnkPt.x>0 && rtsys.parSnkPt.y>0 && rtsys.parSnkPt.z>0 && rtsys.acmpts.size())
		length += (rtsys.parSnkPt-rtsys.acmpts[0]).abs();
	rtsys.rootLen = length;

	double avgWidth = 0;

	for(int p=0; p<rtsys.acmpts.size(); p++)
	{
		double minWidth = 1e100;

		for(int r=0; r<rtsys.regs.size(); r++)
			for(int w=0; w<rtsys.regs[r]->wall.size(); w++)
			{
				double width = (rtsys.acmpts[p]-_3DCoor(rtsys.regs[r]->wall[w], rtsys.regs[r]->d)).abs();

				if(minWidth>width) minWidth = width;
			}

		avgWidth += minWidth;
	}

	if(rtsys.acmpts.size())
	{
		avgWidth /= rtsys.acmpts.size();
		avgWidth *= 2;//convert radius to diameter;

		rtsys.rootWidth = avgWidth;
	}

	for(int i=0; i<rtsys.latRoots.size(); i++)
		if(rtsys.latRoots[i]->size()>menuParam.prune || (!rtsys.size() && rtsys.latRoots[i]->acmpts.size()))//DEBUG
			rootTraits(*rtsys.latRoots[i], updateClassification);
}

void rtlenlst(const RootSys* rtsys)
{
	for(int i=0; i<rtsys->latRoots.size(); i++)
		if(rtsys->latRoots[i]->size()>menuParam.prune)//DEBUG
			rtlenlst(rtsys->latRoots[i]);
	
	lstOfRtLen.push_back(unsigned(rtsys->size()));
}

unsigned rtcount(const RootSys* rtsys)
{
	unsigned count = 0;

	for(int i=0; i<rtsys->latRoots.size(); i++)
		if(rtsys->latRoots[i]->size()>menuParam.prune)//DEBUG
			count += rtcount(rtsys->latRoots[i]);
	
	return 1 + count;	
}

void classifyRoots(RootSys* rtsys)
{
	lstOfRtLen.clear();
	rtlenlst(rtsys);
	sort(lstOfRtLen.begin(), lstOfRtLen.end());

	if(lstOfRtLen.size()<2) return;

	int index = 0;
	double minSE = 1e100;

	for(int i=3; i<=lstOfRtLen.size()-2; i++)
	{
		long long sumX=0, sumY=0, sumXY=0, sumX2=0;

		for(int x=0; x<i; x++)
		{
			sumX += x;
			sumY += lstOfRtLen[x];
			sumXY += x*lstOfRtLen[x];
			sumX2 += x*x;
		}

		double a = double(i*sumXY-sumX*sumY)/(i*sumX2-sumX*sumX);
		double b = double(sumY-a*sumX)/i;

		double estYsum = 0;

		for(int x=0; x<i; x++) estYsum += pow(a*x+b-lstOfRtLen[x], 2);
		
		sumX=0, sumY=0, sumXY=0, sumX2=0;

		for(int x=i; x<lstOfRtLen.size(); x++)
		{
			sumX += x;
			sumY += lstOfRtLen[x];
			sumXY += x*lstOfRtLen[x];
			sumX2 += x*x;
		}

		a = double((lstOfRtLen.size()-i)*sumXY-sumX*sumY)/((lstOfRtLen.size()-i)*sumX2-sumX*sumX);
		b = double(sumY-a*sumX)/(lstOfRtLen.size()-i);

		for(int x=i; x<lstOfRtLen.size(); x++) estYsum += pow(a*x+b-lstOfRtLen[x], 2);

		double stdErr = sqrt(estYsum/(i-2));

		if(minSE>stdErr)
		{
			index = i;
			minSE = stdErr;
		}
	}

	double meanLeft = 0, meanRight = 0;

	for(int x=0; x<index; x++)
		meanLeft += lstOfRtLen[x];
	meanLeft /= index;

	for(int x=index; x<lstOfRtLen.size(); x++)
		meanRight += lstOfRtLen[x];
	meanRight /= lstOfRtLen.size()-index;

	if(index>1)
	{
		if(abs(lstOfRtLen[index]-meanRight)<abs(lstOfRtLen[index]-meanLeft)) index--;

		rtClssTrshld = menuParam.res*lstOfRtLen[index]/1000;//threshold is in mm units
	}
}