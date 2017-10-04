//System Depoendencies
#include<time.h>

//Header Files
#include "../headers/viewer.h"
#include "../headers/distancemap.h"
//#include "../headers/debug.h"

//Bresenham Line Drawing Algorithm
void Region::Skeleton::drawLine(vector<_2DCoor>& line, _2DCoor c)
{
	_2DCoor c0 = *this;
	_2DCoor c1 = c;

	int dx = abs((int)c1.x-(int)c0.x);
	int dy = abs((int)c1.y-(int)c0.y);
			
	int err = dx-dy;

	int sx = (c0.x<c1.x?1:-1);
	int sy = (c0.y<c1.y?1:-1);

	while(true)
	{
		int err2 = 2*err;

		line.push_back(c0);
		if(c0.x == c1.x && c0.y == c1.y) break;

		if(err2 > -dy)
		{
			err -= dy;
			c0.x += sx;
		}

		if(c0.x == c1.x && c0.y == c1.y)
		{
			line.push_back(c0);
			break;
		}

		if(err2 < dx)
		{
			err += dx;
			c0.y += sy;
		}
	}
}

//Global Variables
vector<Region> *rootsTop = NULL, *rootsFront = NULL, *rootsLeft = NULL;

//Segments individual regions contained in the image and returns a list of the regions
vector<Region> segment(vector<_2DCoor>& lstOfPxls, const _2DSize& size = _2DSize())
{
	static Board<unsigned> board;
	
	if(!size.height && !size.width)
	{
		int minH = 0xFFFF, maxH = 0;
		int minW = 0xFFFF,  maxW = 0;
		
		for(int i=0; i<lstOfPxls.size(); i++)
		{
			if(minH>lstOfPxls[i].x) minH = lstOfPxls[i].x;
			if(maxH<lstOfPxls[i].x) maxH = lstOfPxls[i].x;
			if(minW>lstOfPxls[i].y) minW = lstOfPxls[i].y;
			if(maxW<lstOfPxls[i].y) maxW = lstOfPxls[i].y;
		}

		board.resize(_2DSize(maxH - minH + 1, maxW - minW + 1));

		board.baseX = minH;
		board.baseY = minW;
	}
	else
	{
		board.resize(size);
		
		board.baseX = 0;
		board.baseY = 0;
	}
	
	unsigned ID = 1;
	vector<Region> lstOfRegs;

	for(unsigned i=0; i<lstOfPxls.size(); i++)
	{		
		_2DCoor crPxl = lstOfPxls[i];
		_2DCoor crBrd = _2DCoor(lstOfPxls[i].x-board.baseX, lstOfPxls[i].y-board.baseY);

		unsigned long long maxSize = 0;
		unsigned id = 0, numOfNeighbours = 0, maxID;
			
		for(int x=-1; x<=1; x++)
			for(int y=-1; y<=1; y++)
				if(board[crBrd.x+x][crBrd.y+y] && id!=board[crBrd.x+x][crBrd.y+y])
				{
					id = board[crBrd.x+x][crBrd.y+y];
					numOfNeighbours++;

					if(lstOfRegs[id-1].size() > maxSize)
					{
						maxSize = lstOfRegs[id-1].size();
						maxID = id;
					}
				}

		switch(numOfNeighbours)
		{
		case 0:
				
			board[crBrd.x][crBrd.y] = ID++;
			lstOfRegs.push_back(Region(vector<_2DCoor>(1, crPxl)));
			break;

		case 1:
				
			board[crBrd.x][crBrd.y] = id;
			lstOfRegs[id-1].push_back(crPxl);
			break;				

		default:

			for(int x=-1; x<=1; x++)
				for(int y=-1; y<=1; y++)
					if((id = board[crBrd.x+x][crBrd.y+y]) && id != maxID)
					{
						lstOfRegs[maxID-1].insert(lstOfRegs[maxID-1].end(),
												  lstOfRegs[id-1].begin(),
												  lstOfRegs[id-1].end());

						for(unsigned i=0; i<lstOfRegs[id-1].size(); i++)
							board[lstOfRegs[id-1][i].x-board.baseX][lstOfRegs[id-1][i].y-board.baseY] = maxID;

						lstOfRegs[id-1].clear();
					}

			board[crBrd.x][crBrd.y] = maxID;
			lstOfRegs[maxID-1].push_back(crPxl);

			break;
		}
	}

	vector<Region> lRNoEmSp;			

	for(int i=0; i<lstOfRegs.size(); i++)
	{
		if(lstOfRegs[i].size())
		{
			lRNoEmSp.push_back(lstOfRegs[i]);
		
			for(int j=0; j<lstOfRegs[i].size(); j++)
			{
				for(int x=-1; x<=1; x++)
					for(int y=-1; y<=1; y++)
						if(!board[lstOfRegs[i][j].x+x-board.baseX][lstOfRegs[i][j].y+y-board.baseY])
						{
							lRNoEmSp[lRNoEmSp.size()-1].wall.push_back(lstOfRegs[i][j]);
							x=y = 2;//continue
						}
			}

			lRNoEmSp[lRNoEmSp.size()-1].height = board.height-2;
			lRNoEmSp[lRNoEmSp.size()-1].width = board.width-2;

			lRNoEmSp[lRNoEmSp.size()-1].baseX = board.baseX;
			lRNoEmSp[lRNoEmSp.size()-1].baseY = board.baseY;
		}
	}

	//Clean the board to prepare it for processing
	//the next CT slice when segment() is called again
	for(unsigned i=0; i<lstOfPxls.size(); i++)
		board[lstOfPxls[i].x-board.baseX][lstOfPxls[i].y-board.baseY] = 0;

	return lRNoEmSp;
}

void distanceMap(vector<Region>* roots, ImageStack& stack)
{
	//Initiate Random Number Generator for the Cross-Validation Algorithm
	srand(unsigned(time(NULL)));

	vector<_3DCoor> wall;

	for(unsigned d=1; d<stack.depth-1; d++)
	{
		vector<_2DCoor> rootsPxls;

		for(unsigned h=1; h<stack.height-1; h++)
			for(unsigned w=1; w<stack.width-1; w++)
				if(stack[d][h][w] == 0xFF)
				{
					for(int z=-1; z<=1; z++)
						for(int x=-1; x<=1; x++)
							for(int y=-1; y<=1; y++)
								if(!stack[d+z][h+x][w+y])
								{
									wall.push_back(_3DCoor(h, w, d));
									x=y=z = 2;//continue
								}

					rootsPxls.push_back(_2DCoor(h, w));
				}

		roots[d] = segment(rootsPxls, stack);

		for(unsigned r=0; r<roots[d].size(); r++)
			roots[d][r].d = d;
	}

	for(unsigned h=1; h<stack.height-1; h++)
		for(unsigned w=1; w<stack.width-1; w++)
		{
			if(stack[0][h][w])
				wall.push_back(_3DCoor(h, w, 0));
			
			if(stack[stack.depth-1][h][w])
				wall.push_back(_3DCoor(h, w, stack.depth-1));
		}

	int distance = 1;
	vector<_3DCoor> outerLayer = wall, innerLayer;

	for(unsigned i=0; i<wall.size(); i++)
		stack[wall[i].z][wall[i].x][wall[i].y] = distance;
	distance++;

	do
	{
		for(unsigned i=0; i<outerLayer.size(); i++)
		{
			int h = outerLayer[i].x;
			int w = outerLayer[i].y;
			int d = outerLayer[i].z;

			for(int z=-1; z<=1; z++)
				for(int x=-1; x<=1; x++)
					for(int y=-1; y<=1; y++)
						//TRY TO AVOID HAVING TO HAVE AN IF STATEMENT
						if(d+z>0 && d+z<(int)stack.depth-1 && h+x>0 && h+x<(int)stack.height-1 && w+y>0 && w+y<(int)stack.width-1)
							if(stack[d+z][h+x][w+y] == 0xFF)
							{
								stack[d+z][h+x][w+y] = distance;
								innerLayer.push_back(_3DCoor(h+x, w+y, d+z));
							}
		}

		outerLayer = innerLayer;
		innerLayer.clear();
		distance++;
	}
	while(outerLayer.size());

	//make the top-down sweep wall the default
	//root wall to be displayed in 3DViewer.cpp
	if(roots == rootsTop) rootWall = wall;

	for(unsigned d=1; d<stack.depth-1; d++)
		//RENAME i j s and k TO MORE MEANINGFUL INDEX VARIABLES
		for(int i=0; i<roots[d].size(); i++)
		{
			unsigned localMaxDist = 1;
		
			for(int j=0; j<roots[d][i].size(); j++)
				if(stack[d][roots[d][i][j].x][roots[d][i][j].y] > localMaxDist)
					localMaxDist = stack[d][roots[d][i][j].x][roots[d][i][j].y];

			vector<_2DCoor> lstOfMinRegsPxls;

			for(int j=0; j<roots[d][i].size(); j++)
				if(stack[d][roots[d][i][j].x][roots[d][i][j].y] == localMaxDist)
					lstOfMinRegsPxls.push_back(roots[d][i][j]);
			
			vector<Region> lstOfRegs = segment(lstOfMinRegsPxls);
		
			static Board<char> board;

			for(int s=0; s<lstOfRegs.size(); s++)
			{					
				board.resize(lstOfRegs[s]);

				for(int j=0; j<lstOfRegs[s].size(); j++)
					board[lstOfRegs[s][j].x-lstOfRegs[s].baseX][lstOfRegs[s][j].y-lstOfRegs[s].baseY] = (char)0xFF;

				vector<_2DCoor> outerLayer, innerLayer;

				for(int j=0; j<lstOfRegs[s].size(); j++)
					for(int x=-1; x<=1; x++)
						for(int y=-1; y<=1; y++)
							if(!board[lstOfRegs[s][j].x-lstOfRegs[s].baseX+x][lstOfRegs[s][j].y-lstOfRegs[s].baseY+y])
							{
								outerLayer.push_back(_2DCoor(lstOfRegs[s][j].x-lstOfRegs[s].baseX, lstOfRegs[s][j].y-lstOfRegs[s].baseY));
								board[lstOfRegs[s][j].x-lstOfRegs[s].baseX][lstOfRegs[s][j].y-lstOfRegs[s].baseY] = 1;

								x=y = 2;//continue
							}
				do
				{					
					for(unsigned l=0; l<outerLayer.size(); l++)
					{
						int h = outerLayer[l].x;
						int w = outerLayer[l].y;

						for(int x=-1; x<=1; x++)
							for(int y=-1; y<=1; y++)
								if(board[h+x][w+y] == (char)0xFF)
								{
									innerLayer.push_back(_2DCoor(h+x, w+y));
									board[h+x][w+y] = 1;
								}
					}

					if(!innerLayer.size())
					{
						roots[d][i].centroids.push_back(_2DCoor(outerLayer[0].x+lstOfRegs[s].baseX, outerLayer[0].y+lstOfRegs[s].baseY));
						break;
					}

					outerLayer = innerLayer;
					innerLayer.clear();
				}
				while(true);

				//Clear the board so it could be ready to be used for processing the next region
				for(int j=0; j<lstOfRegs[s].size(); j++)
					board[lstOfRegs[s][j].x-lstOfRegs[s].baseX][lstOfRegs[s][j].y-lstOfRegs[s].baseY] = 0;
			}

			//THIS MIGHT NOT NEED TO BE DONE
			//Add one to every value to ensure the range is between 0x01 and 0xF8
			for(int j=0; j<roots[d][i].size(); j++)
				if(stack[d][roots[d][i][j].x][roots[d][i][j].y])
					stack[d][roots[d][i][j].x][roots[d][i][j].y]++;					
		}
}