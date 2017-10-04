//Macros
#undef UNICODE
#define FREEGLUT_LIB_PRAGMAS 0//prevent freeglut.h from linking library

//System Dependencies
#include<io.h>
#include<sstream>
#include<iostream>
#include<iomanip>
#include<fstream>

using namespace std;

//Header Files
#include "../headers/GL/freeglut.h"
#include "../headers/acm.h"
#include "../headers/xml.h"
#include "../headers/main.h"
#include "../headers/viewer.h"
#include "../headers/traits.h"
#include "../headers/console.h"
#include "../headers/fractalise.h"
#include "../headers/skeletonisation.h"
#include "../headers/changelog.h"
//#include "../headers/debug.h"

//Global Variables
//int rtOverheadDisp = 1;
//int rtAngleLenMes = 20;
string csvFileName = "output.csv";
HANDLE consoleThread = NULL;

//Global Static Variables
static stringstream stats;

static double avgAng   = 0;
static double avgWth   = 0;
static double totalLen = 0;

static int quietmode = 200;

static void printRootStats(stringstream& stats, RootSys& rtsys, int& rootNum, int parRootNum, int tab, _3DSize stack, bool printCSV)
{
	//Calculate Average Angle and Root Width for the whole root system
	avgAng   += rtsys.rootAngle[3];
	avgWth   += rtsys.rootWidth*rtsys.rootLen;
	totalLen += rtsys.rootLen;

	//Output Root System measurements in the console window
	if(rootNum<=quietmode || quietmode == -1)
	{
		if(printCSV)
		{
			stats<<rootNum<<",";

			if(rtsys.type==RootSys::PRIMARY)
				stats<<"primary,-,";
			else if(rtsys.type==RootSys::SEMINAL)
				stats<<"seminal,-,";
			else if(rtsys.type==RootSys::LATERAL)
				stats<<"lateral,"<<parRootNum<<",";
			else
				stats<<"unclassified,"<<parRootNum<<",";

			(rtsys.size()?stats<<menuParam.res*rtsys.size()/1000:stats<<'-')<<',';
			stats<<setprecision(int(ceil(log10(menuParam.res*rtsys.rootLen/1000)))+1)<<menuParam.res*rtsys.rootLen/1000<<',';

			(rtsys.size()?stats<<menuParam.res*rtsys.rootWidthCurve:stats<<'-')<<',';
			(rtsys.size()?stats<<setprecision(int(ceil(log10(menuParam.res*rtsys.rootWidth)))+1)<<menuParam.res*rtsys.rootWidth:stats<<'-')<<',';

			(rtsys.size()?stats<<rtsys.rootVolume:stats<<'-')<<',';

			stats<<setprecision(int(ceil(log10(rtsys.rootAngle[0])))+1)<<rtsys.rootAngle[0]<<",";
			stats<<setprecision(int(ceil(log10(rtsys.rootAngle[1])))+1)<<rtsys.rootAngle[1]<<",";
			stats<<setprecision(int(ceil(log10(rtsys.rootAngle[2])))+1)<<rtsys.rootAngle[2]<<",";
			stats<<setprecision(int(ceil(log10(rtsys.rootAngle[3])))+1)<<rtsys.rootAngle[3];
		}
		else
		{
			stats<<"Root "<<rootNum<<": ";
			
			if(rtsys.size()) stats<<"L = "<<menuParam.res*rtsys.size()/1000<<"mm ";
			stats<<"LC = "<<setprecision(int(ceil(log10(menuParam.res*rtsys.rootLen/1000)))+1)<<menuParam.res*rtsys.rootLen/1000<<"mm ";
	
			if(rtsys.size()) stats<<"AW = "<<menuParam.res*rtsys.rootWidthCurve<<"\xE6m ";
			if(rtsys.size()) stats<<"SW = "<<setprecision(int(ceil(log10(menuParam.res*rtsys.rootWidth)))+1)<<menuParam.res*rtsys.rootWidth<<"\xE6m ";
	
			if(rtsys.size()) stats<<"V = "<<rtsys.rootVolume<<"vu\xFC ";

			stats<<"V\xF8 = "<<setprecision(int(ceil(log10(rtsys.rootAngle[3])))+1)<<rtsys.rootAngle[3]<<"\xF8";
		}
		stats<<endl;
	}

	if(rtsys.type == rtsys.PRIMARY || rtsys.type == rtsys.SEMINAL) parRootNum = rootNum;

	for(int i=0; i<rtsys.latRoots.size(); i++)
	{
		if(rtsys.latRoots[i]->size()>menuParam.prune || (!rtsys.size() && rtsys.latRoots[i]->acmpts.size()))//DEBUG
		{
			if(rootNum<=quietmode && !printCSV)
				for(int t=0; t<tab; t++) stats<<"  ";
			printRootStats(stats, *rtsys.latRoots[i], ++rootNum, parRootNum, tab+1, stack, printCSV);
		}
	}
}

void statsOutput(_3DSize stack, bool printCSV, bool updThrld, bool updClss)
{	
	if(!rootSys) return;
	
	stats.str("");

	avgWth	 = 0;
	avgAng	 = 0;
	totalLen = 0;

	if(updThrld) classifyRoots(rootSys);

	rootTraits(*rootSys, updClss);

	if(!printCSV)
	{
		if(rootSys->size())
		{
			stats<<"Smoothing Coeff = "<<minCoeff<<endl;
			/*stats<<"Number of CV Runs = "<<menuParam.K<<"Nested level or roots = "<<rootSys.latRoots.depth<<"; Minimum Square Error = "<<minMSE<<endl*/;
			stats<<"--------------------------------------------------------------------------------------------------------------"<<endl;
		}
	}
	else
		stats<<"Root Number,Root Type,Parent Seminal Root,"\
			   "Root Length CF (mm),Root Length ACM (mm),Root Width CF (microns),Root Width ACM (microns),Root Volume (voxels),"\
			   "Root Angle at 10% (deg),Root Angle at 25% (deg),Root Angle at 50% (deg),Root Angle at 100% (deg)\n";

	int numOfRoots = 1;
	printRootStats(stats, *rootSys, numOfRoots, 0, 1, stack, printCSV);

	if(numOfRoots>1)
		avgAng = avgAng/numOfRoots;
	else
		avgAng = 0;
	
	avgWth = avgWth/totalLen;

	if(numOfRoots>quietmode && quietmode>=0)
		stats<<"\n\nPrinting of traits has been suppressed (current threshold set to "<<quietmode<<"). Use \"quietmode off\" to turn back on\n\n\n"<<endl;
	
	if(!printCSV)
	{
		stats<<"--------------------------------------------------------------------------------------------------------------"<<endl;
		stats<<"Number of Roots    = "<<numOfRoots<<endl;
		stats<<"Total Root Length  = "<<setprecision(int(ceil(log10(menuParam.res*totalLen/1000)))+1)<<menuParam.res*totalLen/1000<<"mm"<<endl;
		if(rootSys->size())
			stats<<"Average Root Width = "<<setprecision(int(ceil(log10(menuParam.res*avgWth)))+1)<<menuParam.res*avgWth<<"\xE6m"<<endl;
		stats<<"Average Laterals Vertical Root Angle = "<<setprecision(int(ceil(log10(avgAng)))+1)<<avgAng<<'\xF8'<<endl<<endl;
		
		if(rootSys->size())
		{
			stats<<"Number of ACM iterations = "<<iterCount<<endl;
			stats<<"Active Contour Model step (dt) = "<<dt<<endl;
			stats<<"Seminal/Lateral Root Classification Threshold = "<<rtClssTrshld<<endl;
		}

		stats<<"Connecting Lines Length = ";
		if(connLinesLen>=0)
			stats<<connLinesLen<<endl;
		else
			stats<<"\b\b- All Lateral Roots Connected"<<endl;
		cout<<endl;
	}
	
	if(!consoleThread)
		consoleThread = CreateThread(NULL, 0, console, NULL, 0, NULL);
}

DWORD WINAPI console(LPVOID lpParam)
{
	if(AllocConsole())
	{
		EnableMenuItem(GetMenu(hMainWnd), ID_TOOLS_CONSOLE, MF_BYCOMMAND + MF_DISABLED);
		SetConsoleTitle((string(title)+" - Console Output").c_str());
		system("mode 110, 300");
		SetWindowPos(GetConsoleWindow(), NULL, 50, 80, 1100, 340, NULL);
						
		*stdin  = *_fdopen(_open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), 0), "r");
		*stdout = *_fdopen(_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),0), "w");

		if(rootSys) sendConsoleCommand("print");

		while(true)
		{
			string input;
			
			cout.clear();
			cout.good();

			cin.clear();
			cin.good();

			cout<<"\n>";
			cin>>input;

			if(input.compare("exit") == 0)
			{
				ExitProcess(0);
			}
			else if(input.compare("clear") == 0 || input.compare("cls") == 0)
			{
				system("cls");
			}
			else if(input.compare("close") == 0)
			{
				cin.clear();
				cin.good();
				FreeConsole();

				consoleThread = NULL;
				EnableMenuItem(GetMenu(hMainWnd), ID_TOOLS_CONSOLE, MF_BYCOMMAND + MF_ENABLED);
				break;
			}
			else if(input.compare("save") == 0)
			{
				fstream file(csvFileName, ios::out);
				statsOutput(imgStack, true, false, false);
				file<<stats.str();
				cout<<"\nWritten to \""<<csvFileName<<"\"\n";
				statsOutput(imgStack, false, false, false);
			}
			else if(input.compare("threshold") == 0)
			{
				cin>>rtClssTrshld;
				statsOutput(imgStack, false, false, true);
			}
			else if(input.compare("rsml") == 0)
			{
				cin>>SPLINE_DEN;
			}
			else if(input.compare("step") == 0)
			{
				cin>>dt;
				statsOutput(imgStack, false, false, false);
			}
			else if(input.compare("param") == 0)
			{
				cin>>attractLow>>attractHigh>>repulseLow>>repulseHigh>>ITER;
				statsOutput(imgStack, false, false, false);
			}
			else if(input.compare("lines") == 0)
			{
				cin>>connLinesLen;
				statsOutput(imgStack, false, false, false);
			}
			else if(input.compare("colour") == 0)
			{
				cin>>background.red>>background.green>>background.blue;
			}
			else if(input.compare("framedelay") == 0)
			{
				cin>>framedelay;
			}
			else if(input.compare("framerate") == 0)
			{
				cin>>framerate;
			}
			else if(input.compare("size") == 0)
			{
				cin>>windowSize.width>>windowSize.height;

				if(glutWnd) glutReshapeWindow(windowSize.width, windowSize.height);
			}
			else if(input.compare("quietmode") == 0)
			{
				string str;
				cin>>str;

				if(str.compare("on") == 0)
					quietmode = 0;
				else if(str.compare("off") == 0)
					quietmode = -1;
				else
				{
					int t;
					stringstream ss(str);
					
					ss>>t;

					if(!ss || t<0)
						cout<<"\n\""<<str<<"\" is not a valid option. Use [\"on\"/\"off\"/value>0] to turn on or silence traits output respectively"<<endl;
					else
						quietmode = t;
				}	
			}
			else if(input.compare("print") == 0)
			{
				cout<<endl<<endl;
				cout<<stats.str();
			}
			else if(input.compare("refresh") == 0)
			{
				statsOutput(imgStack);
			}
			else if(input.compare("version") == 0)
			{
				cout<<endl<<VERSION_4(MAJVERSION, MINVERSION, SUBVERSION, TMPVERSION)<<endl;
			}
			else if(input.compare("help") == 0 || input.compare("?") == 0)
			{
				cout<<endl;
				cout<<"param      [Attract Low][Artract High][Repulse Low][Repulse High] [Iterations] - set ACM calibrate parameters"<<endl;
				cout<<"step       [coefficient] - set the value of dt defining the step between two consecutive ACM iterations"<<endl;
				cout<<"lines      [voxels] - set length of connecting lines in voxel units"<<endl;
				cout<<"threshold  [mm] - set seminal/lateral root length classification threshold"<<endl;
				cout<<"rsml       [number of points] - between two adjacent snake points in exported RSML file (default value 10)"<<endl;
				cout<<"colour     [red] [green] [blue] - set background colour of 3D Root System Viewer"<<endl;
				cout<<"framedelay [miliseconds] - set rendering speed when creating a video frame sequence of the root system"<<endl;
				cout<<"framerate  [number of frames] - set the number of frames between successive roots when rendering video"<<endl;
				cout<<"size       [width] [height] - set the size of the 3D Root System Viewer window (in pixel units)"<<endl;
				cout<<"quietmode  [\"on\"/\"off\"/value] - silence the output of root traits in console"<<endl;
				cout<<"save       - save traits in a *.csv file in the current active directory"<<endl;
				cout<<"print      - display pre-calculated root traits"<<endl;
				cout<<"refresh    - re-calculate root traits"<<endl;
				cout<<"clear      - creates a blank console window"<<endl;
				cout<<"close      - close the console window"<<endl;
				cout<<"exit       - exit "<<title<<endl;
				cout<<"version    - show RooTh version"<<endl;
				cout<<"help       - displays this menu"<<endl;
				cout<<endl;
			}
			else
			{
				cout<<"\n\""<<input<<"\""<<" command not recognised...\n";
			}
		}
	}

	return 0;
}

void sendConsoleCommand(string str)
{
	str += "\r\n";
	DWORD len = (DWORD)str.length();
	INPUT_RECORD* input = new INPUT_RECORD[len];

	memset(input, 0, len*sizeof(INPUT_RECORD));

	for(unsigned l=0; l<len; l++)
	{
		input[l].EventType = KEY_EVENT;
		input[l].Event.KeyEvent.bKeyDown = true;
		input[l].Event.KeyEvent.wRepeatCount = 0;
		input[l].Event.KeyEvent.uChar.AsciiChar = str[l];
	}

	WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), input, len, &len);

	delete [] input;
}