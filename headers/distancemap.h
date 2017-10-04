#pragma once

//System Dependencies
#include<vector>
#include<set>
#include<cmath>

using namespace std;

//Header Files
#include "main.h"
#include "imagestack.h"
//#include "debug.h"

//Class Definitions
class RootSys;

class Region: public vector<_2DCoor>, public _2DSize, public _2DBase
{
 public:
	//Constructor
	 Region(vector<_2DCoor> v): vector(v) {}

	//Member Variables
	vector<_2DCoor> wall;
	vector<_2DCoor> centroids;
	unsigned d;

	//Cross-Section Ellipse
	_2DCoor pA, pB, pC;

	//IMPLEMENT LINE DRAWING ALGORITHM IN drawLine()
	class Skeleton: public _3DCoor
	{
	 public:
		//Variables
		int  k;
		int length;
		bool isTip;
		//bool is3DTip;
		bool isDownward;
		bool visited;
		RootSys* prtsys;
		Region* nbrSegChoice;
		set<Region*> latRootSkip;
		vector<Region*> parent;
		vector<Region*> path;
		vector<_2DCoor> line;
		vector<_2DCoor> mse;

		//Default Constructor
		Skeleton(): k(rand()%menuParam.K), length(), isTip(), isDownward(), visited(), nbrSegChoice(), prtsys() /*is3DTip()*/{}

		//Functions
		void setCoor(const _2DCoor& c, unsigned short z){_2DCoor::operator=(c); this->z=z;}
		void setCoor(const _3DCoor& c){_3DCoor::operator=(c);}
		void drawLine(vector<_2DCoor>& line, _2DCoor c);
		void drawLineRoot(_2DCoor c){drawLine(line, c);}
		void drawLineMSE(_2DCoor c){drawLine(mse, c);}
		void clear()
		{
			if(!isTip)
			{
				length = 0;
				setCoor(_3DCoor());
			}
			
			path.clear();
		}
		
	} skl;
};

//Class Definitions
template<typename type> class Board: public vector<type>, public _2DSize, public _2DBase
{
 public:
	void resize(const _2DSize& size)
	{
		if(height!=size.height+2 || width!=size.width+2)
		{
			height = size.height+2;
			width = size.width+2;
			
			if(height*width>vector::size())
				vector::resize(height*width, 0);
		}
	}
	
	type* operator[](const int index)
	{
		return (type*)data() + (index+1)*width + 1;
	}
};

//Global Variables
extern vector<Region> *rootsTop, *rootsFront, *rootsLeft;

//Function Declarations
void distanceMap(vector<Region>* roots, ImageStack& imgStack);