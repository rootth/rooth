//Definitions
#define _USE_MATH_DEFINES

//System Dependencies
#include<cmath>
#include<set>

//Header Files
#include "../headers/skeletonisation.h"
#include "../headers/distancemap.h"
#include "../headers/main.h"
//#include "../headers/debug.h"

//USE HIGHT WND WIDTH OF THE ROOT INSTEAD OF PRE-SET VALUES
unsigned RADIAL_DIST  = 15;

//Global Variables
double minCoeff;
unsigned long long minMSE;

//Function Definitions
int round(double f) {return int(f>0?f+0.5:f-0.5);}
unsigned long long crossValidation(ImageStack& stack, vector<Region>* roots, double coeff, int k);

void skeletonise(ImageStack& stack)
{	
	vector<Region>* roots = rootsTop;
	
	//Initialise Optimal Path Algorithm
	unsigned d;

	for(d=stack.depth-2; d>=1; d--)
	{
		for(int r=0; r<roots[d].size(); r++)
		{
			roots[d][r].skl.isTip = true;
			//roots[d][r].skl.is3DTip = true;
			roots[d][r].skl.setCoor(roots[d][r].centroids[0], d);
			roots[d][r].skl.length = 1;
		}

		if(roots[d].size()) break;
	}

	Board<unsigned> parBrd;
	parBrd.resize(stack);

	//Clear Board
	for(unsigned h=0; h<stack.height; h++)
		for(unsigned w=0; w<stack.width; w++)
			parBrd[h][w] = 0;

	for(d--; d>=1; d--)
	{
		//Detect all root tips
		for(unsigned rA=0; rA<roots[d].size(); rA++)
		{
			unsigned v = stack[d][roots[d][rA][0].x][roots[d][rA][0].y];
				
			for(unsigned i=1; i<roots[d][rA].size(); i++)
				if(stack[d][roots[d][rA][i].x][roots[d][rA][i].y] != v)
					goto notAtip;

			//Apply heuristics to determine if a false positive root tip
			for(unsigned r=0; r<roots[d].size(); r++)
				if(r != rA)
				{
					double dist;
						
					/*for(unsigned c=0; c<roots[d][r].centroids.size(); c++)
					{
						dist = abs((int)roots[d][rA].centroids[0].x - (int)roots[d][r].centroids[c].x)+
							   abs((int)roots[d][rA].centroids[0].y - (int)roots[d][r].centroids[c].y);

						//MAKE IT RELEVANT TO THE ROOT WIDTH
						if(dist<RADIAL_DIST)
						{
							//Mark root tip as false positive
							for(int i=0; i<roots[d][rA].size(); i++)
								stack[d][roots[d][rA][i].x][roots[d][rA][i].y] = 0xFE;
								
							goto notAtip;
						}
					}*/
					
					for(unsigned w=0; w<roots[d][r].wall.size(); w++)
					{
						dist = sqrt(pow((double)roots[d][rA].centroids[0].x - (double)roots[d][r].wall[w].x, 2)+
									pow((double)roots[d][rA].centroids[0].y - (double)roots[d][r].wall[w].y, 2));

						//DEBUG: Alternatively derive the root wall from the distance map instead of calculating the diameter
						if(10*dist/(roots[d][r].wall.size()/(2*M_PI))<menuParam.permDist)
						{
							//Mark root tip as false positive
							/*for(int i=0; i<roots[d][rA].size(); i++)
								stack[d][roots[d][rA][i].x][roots[d][rA][i].y] = 0xFE;*/

							goto notAtip;
						}
					}

					//Prevent downwards growing roots from being detected
					for(unsigned i=0; i<roots[d][rA].size(); i++)
					{
						if(d>1)
							if(stack[d-1][roots[d][rA][i].x][roots[d][rA][i].y]) break;

						//Mark root tip as upward growing false positive
						/*for(int i=0; i<roots[d][rA].size(); i++)
							stack[d][roots[d][rA][i].x][roots[d][rA][i].y] = 0xF9;*/

						roots[d][rA].skl.isDownward = true;
						//goto notAtip;//checkIf3D;
					}
				}

			roots[d][rA].skl.isTip = true;
			roots[d][rA].skl.setCoor(roots[d][rA].centroids[0], d);
			//roots[d][rA].skl.length = 1;

			//Mark root tips
			/*if(!roots[d][rA].skl.isDownward)
				for(int i=0; i<roots[d][rA].size(); i++)
					stack[d][roots[d][rA][i].x][roots[d][rA][i].y] = 0xFA;*/

/*checkIf3D:const int CUBE_SIZE = 10;
			Region::Skeleton& skl = roots[d][rA].skl;

			if(skl.x>=CUBE_SIZE && skl.x<stack.height-CUBE_SIZE && skl.y>=CUBE_SIZE && skl.y<stack.width-CUBE_SIZE && skl.z>=CUBE_SIZE && skl.z<stack.depth-CUBE_SIZE)
			{
				struct CUBESIDES
				{
					bool top, bottom, front, back, left, right;
			
				} cubeSides = {true, true, true, true, true, true};

				for(int s=CUBE_SIZE/2; s<=CUBE_SIZE; s++)
				{
					CUBESIDES cs = {};

					for(int x=-s; x<=s; x++)
						for(int y=-s; y<=s; y++)
						{
							if(stack[skl.z+s][skl.x+x][skl.y+y])
								cs.top = true;

							if(stack[skl.z-s][skl.x+x][skl.y+y])
								cs.bottom = true;
						}

					for(int z=-s; z<=s; z++)
						for(int x=-s; x<=s; x++)
						{
							if(stack[skl.z+z][skl.x+x][skl.y+s])
								cs.front = true;

							if(stack[skl.z+z][skl.x+x][skl.y-s])
								cs.back = true;
						}

					for(int z=-s; z<=s; z++)
						for(int y=-s; y<=s; y++)
						{
							if(stack[skl.z+z][skl.x-s][skl.y+y])
								cs.left = true;

							if(stack[skl.z+z][skl.x+s][skl.y+y])
								cs.right = true;
						}

					if(cubeSides.top && !cs.top) cubeSides.top = false;
					if(cubeSides.bottom && !cs.bottom) cubeSides.bottom = false;
					if(cubeSides.front && !cs.front) cubeSides.front = false;
					if(cubeSides.back && !cs.back) cubeSides.back = false;
					if(cubeSides.left && !cs.left) cubeSides.left = false;
					if(cubeSides.right && !cs.right) cubeSides.right = false;
				}

				if(!(cubeSides.left && cubeSides.right) && !(cubeSides.front && cubeSides.back))
					skl.is3DTip = true;
			}*/
notAtip:;}

		//Recover root paths
		for(unsigned rB=0; rB<roots[d+1].size(); rB++)
			for(unsigned v=0; v<roots[d+1][rB].size(); v++)
				parBrd[roots[d+1][rB][v].x][roots[d+1][rB][v].y] = rB+1;

		for(unsigned rA=0; rA<roots[d].size(); rA++)
		{
			//Region* ptrRegBelow = NULL;
			set<Region*> setRegBel;

			for(unsigned v=0; v<roots[d][rA].size(); v++)
				if(parBrd[roots[d][rA][v].x][roots[d][rA][v].y])
					if(/*ptrRegBelow !=*/ !setRegBel.count(&roots[d+1][parBrd[roots[d][rA][v].x][roots[d][rA][v].y]-1]))
					{
						roots[d+1][parBrd[roots[d][rA][v].x][roots[d][rA][v].y]-1].skl.parent.push_back(&roots[d][rA]);
						/*ptrRegBelow =*/setRegBel.insert(&roots[d+1][parBrd[roots[d][rA][v].x][roots[d][rA][v].y]-1]);
					}
		}

		//Clear Board and prepare it for next iteration
		for(unsigned rB=0; rB<roots[d+1].size(); rB++)
			for(unsigned v=0; v<roots[d+1][rB].size(); v++)
				parBrd[roots[d+1][rB][v].x][roots[d+1][rB][v].y] = 0;

		/*for(int rB=0; rB<roots[d+1].size(); rB++)
			if(!roots[d+1][rB].skl.parent.size())
			{
				Region* reg = NULL;
				unsigned minDist = 0xFFFFFFFF;

				for(unsigned cB=0; cB<roots[d+1][rB].centroids.size(); cB++)
					for(unsigned rA=0; rA<roots[d].size(); rA++)
						if(!roots[d][rA].skl.isTip)
							for(unsigned cA=0; cA<roots[d][rA].centroids.size(); cA++)
							{
								unsigned dist = abs((int)roots[d+1][rB].centroids[cB].x-(int)roots[d][rA].centroids[cA].x)+
												abs((int)roots[d+1][rB].centroids[cB].y-(int)roots[d][rA].centroids[cA].y);
							
								if(minDist > dist)
								{
									minDist = dist;
									reg = &roots[d][rA];
								}
							}

				//lstOfParReg.push_back(*//*roots[d+1][rB].skl.parent = reg;//);
				roots[d+1][rB].skl.parent.push_back(reg);
			}*/

		/*vector<Region*> lstOfParReg;
		
		for(unsigned rA=0; rA<roots[d].size(); rA++)
		{
			if(roots[d][rA].skl.isTip)
				break;
			
			for(int r=0; r<lstOfParReg.size(); r++)
				if(&roots[d][rA] == lstOfParReg[r])
					goto hasAParReg;

			volatile int a=0;
			a++;

			roots[d][rA].skl.isTip;
			roots[d][rA].skl.setCoor(roots[d][rA].centroids[0], d);
			roots[d][rA].skl.length = 1;

hasAParReg:;}*/
	}

	//Cross-Validation
	minCoeff = 1;
	minMSE = 0xFFFFFFFFFFFFFFFF;

	if(menuParam.autoSmthCoeff)
	{
		for(double coeff = 0; coeff<=1; coeff+=0.01)
		{
			unsigned long long avgMSE = 0;

			for(unsigned k=0; k<menuParam.K; k++)
				avgMSE += crossValidation(stack, roots, coeff, k);

			if(minMSE>=avgMSE)
			{
				minMSE = avgMSE;
				minCoeff = coeff;
			}
		}

		for(d=0; d<stack.depth; d++)
			for(int r=0; r<roots[d].size(); r++)
				roots[d][r].skl.mse.clear();
	}
	else
		minCoeff = (menuParam.smthCoeff?menuParam.smthCoeff/100.0:0);

	for(unsigned k=0; k<=menuParam.K; k++)
		crossValidation(stack, roots, minCoeff, k);

	//Draw horizonal lines connecting the skeleton at each root cross-section
	for(d=stack.depth-2; d>=1; d--)
		for(unsigned r=0; r<roots[d].size(); r++)
			if(!roots[d][r].skl.isTip)
				for(int p=0; p<roots[d][r].skl.parent.size(); p++)
					if((abs((int)roots[d][r].skl.parent[p]->skl.x-(int)roots[d][r].skl.x)>1 ||
					   abs((int)roots[d][r].skl.parent[p]->skl.y-(int)roots[d][r].skl.y)>1) &&
					   roots[d][r].skl.x && roots[d][r].skl.y &&
					   roots[d][r].skl.parent[p]->skl.x && roots[d][r].skl.parent[p]->skl.y)
						if(roots[d][r].skl.parent.size()>1)
							roots[d][r].skl.parent[p]->skl.drawLineRoot(roots[d][r].skl);
						else
							roots[d][r].skl.drawLineRoot(roots[d][r].skl.parent[p]->skl);

	//Mark various regions
	/*for(d=1; d<stack.depth-1; d++)
		for(unsigned r=0; r<roots[d].size(); r++)
		{
			if(!roots[d][r].skl.isTip)
			{
				unsigned localMaxDist = 1;
		
				for(int i=0; i<roots[d][r].size(); i++)
					if(stack[d][roots[d][r][i].x][roots[d][r][i].y] > localMaxDist &&
					   stack[d][roots[d][r][i].x][roots[d][r][i].y] <= 0xF8)
						localMaxDist = stack[d][roots[d][r][i].x][roots[d][r][i].y];

				//Mark the minimum regions within each root segment
				for(int i=0; i<roots[d][r].size(); i++)
					if(stack[d][roots[d][r][i].x][roots[d][r][i].y] == localMaxDist)
						stack[d][roots[d][r][i].x][roots[d][r][i].y] = 0xFD;

				int scale = 0xF8/localMaxDist;

				//Mark the body of the root to visualise the distance map
				for(int i=0; i<roots[d][r].size(); i++)
					if(stack[d][roots[d][r][i].x][roots[d][r][i].y] &&
					   stack[d][roots[d][r][i].x][roots[d][r][i].y] <= 0xF8)
						stack[d][roots[d][r][i].x][roots[d][r][i].y] = 
						scale*(localMaxDist - stack[d][roots[d][r][i].x][roots[d][r][i].y])+1;
				
				//Mark the centroids of the roots
				for(unsigned c=0; c<roots[d][r].centroids.size(); c++)
					for(int x=-1; x<=1; x++)
						for(int y=-1; y<=1; y++)
							stack[d][roots[d][r].centroids[c].x+x][roots[d][r].centroids[c].y+y] = 0xFB;

				//Mark the wall of the roots
				for(unsigned c=0; c<roots[d][r].wall.size(); c++)
					stack[d][roots[d][r].wall[c].x][roots[d][r].wall[c].y] = 0xFC;

				//Mark Regions which have not been recognised by the optimal path algorithm
				if(!roots[d][r].skl.x && !roots[d][r].skl.y)
					for(int i=0; i<roots[d][r].size(); i++)
						stack[d][roots[d][r][i].x][roots[d][r][i].y] = 0xFC;
			}

			//Mark the skeleton of the root
			for(int i=0; i<roots[d][r].skl.line.size(); i++)
				stack[d][roots[d][r].skl.line[i].x][roots[d][r].skl.line[i].y] = 0xFC;
			for(int x=-1; x<=1; x++)
				for(int y=-1; y<=1; y++)
					stack[d][roots[d][r].skl.x+x][roots[d][r].skl.y+y] = 0xF9;

			/*if(roots[d][r].skl./*parent*//*path.size() == 1)
				for(unsigned v=0; v<roots[d][r].size(); v++)
					stack[d][roots[d][r][v].x][roots[d][r][v].y] = 0xFB;

			if(roots[d][r].skl./*parent*//*path.size() == 2)
				for(unsigned v=0; v<roots[d][r].size(); v++)
					stack[d][roots[d][r][v].x][roots[d][r][v].y] = 0xFD;

			if(roots[d][r].skl./*parent*//*path.size() > 2)
				for(unsigned v=0; v<roots[d][r].size(); v++)
					stack[d][roots[d][r][v].x][roots[d][r][v].y] = 0xF9;*/
		//}
}

unsigned long long crossValidation(ImageStack& stack, vector<Region>* roots, double coeff, int k)
{
	unsigned d;
	unsigned long long avgMSE = 0;

	for(d=0; d<stack.depth; d++)
		for(int r=0; r<roots[d].size(); r++)
			roots[d][r].skl.clear();

	for(d=stack.depth-2; d>=1; d--)
		if(roots[d].size()) break;

	for(d--; d>=1; d--)		
		for(int rB=0; rB<roots[d+1].size(); rB++)
			if(roots[d+1][rB].skl.x && roots[d+1][rB].skl.y && roots[d+1][rB].skl.z && roots[d+1][rB].skl.parent.size())
				for(unsigned p=0; p<roots[d+1][rB].skl.parent.size(); p++)
				{
					_2DCoor coor;
					Region* reg = roots[d+1][rB].skl.parent[p];
					unsigned minDist = 0xFFFFFFFF;

					for(unsigned c=0; c<reg->centroids.size(); c++)
					{
						unsigned dist = abs((int)roots[d+1][rB].skl.x-(int)reg->centroids[c].x)+
										abs((int)roots[d+1][rB].skl.y-(int)reg->centroids[c].y);
							
						if(minDist > dist)
						{
							minDist = dist;
							coor = reg->centroids[c];
						}
					}

					_2DCoor kcoor = coor;

					//if(roots[d+1][rB].skl.length > reg->skl.length)
					{
						//if(!reg->skl.length) reg->skl.length++;

						if(k == reg->skl.k && reg->skl.parent.size())
						{
							Region* kreg = reg;

							while(k == kreg->skl.parent[0]->skl.k && kreg->skl.parent[0]->skl.parent.size())
								kreg = kreg->skl.parent[0];
								
							unsigned minDist = 0xFFFFFFFF;

							for(unsigned c=0; c<kreg->skl.parent[0]->centroids.size(); c++)
							{
								unsigned dist = abs((int)roots[d+1][rB].skl.x-(int)kreg->skl.parent[0]->centroids[c].x)+
												abs((int)roots[d+1][rB].skl.y-(int)kreg->skl.parent[0]->centroids[c].y);
							
								if(minDist > dist)
								{
									minDist = dist;
									coor = kreg->skl.parent[0]->centroids[c];
								}
							}
						}

						int dx = (int)roots[d+1][rB].skl.x-(int)coor.x;
						int dy = (int)roots[d+1][rB].skl.y-(int)coor.y;

						//double coeff = 0.85 + abs(dx+dy)/(4*stack[d][coor.x][coor.y]);

						//if(abs(dx+dy)<16*stack[d][coor.x][coor.y])
						reg->skl.setCoor(_2DCoor(coor.x+round(coeff*dx), coor.y+round(coeff*dy)), d);
						//else
						//	reg->skl.setCoor(coor, d);

						if(k == reg->skl.k && reg->skl.parent.size())
						{
							unsigned long long mse = abs((int)kcoor.x-(int)reg->skl.x)+
													 abs((int)kcoor.y-(int)reg->skl.y);
							avgMSE += mse*mse;

							reg->skl.drawLineMSE(kcoor);
						
							/*avgMSE += double(abs((int)kcoor.x-(int)reg->skl.x)+
									  abs((int)kcoor.y-(int)reg->skl.y))/
									  (reg->wall.size()?
									   reg->wall.size():1);*/
						}
					}

					//reg->skl.length += roots[d+1][rB].skl.length;
					reg->skl.path.push_back(&roots[d+1][rB]);
				}

	return avgMSE;
}