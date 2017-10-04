//Compiler Directives
//Enable Modern Visual Styles for default controls like buttons and sliders
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#define FREEGLUT_LIB_PRAGMAS 0//prevent freeglut.h from linking library
#undef UNICODE

//System Dependencies
#include<Windows.h>
#include<Commctrl.h>
#include<Shlobj.h>
#include<fstream>

using namespace std;

//Headers
#include "../headers/GL/freeglut.h"
#include "../headers/main.h"
#include "../headers/dialog.h"
#include "../headers/fileio.h"
#include "../headers/imagestack.h"
#include "../headers/distancemap.h"
#include "../headers/skeletonisation.h"
#include "../headers/fractalise.h"
#include "../headers/acm.h"
#include "../headers/viewer.h"
#include "../headers/console.h"
#include "../headers/xml.h"

#include "../headers/changelog.h"
//#include "../headers/debug.h" //only used for debugging. Remove from Release version

//Global Variables
char* argv;
int   argc;
HWND hMainWnd = NULL;
const char title[] = "RooTh";
_3DSize imgStack;
ImageStack *imgStackTop = NULL, *imgStackFront = NULL, *imgStackLeft;
unsigned char* regressionImg = NULL;
HICON icon;
MENUFLAGS menuFlags = {};
MENUPARAM menuParam = {true, 0, 3, 50, 1, 150, 3, 54};

//Static Global variables
static HBRUSH hBackground = CreateSolidBrush(RGB(53, 53, 53));

//Static Function Declarations
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//Program Entry Point - Initialise and display the main window and entre the main window
//message loop. Runs on the Main Thread.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{	
	WNDCLASS wc;

	argc = 1;
	argv = lpCmdLine;

	//Initialise and register the windows class (MainClass)
	wc.style = CS_HREDRAW + CS_VREDRAW;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = icon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor = NULL;
	wc.hbrBackground = hBackground;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.lpszClassName = "MainWindow";
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);

	if(!RegisterClass(&wc))
	{
		popupError("Incompatible Version of Windows. Please Use Windows XP or later...");
		return -1;
	}

	HACCEL hAcc	= LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));

	//Calculate the initial position of the main window
	const unsigned screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
	const unsigned screenWidth  = GetSystemMetrics(SM_CXFULLSCREEN);

	const unsigned winXPos = screenWidth/4;
	const unsigned winWidth = screenWidth/2;
	const unsigned winHeight = 3*winWidth/4;
	const unsigned winYPos = (screenHeight-winHeight)/2;

	//Create and Display the Main Window
	hMainWnd = CreateWindowEx(WS_EX_ACCEPTFILES, wc.lpszClassName, title,
							  WS_OVERLAPPEDWINDOW + WS_VSCROLL,
							  winXPos, winYPos, winWidth, winHeight,
							  NULL, NULL, hInstance, NULL);

	ShowWindow(hMainWnd, iCmdShow);
	UpdateWindow(hMainWnd);

	MSG message;

	//Enter the main message loop (runs on the Main Thread)
	while(GetMessage(&message, NULL, 0, 0))
	{
		if(!TranslateAccelerator(hMainWnd, hAcc, &message))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	//Once the main window is closed the program exits
	return (int)message.wParam;
}

//Callback Procedure for the main Window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static VGM* vgm = NULL;
	static Image* dispImage = NULL;
	static ListOfFileNames lstOfFN;
	//static int winHeight = 0, winWidth = 0;

	static struct
	{
		bool updateScreen;
		enum {ARROW, WAIT} cursor;
		enum {NONE, ORIGINAL, ANALYSED/*, REGRESSION*/} dspImgType;

	} flags = {false, flags.ARROW, flags.NONE};

	static unsigned char* _32bitImage = NULL;
	
	switch(message)
	{
		case WM_CREATE:
		{
			EnableScrollBar(hWnd, SB_VERT, ESB_DISABLE_BOTH);

			return 0;
		}

		case WM_DESTROY:
		{
			//SendMessage(hWnd, WM_COMMAND, ID_FILE_CLOSE, 0);
			//PostQuitMessage(0);
			ExitProcess(0);

			return 0;
		}

		case WM_DROPFILES:
		{
			char fileName[256];
			MENUITEMINFO itemInfo;

			DragQueryFile((HDROP)wParam, 0, fileName, 255);			
			DragFinish((HDROP)wParam);

			{
				string fn = fileName;
				string ext = fn.substr(fn.length()-4);
				string extl = fn.substr(fn.length()-5);

				for(int i=0; i<extl.length(); i++)
					if(extl[i]>='A' && extl[i]<='Z')
						extl[i] += 'a'-'A';

				for(int i=0; i<ext.length(); i++)
					if(ext[i]>='A' && ext[i]<='Z')
						ext[i] += 'a'-'A';

				if(extl.compare(".rsml")==0)
				{
					if(imgStack.depth && imgStack.height && imgStack.width)
						goto import;
					else
					{
						popupInfo("Open a *.bmp Image or a *.vgm file before importing an *.rsml file");
						return 0;
					}
				}
				else if(ext.compare(".obj")==0)
				{
					goto mesh;
				}
				else if(imgStack.depth && imgStack.height && imgStack.width)
				{
					popupError("Drag and drop an *.rsml file or close the current image stack and load a new one");
					return 0;
				}
			}

			//Check that the root thinning process isn't working in the background before loading a new task
			memset(&itemInfo, 0, sizeof(itemInfo));
			itemInfo.cbSize = sizeof(itemInfo);
			itemInfo.fMask = MIIM_STATE;

			GetMenuItemInfo(GetMenu(hWnd), ID_FILE_OPEN, false, &itemInfo);

			if(itemInfo.fState == MF_ENABLED && wParam) goto load;

			return 0;

		case WM_COMMAND:

			switch(LOWORD(wParam))
			{
				//Display a Dialog Box prompting the User to select an image file or
				//sequence of image files (*.bmp) representing a CT scan of a plant root system
				case IDA_CTRL_O:
				case ID_FILE_OPEN:
				{
					if(Image::selectImageDialog(hWnd, fileName))
					{
load:					SetWindowText(hWnd, (string(fileName).substr(string(fileName).rfind("\\")+1) + " - " + title).c_str());
						
						//Check file type
						string fn = fileName;

						if(fn.length()>4)
						{
							string ext = fn.substr(fn.length()-4);
							for(int i=0; i<ext.length(); i++)
								if(ext[i]>='A' && ext[i]<='Z')
									ext[i] += 'a'-'A';

							if(ext.compare(".bmp")==0)
							{
								//load all files into the memory (into  3D array) and 
								//pass stack variable to the analyse funtion to further process the images
								if(dispImage) delete dispImage, dispImage = NULL;
								dispImage = new Image(fileName);

								lstOfFN = ListOfFileNames(fileName);
								
								imgStack = _3DSize((unsigned)lstOfFN.list.size(), *dispImage);
							}
							else if(ext.compare(".vgm")==0)
							{
								if(dispImage) delete dispImage;
								if(vgm) delete vgm;

								dispImage = new Image(vgm = new VGM(fileName), 0);

								lstOfFN = ListOfFileNames(*vgm, fileName);
								
								imgStack = vgm->volSize;
							}
							else
							{
								fn = fileName;

								if(ext[0] != '.') ext = (fn.find_last_of('.')<fn.size()?fn.substr(fn.find_last_of('.')):"");

								popupError("File Type *"+ext+" not recognised\nTry Loading a *.bmp Image Stack or a *.vgm File");
								break;
							}
						}

						if(!dispImage->image)
						{
							if(!popupQuestion("Contiue Loading remaning images anyway?", true))
								return -1;
						}
						else
							_32bitImage = new unsigned char[4*dispImage->height*dispImage->width];

						EnableMenuItem(GetMenu(hWnd), ID_FILE_OPEN, MF_BYCOMMAND + MF_DISABLED);
						EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVEAS, MF_BYCOMMAND + MF_ENABLED);
						EnableMenuItem(GetMenu(hWnd), ID_FILE_CLOSE, MF_BYCOMMAND + MF_ENABLED);
						EnableMenuItem(GetMenu(hWnd), ID_FILE_IMPORT, MF_BYCOMMAND + MF_ENABLED);

						EnableMenuItem(GetMenu(hWnd), ID_VIEW_ORIGINAL, MF_BYCOMMAND+MF_ENABLED);
						EnableMenuItem(GetMenu(hWnd), ID_VIEW_ANALYSED, MF_BYCOMMAND + MF_DISABLED);

						CheckMenuRadioItem(GetMenu(hWnd), ID_VIEW_ORIGINAL, ID_VIEW_ANALYSED, ID_VIEW_ORIGINAL, MF_BYCOMMAND + MF_CHECKED);

						SCROLLINFO scroll = {sizeof(scroll), SIF_ALL, 0, lstOfFN.size-1, 1};

						EnableScrollBar(hWnd, SB_VERT, ESB_ENABLE_BOTH);
						SetScrollInfo(hWnd, SB_VERT, &scroll, true);

						buttons.analyse = true;
						
						if(hCtrlDlg)
							SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);
						else
							CreateThread(NULL, 0, CtrlThread, NULL, 0, NULL);

						RECT rect;

						GetWindowRect(hWnd, &rect);

						int height   = rect.bottom-rect.top;
						int width    = rect.right-rect.left;
						double ratio = double(dispImage->width)/dispImage->height;

						if(ratio<1)
						{
							if((height = int(width/ratio))>GetSystemMetrics(SM_CYFULLSCREEN))
							{
								width  = int(double(width)*GetSystemMetrics(SM_CYFULLSCREEN)/height)+GetSystemMetrics(SM_CXVSCROLL)+2*GetSystemMetrics(SM_CXFRAME);
								height = GetSystemMetrics(SM_CYFULLSCREEN);
							}
						}
						else
						{
							if((width = int(height*ratio))>GetSystemMetrics(SM_CXFULLSCREEN))
							{
								height = int(double(height)*(GetSystemMetrics(SM_CXFULLSCREEN)-GetSystemMetrics(SM_CXVSCROLL)-2*GetSystemMetrics(SM_CXFRAME))/width);
								width  = GetSystemMetrics(SM_CXFULLSCREEN);
							}
						}
						
						height += GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU) + 2*GetSystemMetrics(SM_CYBORDER);
						
						SetWindowPos(hWnd, NULL, 0, 0, width, height, SWP_NOMOVE);
						SetClassLong(hWnd, -10, NULL);

						flags.dspImgType = flags.ORIGINAL;
						flags.updateScreen = true;

						InvalidateRect(hWnd, NULL, false);
						UpdateWindow(hWnd);
					}
					
					break;
				}

				case IDA_CTRL_S:
				case ID_FILE_SAVEAS:
				{
					char fileName[256] = {};
					OPENFILENAME ofn = {};
					
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = hMainWnd;
					ofn.lpstrFile = fileName;
					ofn.nMaxFile = sizeof(fileName);
					ofn.lpstrFilter = "24-bit Bitmap (*.bmp)\0*.bmp\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_EXPLORER;

					if(dispImage && GetSaveFileName(&ofn))
					{			
						unsigned height = dispImage->height;
						unsigned width = dispImage->width;

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

						string fn = fileName;

						//Append *.bmp extension at the end of the file name if not typed in by the user
						if(fn.length()>4)
						{
							string ext = fn.substr(fn.length()-4);
							for(int i=0; i<ext.length(); i++)
								if(ext[i]>='A' && ext[i]<='Z')
									ext[i] += 'a'-'A';

							if(ext.compare(".bmp"))
							{
								fn += ".bmp";
								strcpy_s(fileName, fn.c_str());
							}
						}

						HANDLE hFile = CreateFile(fileName, GENERIC_WRITE, NULL, NULL, CREATE_NEW, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

						if(hFile!=INVALID_HANDLE_VALUE)
						{
							int fileSize;

							WriteFile(hFile, &header, sizeof(BITMAPFILEHEADER), (LPDWORD)&fileSize, NULL);
							WriteFile(hFile, &info, sizeof(BITMAPINFOHEADER), (LPDWORD)&fileSize, NULL);

							unsigned char* line = new unsigned char[3*width+((4-3*width%4)%4)];
							memset(line, 0, 3*width+((4-3*width%4)%4));

							for(int h=height-1; h>=0; h--)
							{
								for(int w=0; w<(int)width; w++)
								{
									line[3*w]   = _32bitImage[4*(h*width+w)];//(*imgStack)[lstOfFN.index][h][w];
									line[3*w+1] = _32bitImage[4*(h*width+w)+1];//(*imgStack)[lstOfFN.index][h][w];
									line[3*w+2] = _32bitImage[4*(h*width+w)+2];//(*imgStack)[lstOfFN.index][h][w];
								}
								
								WriteFile(hFile, line, 3*width+((4-3*width%4)%4), (LPDWORD)&fileSize, NULL);
							}

							delete line;

							CloseHandle(hFile);
						}
					}
					
					break;
				}

				case ID_FILE_SKELETON:
				{
					if(rootSys)
					{
						BROWSEINFO folder = {};

						folder.hwndOwner = hWnd;
						folder.lpszTitle = "Select Folder to Store Skeleton Image Stack";
						folder.ulFlags   = BIF_EDITBOX+BIF_VALIDATE+BIF_NEWDIALOGSTYLE;
						
						PIDLIST_ABSOLUTE pid = SHBrowseForFolder(&folder);

						if(pid)
						{
							char path[256] = {};

							SHGetPathFromIDList(pid, path);
							
							exportSkeletonStack(imgStackTop, path);
						}
					}

					break;
				}

				case ID_FILE_IMPORT:
				{
					//Load root wall triangular mesh wavefront file (computed using marching cubes with FIJI)
					OPENFILENAME ofn;
					memset(&ofn, 0, sizeof(ofn));
					
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = hMainWnd;
					ofn.lpstrFile = fileName;
					ofn.nMaxFile = 256; //Windows file names maximum characters limit
					ofn.lpstrFilter = "Root System Markup Language (*.rsml)\0*.rsml\0";
					ofn.lpstrTitle = "Import root system skeleton from an RSML encoded file";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST + OFN_FILEMUSTEXIST + OFN_EXPLORER;

					if(GetOpenFileName(&ofn))
					{
import:					fstream file(fileName, ios::in);

						rootSys = importRootSystem(file);//debug

						file.close();

						if(!rootSys)
						{
							popupError(string("RSML file ")+fileName+" is corrupt");
							break;
						}
						
						rootSysLst.clear();
						rootSysLst.push_back(rootSys);

						EnableMenuItem(GetMenu(hWnd), ID_FILE_EXPORT, MF_BYCOMMAND + MF_ENABLED);
						EnableMenuItem(GetMenu(hWnd), ID_FILE_TRAITS, MF_BYCOMMAND + MF_ENABLED);

						connLinesLen		   = -1;
						display.rootSys		   = display.SKEL;
						display.crvFitSkl	   = false;
						display.spline		   = true;
						buttons.classification = true;
							
						if(!hCtrlDlg)
							CreateThread(NULL, 0, CtrlThread, NULL, 0, NULL);
						else
							SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

						if(!glutWnd)
							CreateThread(NULL, 0, init, NULL, 0, NULL);
						else
							glutDisplayFunc(drawSkeleton);

						statsOutput(imgStack, false, false, false);
						sendConsoleCommand("print");
					}

					break;
				}

				case IDA_CTRL_E:
				case ID_FILE_EXPORT:
				{
					if(!rootSys) break;

					if(runACM || runCalibrate)
					{
						popupError("The Active Cotour Model is currently running.\nPlease stop it first before exporting root system architecture");
						break;
					}

					exportRootSystem(*rootSys);

					break;
				}

				case IDA_CTRL_T:
				case ID_FILE_TRAITS:
				{
					if(!rootSys) break;

					if(runACM || runCalibrate)
					{
						popupError("The Active Cotour Model is currently running.\nPlease stop it first before exporting root system traits");
						break;
					}

					char fileName[256] = {};
					OPENFILENAME ofn = {};
					
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = hMainWnd;
					ofn.lpstrFile = fileName;
					ofn.nMaxFile = sizeof(fileName);
					ofn.lpstrFilter = "Comma-Separated Values File/Microsoft Excel (*.csv)\0*.csv\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_EXPLORER;

					if(GetSaveFileName(&ofn))
					{
						csvFileName = fileName;

						//Append *.rsml extension at the end of the file name if not typed by the user
						if(csvFileName.length()>4)
						{
							string ext = csvFileName.substr(csvFileName.length()-4);
							for(int i=0; i<ext.length(); i++)
								if(ext[i]>='A' && ext[i]<='Z')
									ext[i] += 'a'-'A';

							if(ext.compare(".csv")) csvFileName += ".csv";
						}

						sendConsoleCommand("save");
					}

					break;
				}

				case IDA_CTRL_M:
				case ID_FILE_MESH:
				{
					if(!meshMutex) meshMutex = CreateMutex(NULL, false, NULL);
					
					//Load root wall triangular mesh wavefront file (computed using marching cubes with FIJI)
					OPENFILENAME ofn;
					memset(&ofn, 0, sizeof(ofn));
					
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner = hMainWnd;
					ofn.lpstrFile = fileName;
					ofn.nMaxFile = 256; //Windows file names maximum characters limit
					ofn.lpstrFilter = "Wavefront File (*.obj)\0*.obj\0";
					ofn.lpstrTitle = "Select a Wavefront File Containing the Triangular Mesh Representation of the Root System";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST + OFN_FILEMUSTEXIST + OFN_EXPLORER;
					
					if(GetOpenFileName(&ofn))
					{
mesh:					fstream file(fileName);
						vector<_3DCoorT<float>> vertecies;

						flags.cursor = flags.WAIT;
						SendMessage(hWnd, WM_SETCURSOR, NULL, 0);

						WaitForSingleObject(meshMutex, INFINITE);
						
							mesh.clear();

							while(!file.eof())
							{
								char symbol;
								float x, y, z;

								file>>symbol;

								if(symbol == '#')
								{
									getline(file, string());
									continue;
								}

								if(symbol == 'v')
								{
									file>>x>>y>>z;

									vertecies.push_back(_3DCoorT<float>(x, y, z));
								}

								if(symbol == 's')
								{
									file>>x;
								}

								if(symbol == 'f')
								{
									int i1, i2, i3;

									file>>i1>>i2>>i3;
									i1--; i2--; i3--;

									mesh.push_back(Triangle(vertecies[i1], vertecies[i2], vertecies[i3]));
								
								}
							}

						ReleaseMutex(meshMutex);

						flags.cursor = flags.ARROW;
						SendMessage(hWnd, WM_SETCURSOR, NULL, 0);

						if(imgStack.depth && imgStack.height && imgStack.width)
						{
							display.rootSys = display.MESH;
							
							CheckDlgButton(hCtrlDlg, IDC_MESH, true);
							CheckDlgButton(hCtrlDlg, IDC_WALL, false);
							CheckDlgButton(hCtrlDlg, IDC_SKEL, false);

							SendMessage(hCtrlDlg, WM_COMMAND, IDC_MESH, 0);
							if(!glutWnd)
								CreateThread(NULL, 0, init, NULL, 0, NULL);
							else
								glutDisplayFunc(drawWall);
						}
					}

					break;
				}

				case IDA_CTRL_R:
				case ID_FILE_RENDER:
				{
					if(!rootSys) break;

					if(!renderMovie)
					{
						if(!buttons.classification)
							if(!popupQuestion("Root System has not been skeletonised.\nContinue Rendering Video Anyway?", true)) break;

						BROWSEINFO folder = {};

						folder.hwndOwner = hWnd;
						folder.lpszTitle = "Select Folder to Store Rendered Video Frames";
						folder.ulFlags   = BIF_EDITBOX+BIF_VALIDATE+BIF_NEWDIALOGSTYLE;
						
						PIDLIST_ABSOLUTE pid = SHBrowseForFolder(&folder);

						if(pid)
						{
							SHGetPathFromIDList(pid, videoPath);
							
							ModifyMenu(GetMenu(hWnd), ID_FILE_RENDER, MF_BYCOMMAND + MF_STRING, ID_FILE_RENDER, STOP_STR);
							renderMovie = true;
						}
					}
					else
					{
						ModifyMenu(GetMenu(hWnd), ID_FILE_RENDER, MF_BYCOMMAND + MF_STRING, ID_FILE_RENDER, START_STR);
						renderMovie = false;
					}

					break;
				}

				case IDA_ALT_C:
				case ID_FILE_CLOSE:
				{
					//Calculate the position of the main window and
					//set the main window settings back to their default values
					//i.e. default title, default background and an arrow icon for the cursor
					const unsigned screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
					const unsigned screenWidth  = GetSystemMetrics(SM_CXFULLSCREEN);

					const unsigned winWidth = screenWidth/2;
					const unsigned winHeight = 3*winWidth/4;
					
					flags.updateScreen = false;
					flags.dspImgType = flags.NONE;
					flags.cursor = flags.ARROW;

					SetWindowText(hWnd, title);
					SendMessage(hMainWnd, WM_SETCURSOR, NULL, 0);
					SetClassLong(hWnd, -10, (long)(hBackground));
					SetWindowPos(hWnd, NULL, 0, 0, winWidth, winHeight, SWP_NOMOVE);
					
					InvalidateRect(hWnd, NULL, true);
					UpdateWindow(hWnd);

					EnableMenuItem(GetMenu(hWnd), ID_FILE_OPEN,     MF_BYCOMMAND + MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVEAS,   MF_BYCOMMAND + MF_DISABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_SKELETON, MF_BYCOMMAND + MF_DISABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_CLOSE,    MF_BYCOMMAND + MF_DISABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_EXPORT,   MF_BYCOMMAND + MF_DISABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_TRAITS,   MF_BYCOMMAND + MF_DISABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_RENDER,   MF_BYCOMMAND + MF_DISABLED);

					EnableMenuItem(GetMenu(hWnd), ID_VIEW_ORIGINAL,   MF_BYCOMMAND + MF_DISABLED);
					EnableMenuItem(GetMenu(hWnd), ID_VIEW_ANALYSED,   MF_BYCOMMAND + MF_DISABLED);
					EnableMenuItem(GetMenu(hWnd), ID_VIEW_REGRESSION, MF_BYCOMMAND + MF_DISABLED);

					CheckMenuRadioItem(GetMenu(hWnd), ID_VIEW_ORIGINAL, ID_VIEW_ANALYSED, NULL, MF_BYCOMMAND + MF_UNCHECKED);
					CheckMenuRadioItem(GetMenu(hWnd), ID_VIEW_ORIGINAL, ID_VIEW_ANALYSED, NULL, MF_BYCOMMAND + MF_UNCHECKED);

					if(glutWnd) closeViewer = true;
					sendConsoleCommand("close");

					merged   = false;
					analysed = false;
					buttons.analyse = false;
					SendMessage(hCtrlDlg, WM_COMMAND, IDCANCEL, 0);

					//Clear data structures
					if(vgm) delete vgm, vgm = NULL;
					if(imgStackTop) delete imgStackTop, imgStackTop = NULL;
					if(rootsTop) delete [] rootsTop, rootsTop = NULL;
					if(dispImage) delete dispImage, dispImage = NULL;
					if(_32bitImage) delete [] _32bitImage, _32bitImage = NULL;

					lstOfFN.clear();
					mesh.clear();
					rootWall.clear();
					rootSysLst.clear();
					
					rootSys = NULL;
					iterCount = 0;

					imgStack = _3DSize();
					
					break;
				}
			
				//Close the Main Window and exit the program
				case IDA_ALT_X:
				case ID_FILE_EXIT:
				{
					SendMessage(hWnd, WM_DESTROY, 0, 0);
					break;
				}

				case ID_VIEW_ORIGINAL:
				{
					SendMessage(hWnd, WM_KEYDOWN, VK_UP, NULL);
					break;
				}

				case ID_VIEW_ANALYSED:
				{
					SendMessage(hWnd, WM_KEYDOWN, VK_DOWN, NULL);
					break;
				}

				/*case ID_VIEW_REGRESSION:
				{
					CheckMenuRadioItem(GetMenu(hWnd), ID_VIEW_ORIGINAL, ID_VIEW_REGRESSION, ID_VIEW_REGRESSION, MF_BYCOMMAND + MF_CHECKED);
					statsOutput(imgStack, lstOfFN);

					break;
				}*/

				case ID_VIEW_MSE:
				{
					menuFlags.mse = !menuFlags.mse;
					CheckMenuItem(GetMenu(hWnd), ID_VIEW_MSE, MF_BYCOMMAND + (menuFlags.mse?MF_CHECKED:MF_UNCHECKED));
					
					break;
				}

				case ID_VIEW_UNDETECTED:
				{
					menuFlags.undetect = !menuFlags.undetect;
					CheckMenuItem(GetMenu(hWnd), ID_VIEW_UNDETECTED, MF_BYCOMMAND + (menuFlags.undetect?MF_CHECKED:MF_UNCHECKED));

					break;
				}

				case ID_TOOLS_VIEWER:
				{
					if(!glutWnd) CreateThread(NULL, 0, init, NULL, 0, NULL);

					break;
				}

				case ID_TOOLS_CTRL:
				{
					if(!hCtrlDlg) CreateThread(NULL, 0, CtrlThread, NULL, 0, NULL);
					break;
				}

				case ID_TOOLS_CONSOLE:
				{
					if(!consoleThread) consoleThread = CreateThread(NULL, 0, console, NULL, 0, NULL);
					break;
				}

				case IDA_CTRL_P:
				case ID_TOOLS_OPTIONS:
				{
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_OPTIONS), hWnd, DlgProc);
					break;
				}

				//Display a Dialog Box with information about the Root Thinning Tool
				case IDA_F1:
				case ID_HELP_ABOUT:
				{
					popupInfo("Plant Root Skeletonisation Tool \n\n" \
							  "Version - " VERSION(MAJVERSION, MINVERSION, SUBVERSION, TMPVERSION) "\n" \
							  "Compiled on "__DATE__ " at " __TIME__ "\n\n"
							  "Project Funded by the\n" \
							  "European Research Council");
					break;
				}
			}

			return 0;
		}

		case WM_VSCROLL:
		{
			SCROLLINFO scroll = {sizeof(scroll), SIF_POS + SIF_TRACKPOS};

			GetScrollInfo(hWnd, SB_VERT, &scroll);

			switch(LOWORD(wParam))
			{
				case SB_PAGEUP:
				case SB_LINEUP:

					scroll.nPos--;

					break;

				case SB_PAGEDOWN:
				case SB_LINEDOWN:

					scroll.nPos++;

					break;

				case SB_THUMBTRACK:

					scroll.nPos = scroll.nTrackPos;
			}

			lstOfFN.index = scroll.nPos;

			goto reload;

			return 0;
		}

		case WM_MOUSEWHEEL:
		{
			lstOfFN.index += (GET_WHEEL_DELTA_WPARAM(wParam)<0?1:-1);
			
			if(lstOfFN.index<0) lstOfFN.index++;
			if(lstOfFN.index>=(int)lstOfFN.size) lstOfFN.index--;

			goto reload;

			return 0;
		}

		case WM_KEYDOWN:
		{
			switch(wParam)
			{
				case VK_UP:
				{
					if(dispImage)
					{
						flags.dspImgType = flags.ORIGINAL;
						CheckMenuRadioItem(GetMenu(hWnd),
						ID_VIEW_ORIGINAL, ID_VIEW_REGRESSION, ID_VIEW_ORIGINAL, MF_BYCOMMAND + MF_CHECKED);

						/*display.rootSys = DISPLAY::WALL;
		
						if(glutWnd) glutDisplayFunc(drawWall);
						CheckRadioButton(hCtrlDlg, IDC_MESH, IDC_SKEL, IDC_WALL);*/

						goto reload;
					}

					break;
				}

				case VK_DOWN:
				{
					if(imgStack.depth)
						if(imgStackTop->isLoaded())
						{
							flags.dspImgType = flags.ANALYSED;
							CheckMenuRadioItem(GetMenu(hWnd),
							ID_VIEW_ORIGINAL, ID_VIEW_REGRESSION, ID_VIEW_ANALYSED, MF_BYCOMMAND + MF_CHECKED);

							/*display.rootSys = DISPLAY::SKEL;
		
							if(glutWnd) glutDisplayFunc(drawSkeleton);
							CheckRadioButton(hCtrlDlg, IDC_MESH, IDC_SKEL, IDC_SKEL);*/

							goto reload;
						}

					break;
				}

				case VK_LEFT:
				{
					if(lstOfFN.index>0)
						lstOfFN.index-=2;
					else
						break;

				case VK_RIGHT:

					if(lstOfFN.index<(int)lstOfFN.size-1)
						lstOfFN.index++;
					else
						break;
					
reload:				if(lstOfFN.size)
					{
						SetScrollPos(hWnd, SB_VERT, lstOfFN.index, true);

						flags.updateScreen = true;
					
						if(lstOfFN.format == ListOfFileNames::BMP)
						{
							if(flags.dspImgType == flags.ORIGINAL)
							{
								if(dispImage) delete dispImage;
								dispImage = new Image(lstOfFN.list[lstOfFN.index].c_str());
							}

							SetWindowText(hWnd, (lstOfFN.list[lstOfFN.index].substr(lstOfFN.list[lstOfFN.index].rfind("\\")+1) + " - " + title).c_str());
						}
						else if(lstOfFN.format == ListOfFileNames::VGM)
						{
							if(flags.dspImgType == flags.ORIGINAL)
							{
								if(vgm && dispImage)
								{
									delete dispImage;
									dispImage = new Image(vgm, lstOfFN.index);
								}
							}

							char str[16] = {};

							wsprintf(str, ": Slice %i", lstOfFN.index+1);
							SetWindowText(hWnd, (lstOfFN.fileName.substr(lstOfFN.fileName.rfind("\\")+1) + str + " - " + title).c_str());
						}

						InvalidateRect(hWnd, NULL, false);
						UpdateWindow(hWnd);
					}

					break;
				}

				/*case VK_PRIOR:

					if(imgStack == imgStackFront) imgStack = imgStackLeft;
					if(imgStack == imgStackTop)   imgStack = imgStackFront;

					goto reallocImage;

				case VK_NEXT:

					if(imgStack == imgStackFront) imgStack = imgStackTop;
					if(imgStack == imgStackLeft)  imgStack = imgStackFront;

 reallocImage:		if(_32bitImage)
					{
						delete _32bitImage;
						_32bitImage = new unsigned char[4*imgStack->height*imgStack->width];
					}

					goto reload;*/
			}

			return 0;
		}

		/*case WM_CHAR:
		{
			switch(wParam)
			{
				case '+':
				{
					rtOverheadDisp++;
					statsOutput(imgStack, lstOfFN);

					break;
				}

				case '-':
				{
					rtOverheadDisp--;
					statsOutput(imgStack, lstOfFN);

					break;
				}

				case '[':
				{
					if(rtAngleLenMes>20) rtAngleLenMes-=20;
					statsOutput(imgStack, lstOfFN);
					
					break;
				}

				case ']':
				{
					rtAngleLenMes+=20;
					statsOutput(imgStack, lstOfFN);
					
					break;
				}
			}

			return 0;
		}*/

		case WM_SETCURSOR:
		{	
			switch (flags.cursor)
			{
				case flags.WAIT:
					
					SetCursor(LoadCursor(NULL, IDC_WAIT));
					break;

				case flags.ARROW:
					
					SetCursor(LoadCursor(NULL, IDC_ARROW));
					break;
			}
			
			return wParam?DefWindowProc(hWnd, message, wParam, lParam):0;
		}

		/*case WM_SIZE:
		{
			winWidth  = LOWORD(lParam);
			winHeight = HIWORD(lParam);

			return 0;
		}*/

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);

			static HBITMAP hImage = NULL;

			unsigned height = 0, width = 0;

			switch(flags.dspImgType)
			{
				case flags.ORIGINAL:
									
					height = dispImage->height;
					width  = dispImage->width;
					break;

				case flags.ANALYSED:
				//case flags.REGRESSION:

					height = imgStack.height;
					width  = imgStack.width;
					break;
			}
			
			if(flags.updateScreen)
			{
				if(hImage) DeleteObject(hImage), hImage = NULL;
				hImage = CreateCompatibleBitmap(hDC, width, height);
				
				//if(dispImage && imgStack.depth)
				//	if(dispImage->image)
					{
						for(unsigned h=0; h<height; h++)
							for(unsigned w=0; w<width; w++)
							{
								unsigned char value;

								switch(flags.dspImgType)
								{
									case flags.ORIGINAL:
									
										value = dispImage->image[h][w];

										_32bitImage[4*(h*width+w)]   = \
										_32bitImage[4*(h*width+w)+1] = \
										_32bitImage[4*(h*width+w)+2] = value;

										break;

									case flags.ANALYSED:

										value =  (*imgStackTop)[lstOfFN.index][h][w];

										if(value == 0x00)//Background
										{
											_32bitImage[4*(h*width+w)]   = 0x44;
											_32bitImage[4*(h*width+w)+1] = 0x77;
											_32bitImage[4*(h*width+w)+2] = 0x00;
										}
										else if(value == 0xF9)//Skeleton
										{
											_32bitImage[4*(h*width+w)]   = 0x00;
											_32bitImage[4*(h*width+w)+1] = 0xFF;
											_32bitImage[4*(h*width+w)+2] = 0x00;
										}
										else if(value == 0xFA)//Root Tips
										{
											_32bitImage[4*(h*width+w)]   = 0x00;
											_32bitImage[4*(h*width+w)+1] = 0x64;
											_32bitImage[4*(h*width+w)+2] = 0xFF;
										}
										else if(value == 0xFB)//Centroid
										{
											_32bitImage[4*(h*width+w)]   = 0x00;
											_32bitImage[4*(h*width+w)+1] = 0x00;
											_32bitImage[4*(h*width+w)+2] = 0xFF;
										}
										else if(value == 0xFC)//Wall
										{
											_32bitImage[4*(h*width+w)]   = 0x00;
											_32bitImage[4*(h*width+w)+1] = 0xFF;
											_32bitImage[4*(h*width+w)+2] = 0xFF;
										}
										else if(value == 0xFD)//Local Minima
										{
											_32bitImage[4*(h*width+w)]   = 0xFF;
											_32bitImage[4*(h*width+w)+1] = 0x00;
											_32bitImage[4*(h*width+w)+2] = 0x00;
										}
										else if(value == 0xFE)//False-positive root tip
										{
											_32bitImage[4*(h*width+w)]   = 0x00;
											_32bitImage[4*(h*width+w)+1] = 0x00;
											_32bitImage[4*(h*width+w)+2] = 0xAA;
										}
										else//Distance Map of Root Body
										{
											_32bitImage[4*(h*width+w)]   = \
											_32bitImage[4*(h*width+w)+1] = \
											_32bitImage[4*(h*width+w)+2] = value;
										}

										break;

									/*case flags.REGRESSION:

										value = regressionImg[h*width+w];
										break;*/
								}

								_32bitImage[4*(h*width+w)+3] = 0;
							}
					}

				SetBitmapBits(hImage, 4*height*width, _32bitImage);

				//Draw and Ellipse around each root cross-section
				/*if(dispImage->image && imgStack && flags.dspImgType == flags.ANALYSED)
				{
					HDC hMem = CreateCompatibleDC(hDC);
					HPEN hPen = CreatePen(PS_SOLID, 1, RGB(32, 32, 128));
					HBRUSH hBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);//CreateSolidBrush(RGB(32, 32, 128));

					SetGraphicsMode(hMem, GM_ADVANCED);
					//SetMapMode(hMem, MM_LOENGLISH);

					SelectObject(hMem, hImage);
					SelectObject(hMem, hPen);
					SelectObject(hMem, hBrush);					

					for(unsigned r=0; r<rootsTop[lstOfFN.index].size(); r++)
					{
						POINT p[3] = {};
						RECT rect = {};
						XFORM xForm = {};

						_2DCoor pA = rootsTop[lstOfFN.index][r].pA;
						_2DCoor pB = rootsTop[lstOfFN.index][r].pB;
						_2DCoor pC = rootsTop[lstOfFN.index][r].pC;

						float angle = atan(float((short)pA.x-pB.x)/((short)pA.y-pB.y));

						xForm.eM11 = cos(angle);
						xForm.eM12 = sin(angle);
						xForm.eM21 = sin(-angle);
						xForm.eM22 = cos(-angle);
						
						p[0].x = pA.y;
						p[0].y = pA.x;

						p[1].x = pB.y;
						p[1].y = pB.x;

						p[2].x = pC.y;
						p[2].y = pC.x;

						SetWorldTransform(hMem, &xForm);
						DPtoLP(hMem, (LPPOINT)&p, 3);

						rect.left	= p[0].x;
						rect.top	= p[0].y;
						rect.right	= p[1].x;
						rect.bottom	= p[1].y;

						if(p[0].x<p[1].x)
						{
							rect.left = p[0].x;
							rect.right = p[1].x;
						}
						else
						{
							rect.left = p[1].x;
							rect.right = p[0].x;
						}

						if(p[0].y-p[2].y>0)
						{
							rect.top = p[0].y+(p[0].y-p[2].y);
							rect.bottom = p[2].y;
						}
						else
						{
							rect.top = p[2].y;
							rect.bottom = p[0].y-(p[2].y-p[0].y);
						}

						Ellipse(hMem, rect.left, rect.top, rect.right, rect.bottom);
					}

					DeleteObject(hBrush);
					DeleteObject(hPen);
					DeleteDC(hMem);
				}*/

				flags.updateScreen = false;
			}

			if(flags.dspImgType)
			{
				HDC hMem = CreateCompatibleDC(hDC);
				RECT rect;

				GetClientRect(hWnd, &rect);

				SetStretchBltMode(hDC, HALFTONE);
				SelectObject(hMem, hImage);
				StretchBlt(hDC, 0, 0, rect.right/*winWidth*/, rect.bottom/*winHeight*/, hMem, 0, 0, width, height, SRCCOPY);

				DeleteDC(hMem);

				//Draw ACM Parameters on the Screen
				/*char str[256] = {};

				SetTextColor(hDC, RGB(255, 255, 0));
				SetBkMode(hDC, TRANSPARENT);

				rect.left   = rect.right-150;
				rect.top    = 25;
				rect.bottom = 40;
				memset(str, 0, 256);
				wsprintf(str, "Stretch = %i", (int)alpha);
				DrawText(hDC, str, 256, &rect, DT_LEFT);

				rect.top    = 40;
				rect.bottom = 55;
				memset(str, 0, 256);
				wsprintf(str, "Stiffness = %i", (int)beta);
				DrawText(hDC, str, 256, &rect, DT_LEFT);

				rect.top    = 55;
				rect.bottom = 70;
				memset(str, 0, 256);
				wsprintf(str, "Force Strength = %i", (int)gamma);
				DrawText(hDC, str, 256, &rect, DT_LEFT);

				rect.top    = 70;
				rect.bottom = 85;
				memset(str, 0, 256);
				wsprintf(str, "Repulsion = %i", (int)delta);
				DrawText(hDC, str, 256, &rect, DT_LEFT);

				SetTextColor(hDC, RGB(255, 0, 0));

				rect.top    = 85;
				rect.bottom = 100;
				memset(str, 0, 256);
				wsprintf(str, "Iteration = %i", (int)iterCount+1);
				DrawText(hDC, str, 256, &rect, DT_LEFT);*/
			}

			EndPaint(hWnd, &ps);

			return 0;
		}

		case UM_UPDATE:
		{
			switch(wParam)
			{
				case IDUM_ANALYSE:
				{
					if(lstOfFN.format == ListOfFileNames::BMP)
						imgStackTop = new ImageStack(lstOfFN, imgStack);
					else if(lstOfFN.format == ListOfFileNames::VGM)
						imgStackTop = new ImageStack(lstOfFN.fileName, imgStack);

					break;
				}

				case IDUM_HOURGLASS:
				{
					flags.cursor = flags.WAIT;
					SendMessage(hWnd, WM_SETCURSOR, NULL, 0);

					break;
				}

				case IDUM_UPDATESCREEN:
				{
					flags.cursor = flags.ARROW;
					SendMessage(hWnd, WM_SETCURSOR, NULL, 0);

					EnableMenuItem(GetMenu(hWnd), ID_FILE_OPEN,     MF_BYCOMMAND + MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_SKELETON, MF_BYCOMMAND + MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_CLOSE,    MF_BYCOMMAND + MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_EXPORT,   MF_BYCOMMAND + MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_TRAITS,   MF_BYCOMMAND + MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), ID_FILE_RENDER,   MF_BYCOMMAND + MF_ENABLED);

					EnableMenuItem(GetMenu(hWnd), ID_VIEW_ANALYSED, MF_BYCOMMAND + MF_ENABLED);
					//EnableMenuItem(GetMenu(hWnd), ID_VIEW_REGRESSION, MF_BYCOMMAND + MF_ENABLED);

					CheckMenuRadioItem(GetMenu(hWnd), ID_VIEW_ORIGINAL, ID_VIEW_ANALYSED, ID_VIEW_ANALYSED, MF_BYCOMMAND + MF_CHECKED);

					analysed = true;
					SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

					flags.updateScreen = true;
					flags.dspImgType = flags.ANALYSED;

					InvalidateRect(hWnd, NULL, false);
					UpdateWindow(hWnd);

					break;
				}

				/*case IDUM_UPDATEREGRESSIONIMG:
				{
					flags.updateScreen = true;
					flags.dspImgType = flags.REGRESSION;

					InvalidateRect(hWnd, NULL, false);
					UpdateWindow(hWnd);

					break;
				}*/

				case IDUM_REFRESH:
				{
					flags.updateScreen = true;
					InvalidateRect(hWnd, NULL, false);
					UpdateWindow(hWnd);
				}
			}

			return 0;
		}

		default: return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void analyse()
{
	statusIndex = 5;
	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

	imgStackFront = new ImageStack(_3DSize(imgStackTop->height, imgStackTop->depth, imgStackTop->width));				
	rootsFront = new vector<Region>[imgStackFront->depth];

	for(unsigned d=0; d<imgStackTop->depth; d++)
	{
		for(unsigned h=0; h<imgStackTop->height; h++)
			for(unsigned w=0; w<imgStackTop->width; w++)
				(*imgStackFront)[h][d][w] = (*imgStackTop)[d][h][w];

		SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(50+double(SCROLL_RANGE/8)*d/imgStackTop->depth), 0);
	}

	distanceMap(rootsFront, *imgStackFront);

	//Remove some false-postive control points in Front side sweep
	for(unsigned d=1; d<imgStackFront->depth-1; d++)
	{
		for(unsigned r=0; r<rootsFront[d].size(); r++)
		{
			bool isFlsPstv = true;

			for(unsigned v=0; v<rootsFront[d][r].size(); v++)
				if((*imgStackFront)[d][rootsFront[d][r][v].x][rootsFront[d][r][v].y]>=menuParam.sideCtrlPtsThrld)
					isFlsPstv = false;

			if(isFlsPstv)
				rootsFront[d][r].centroids.clear();
		}

		SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(62.5+double(SCROLL_RANGE/8)*(d-1)/(imgStackTop->depth-2)), 0);
	}

	delete imgStackFront;

	statusIndex = 6;
	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);
					
	imgStackLeft  = new ImageStack(_3DSize(imgStackTop->width, imgStackTop->depth, imgStackTop->height));
	rootsLeft  = new vector<Region>[imgStackLeft->depth];

	for(unsigned d=0; d<imgStackTop->depth; d++)
	{
		for(unsigned w=0; w<imgStackTop->width; w++)
			for(unsigned h=0; h<imgStackTop->height; h++)
				(*imgStackLeft)[w][d][h] = (*imgStackTop)[d][h][w];

		SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(75+double(SCROLL_RANGE/8)*d/imgStackTop->depth), 0);
	}

	distanceMap(rootsLeft, *imgStackLeft);

	//Remove some false-postive control points in Left side sweep
	for(unsigned d=1; d<imgStackLeft->depth-1; d++)
	{
		for(unsigned r=0; r<rootsLeft[d].size(); r++)
		{
			bool isFlsPstv = true;

			for(unsigned v=0; v<rootsLeft[d][r].size(); v++)
				if((*imgStackLeft)[d][rootsLeft[d][r][v].x][rootsLeft[d][r][v].y]>=menuParam.sideCtrlPtsThrld)
					isFlsPstv = false;

			if(isFlsPstv)
				rootsLeft[d][r].centroids.clear();
		}

		SendMessage(GetDlgItem(hCtrlDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, int(87.5+double(SCROLL_RANGE/8)*(d-1)/(imgStackTop->depth-2)), 0);
	}

	delete imgStackLeft;

	statusIndex = 7;
	SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

	rootsTop = new vector<Region>[imgStackTop->depth];

	distanceMap(rootsTop, *imgStackTop);
	skeletonise(*imgStackTop);
	aggregate(*imgStackTop);

	if(!acmMutex) acmMutex = CreateMutex(NULL, false, NULL);

	if(rootSys)
	{
		iterCount = 0;
		acmInit(*rootSys);
	}

	statsOutput(imgStack);
	sendConsoleCommand("print");
	if(!glutWnd)  CreateThread(NULL, 0, init, NULL, 0, NULL);
	if(!hCtrlDlg) CreateThread(NULL, 0, CtrlThread, NULL, 0, NULL);

	statusIndex = -1;
	PostMessage(hMainWnd, UM_UPDATE, IDUM_UPDATESCREEN, NULL);
}