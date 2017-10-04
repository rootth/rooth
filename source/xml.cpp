//Definitions
#undef UNICODE

//System Dependencies
#include<Windows.h>
#include<fstream>
#include<sstream>
#include<iomanip>
#include<string>
#include<cmath>

using namespace std;

//Header Files
#include "../headers/acm.h"
#include "../headers/xml.h"
#include "../headers/main.h"
#include "../headers/console.h"
#include "../headers/changelog.h"

//Global Variables
int SPLINE_DEN = 10;

//Static Global Variables
static int id = 1;

//Function Definitions
static void findRoots(fstream& file, RootSys& rtsys, int tab, int rootNumber = 0, int rootLabel = 0)
{
	stringstream label;

	if(rootLabel==0)
	{
		label<<"primary";
	}
	else if(rootLabel>=1)
	{
		for(int r=1; r<rootLabel; r++)
			label<<"sub_";
		label<<"lat_"<<rootNumber;
	}

	rootLabel++;
	
	for(int t=0; t<tab; t++) file<<'\t';
	file<<"<root id=\""<<id++<<"\" label=\""<<label.str().c_str()<<"\">"<<endl;//length=\""<<rtsys.size()<<"\">"<<endl;
	
	for(int t=0; t<tab+1; t++) file<<'\t';
	file<<"<properties>"<<endl;

	for(int t=0; t<tab+2; t++) file<<'\t';
	file<<"<length value=\""<<rtsys.rootLen<<"\" />"<<endl;

	for(int t=0; t<tab+2; t++) file<<'\t';
	file<<"<width value=\"";
	if(rtsys.size())//debug
		file<<rtsys.rootWidth;
	else
		file<<'-';
	file<<"\" />"<<endl;

	for(int t=0; t<tab+2; t++) file<<'\t';
	file<<"<angle value=\""<<rtsys.rootAngle[3]<<"\" />"<<endl;

	for(int t=0; t<tab+2; t++) file<<'\t';
	file<<"<type value=\"";
		if(rtsys.type==RootSys::PRIMARY) file<<"primary";
		if(rtsys.type==RootSys::SEMINAL) file<<"seminal";
		if(rtsys.type==RootSys::LATERAL) file<<"lateral";
		if(rtsys.type==RootSys::UNCLSSD) file<<"unclassified";
	file<<"\" />"<<endl;

	for(int t=0; t<tab+1; t++) file<<'\t';
	file<<"</properties>"<<endl;

	for(int t=0; t<tab+1; t++) file<<'\t';
	file<<"<geometry>"<<endl;

	for(int t=0; t<tab+2; t++) file<<'\t';
	file<<"<polyline>"<<endl;

	/*for(int i=0; i<rtsys.size(); i++)
	{
		for(int t=0; t<tab+3; t++) file<<"\t";
		file<<"<point x=\""<<rtsys[i].y<<"\" y=\""<<rtsys[i].x<<"\" z=\""<<rtsys[i].z<<"\" />"<<endl;
	}*/

	if(rtsys.parSnkPt.x>0 && rtsys.parSnkPt.y>0 && rtsys.parSnkPt.z>0 && rtsys.acmpts.size())
	{
		VECTOR v = rtsys.parSnkPt-rtsys.acmpts[0];

		for(int l=0; l<v.abs()/2-1; l++)
		{
			VECTOR point = rtsys.parSnkPt-double(l)/int(v.abs()/2)*v;

			for(int t=0; t<tab+3; t++) file<<"\t";
				file<<"<point x=\""<<point.x<<"\" y=\""<<point.y<<"\" z=\""<<point.z<<"\" />"<<endl;
		}
	}

	for(int c=0; c<rtsys.curve.size(); c++)
		for(double t=0; t<1.0f; t+=1.0f/SPLINE_DEN)
		{
			double x = rtsys.curve[c].xp.a*t*t*t + rtsys.curve[c].xp.b*t*t + rtsys.curve[c].xp.c*t + rtsys.curve[c].xp.d;
			double y = rtsys.curve[c].yp.a*t*t*t + rtsys.curve[c].yp.b*t*t + rtsys.curve[c].yp.c*t + rtsys.curve[c].yp.d;
			double z = rtsys.curve[c].zp.a*t*t*t + rtsys.curve[c].zp.b*t*t + rtsys.curve[c].zp.c*t + rtsys.curve[c].zp.d;

			for(int t=0; t<tab+3; t++) file<<"\t";
				file<<"<point x=\""<<x<<"\" y=\""<<y<<"\" z=\""<<z<<"\" />"<<endl;
		}

	for(int t=0; t<tab+2; t++) file<<'\t';
	file<<"</polyline>"<<endl;

	for(int t=0; t<tab+1; t++) file<<'\t';
	file<<"</geometry>"<<endl;

	if(rtsys.size())
	{
		for(int t=0; t<tab+1; t++) file<<'\t';
		file<<"<functions>"<<endl;

		for(int t=0; t<tab+2; t++) file<<'\t';
		file<<"<function domain=\"polyline\" name=\"diameter\">"<<endl;

		if(rtsys.parSnkPt.x>0 && rtsys.parSnkPt.y>0 && rtsys.parSnkPt.z>0 && rtsys.acmpts.size())
			for(int l=0; l<(rtsys.parSnkPt-VECTOR(rtsys[0])).abs()/2-1; l++)
			{
				for(int t=0; t<tab+3; t++) file<<"\t";
				file<<"<sample value=\"connecting line\" />"<<endl;
			}

		for(int c=0; c<rtsys.curve.size(); c++)
			for(double t=0; t<1.0f; t+=1.0f/SPLINE_DEN)
			{
				double minWidth = 1e100;

				double x = rtsys.curve[c].xp.a*t*t*t + rtsys.curve[c].xp.b*t*t + rtsys.curve[c].xp.c*t + rtsys.curve[c].xp.d;
				double y = rtsys.curve[c].yp.a*t*t*t + rtsys.curve[c].yp.b*t*t + rtsys.curve[c].yp.c*t + rtsys.curve[c].yp.d;
				double z = rtsys.curve[c].zp.a*t*t*t + rtsys.curve[c].zp.b*t*t + rtsys.curve[c].zp.c*t + rtsys.curve[c].zp.d;

				for(int t=0; t<tab+3; t++) file<<"\t";
				file<<"<sample value=\"";

				if(z>0 && z<imgStack.depth && x>0 && x<imgStack.height && y>0 && y<imgStack.width && (*imgStackTop)[int(z)][int(x)][int(y)])
				{
					for(int r=0; r<rtsys.regs.size(); r++)
						for(int w=0; w<rtsys.regs[r]->wall.size(); w++)
						{
							double width = (VECTOR(x, y, z)-_3DCoor(rtsys.regs[r]->wall[w], rtsys.regs[r]->d)).abs();

							if(minWidth>width) minWidth = width;
						}

					file<<setprecision(int(ceil(log10(2*minWidth)))+1)<<2*minWidth<<setprecision(6);
				}
				else
					file<<"not defined";

				file<<"\" />"<<endl;
			}

		for(int t=0; t<tab+2; t++) file<<'\t';
		file<<"</function>"<<endl;

		for(int t=0; t<tab+1; t++) file<<'\t';
		file<<"</functions>"<<endl;
	}

	int rn = 0;

	for(int i=0; i<rtsys.latRoots.size(); i++)
	{
		if(rtsys.latRoots[i]->size()>menuParam.prune || (!rtsys.size() && rtsys.latRoots[i]->acmpts.size()))
			findRoots(file, *rtsys.latRoots[i], tab+1, rn++, rootLabel);
	}

	for(int t=0; t<tab; t++) file<<"\t";
	file<<"</root>"<<endl;
}

void exportRootSystem(RootSys& rtsys)
{
	char fileName[256] = {};
	OPENFILENAME ofn = {};
					
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hMainWnd;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = sizeof(fileName);
	ofn.lpstrFilter = "Root System Markup Language (*.rsml)\0*.rsml\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_EXPLORER;

	if(GetSaveFileName(&ofn))
	{
		SYSTEMTIME time;
		string fn = fileName;

		//Append *.rsml extension at the end of the file name if not typed by the user
		if(fn.length()>5)
		{
			string ext = fn.substr(fn.length()-5);
			for(int i=0; i<ext.length(); i++)
				if(ext[i]>='A' && ext[i]<='Z')
					ext[i] += 'a'-'A';

			if(ext.compare(".rsml"))
			{
				fn += ".rsml";
				strcpy_s(fileName, fn.c_str());
			}
		}

		string fileKey = fn.substr(fn.rfind('\\')+1);

		id = 1;
		GetLocalTime(&time);
		
		fstream file(fileName, ios::out);
		
		file<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<endl;
		file<<"<rsml xmlns:po=\"http://www.plantontology.org/xml-dtd/po.dtd\">"<<endl;

		file<<"\t<metadata>"<<endl;
		file<<"\t\t<version>"VERSION_4(MAJVERSION,MINVERSION,SUBVERSION,TMPVERSION)"</version>"<<endl;
		file<<"\t\t<unit>pixel</unit>"<<endl;
		file<<"\t\t<resolution>1</resolution>"<<endl;
		file<<"\t\t<last-modified>"<<time.wYear<<"-"<<time.wMonth<<"-"<<time.wDay<<"T"<<time.wHour<<":"<<time.wMinute<<":"<<time.wSecond<<"</last-modified>"<<endl;
		file<<"\t\t<software>"<<title<<"</software>"<<endl;
		file<<"\t\t<user>jzj@cs.nott.ac.uk</user>"<<endl;
		file<<"\t\t<file-key>"<<fileKey.substr(0, fileKey.length()-5)<<"</file-key>"<<endl;
		/*file<<"\tzt<image>"<<endl;
		file<<"\t\t\t<name>"<<fileName<<"</name>"<<endl;
		file<<"\t\t\t<captured>201#-##-##T##:##:##</captured>"<<endl;
		file<<"\t\t</image>"<<endl;*/
		file<<"\t</metadata>"<<endl;

		file<<"\t<scene>"<<endl;
		file<<"\t\t<plant id=\""<<id++<<"\" label=\"root system\">"<<endl;
		findRoots(file, rtsys, 3);
		file<<"\t\t</plant>"<<endl;
		file<<"\t</scene>"<<endl;

		file<<"</rsml>";

		file.close();
	}
}

RootSys* importRootSystem(fstream& file)
{
	int conn = 0;
	string label;
	RootSys* rtsys = new RootSys;
	vector<VECTOR> points;

	do
	{
		file>>std::ws;
		file>>label;

		if(label.compare("<type")==0)
		{
			file>>label;

			if(label.compare("value=\"primary\"")==0) rtsys->type = RootSys::PRIMARY;
			if(label.compare("value=\"seminal\"")==0) rtsys->type = RootSys::SEMINAL;
			if(label.compare("value=\"lateral\"")==0) rtsys->type = RootSys::LATERAL;
		}

		if(file.eof())
		{
			delete rtsys;
			return NULL;
		}
	}
	while(label.compare("<polyline>")!=0);

	do
	{
		file>>std::ws;
		file>>label;

		if(label.compare("<point")==0)
		{
			char c;
			double x, y, z;

			file>>c>>c>>c;
			file>>x;
			file>>c>>c>>c>>c;
			file>>y;
			file>>c>>c>>c>>c;
			file>>z;

			points.push_back(VECTOR(x, y, z));
		}

		if(file.eof())
		{
			delete rtsys;
			return NULL;
		}
	}
	while(label.compare("</polyline>")!=0);

	do
	{
		file>>std::ws;
		file>>label;

		if(label.compare("<function")==0)
		{
			file>>label>>label;

			if(label.compare("name=\"diameter\">")==0)
				do
				{
					file>>std::ws;
					file>>label;

					if(label.compare("<sample")==0)
					{
						file>>label;

						if(label.compare("value=\"connecting")==0) conn++;
					}

					if(file.eof())
					{
						delete rtsys;
						return NULL;
					}
				}
				while(label.compare("</functions>")!=0);
		}

		if(label.compare("<root")==0)
		{
			RootSys* rs = importRootSystem(file);

			if(rs)
				rtsys->latRoots.push_back(rs);
			else
				return NULL;
		}

		if(file.eof())
		{
			delete rtsys;
			return NULL;
		}
	}
	while(label.compare("</root>")!=0);

	if(conn) rtsys->parSnkPt = points[0];
	for(int p=conn; p<points.size(); p+=SPLINE_DEN) rtsys->acmpts.push_back(points[p]);
	if(points.size() && (points.size()-1-conn)%SPLINE_DEN) rtsys->acmpts.push_back(points[points.size()-1]);

	//Calculate the smoothing spline connecting the points
	if(rtsys->acmpts.size()>=3)
	{
		vector<CUBF_PARAM> xpars = spline(rtsys->acmpts, &VECTOR::x);
		vector<CUBF_PARAM> ypars = spline(rtsys->acmpts, &VECTOR::y);
		vector<CUBF_PARAM> zpars = spline(rtsys->acmpts, &VECTOR::z);

		rtsys->curve.clear();
		for(int i=0; i<rtsys->acmpts.size()-1; i++)
			rtsys->curve.push_back(PARAM_CURVE(xpars[i], ypars[i], zpars[i]));
	}

	return rtsys;
}

static void traverseRootSys(RootSys* rtsys, ImageStack& imgStack)
{
	for(int i=0; i<rtsys->size(); i++)
		imgStack[(*rtsys)[i].x][(*rtsys)[i].y][(*rtsys)[i].z] = 255;

	for(int i=0; i<rtsys->latRoots.size(); i++)
		traverseRootSys(rtsys->latRoots[i], imgStack);
}

//Export Root System as a BMP Stack
void exportSkeletonStack(ImageStack* imgStack, char* path)
{
	for(unsigned d=0; d<imgStack->depth; d++)
		for(unsigned h=0; h<imgStack->height; h++)
			for(unsigned w=0; w<imgStack->width; w++)
				(*imgStack)[d][h][w] = 0;

	traverseRootSys(rootSys, *imgStack);
	
	unsigned height = imgStack->height;
	unsigned width = imgStack->width;

	BITMAPFILEHEADER header = {};

	header.bfType = 'MB';
	header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 3*height*width+height*((4-3*width%4)%4);
	header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BITMAPINFOHEADER info ={};

	info.biSize = sizeof(BITMAPINFOHEADER);
	info.biHeight = height;
	info.biWidth = width;
	info.biPlanes = 1;
	info.biBitCount = 24;
	info.biCompression = BI_RGB;
	info.biSizeImage = 3*height*width+height*((4-3*width%4)%4);

	for(unsigned d=0; d<imgStack->depth; d++)
	{
		stringstream bmpFileName;
		bmpFileName<<path<<"\\img"<<d<<".bmp";

		int fileSize;

		HANDLE hFile = CreateFile(bmpFileName.str().c_str(), GENERIC_WRITE, NULL, NULL, CREATE_NEW, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if(hFile!=INVALID_HANDLE_VALUE)
		{
			WriteFile(hFile, &header, sizeof(BITMAPFILEHEADER), (LPDWORD)&fileSize, NULL);
			WriteFile(hFile, &info, sizeof(BITMAPINFOHEADER), (LPDWORD)&fileSize, NULL);

			unsigned char* line = new unsigned char[3*width+((4-3*width%4)%4)];
			memset(line, 0, 3*width+((4-3*width%4)%4));

			for(int h=height-1; h>=0; h--)
			{
				for(int w=0; w<(int)width; w++)
				{
					line[3*w]   = (*imgStack)[d][h][w];
					line[3*w+1] = (*imgStack)[d][h][w];;
					line[3*w+2] = (*imgStack)[d][h][w];
				}
								
				WriteFile(hFile, line, 3*width+((4-3*width%4)%4), (LPDWORD)&fileSize, NULL);
			}

			delete line;

			CloseHandle(hFile);
		}
	}
}