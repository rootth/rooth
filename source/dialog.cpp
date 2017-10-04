//Macros
#define FREEGLUT_LIB_PRAGMAS 0//prevent freeglut.h from linking library
#undef  UNICODE

//System Dependencies
#include<Windows.h>
#include<Commctrl.h>
#include<sstream>
#include<iomanip>

using namespace std;

//Header Files
#include "../headers/GL/freeglut.h"
#include "../headers/main.h"
#include "../headers/skeletonisation.h"
#include "../headers/dialog.h"
#include "../headers/viewer.h"
#include "../headers/console.h"
#include "../headers/acm.h"

//Global Variables
HWND    hCtrlDlg = NULL;

bool analysed   = false;
bool merged     = false;
BUTTONS buttons = {true, true, true, true, true, true};

stack<UNDO> undo;

int statusIndex = 0;

//Static Global Variables
static char* statusText[] = 
{
	"",
	"1/5 Loading Image Stack into Memory...",
	"1/5 Loading VGM Stack into Memory...",
	"2/5 Applying Morphological Dilation...",
	"2/5 Applying Morphological Erosion...",
	"3/5 Calculating front to back sweep Control Points...",
	"4/5 Calculating left to right sweep Control Points...",
	"5/5 Skeletonising Root System..."
};

//Function Definitions
void updateLog(HWND hDlg)
{
	stringstream s;
	int numOfMrgLns = 0;

	for(int i=0; i<undo.size(); i++)
	{
		if(undo._Get_container()[i].action != UNDO::PRUNE)
			s<<i+1-numOfMrgLns<<" - ";
		else
			numOfMrgLns++;

		switch(undo._Get_container()[i].action)
		{
			case UNDO::DEL:
							
				s<<"Deleted root "<<undo._Get_container()[i].rootNum;
				break;
						
			case UNDO::DISCONN:

				s<<"Disc lat root "<<undo._Get_container()[i].latRootNum<<" from root "<<undo._Get_container()[i].rootNum;
				break;

			case UNDO::LOCK:

				s<<"Lock root "<<undo._Get_container()[i].rootNum <<" at sg/br ("<<undo._Get_container()[i].segRootNum<<","<<undo._Get_container()[i].latRootNum<<")";
				break;

			case UNDO::CHANGE:

				s<<"Change root "<<undo._Get_container()[i].rootNum<<" at s/b ("<<undo._Get_container()[i].segRootNum<<","<<undo._Get_container()[i].latRootNum<<")";
				break;

			case UNDO::MERGE:

				s<<"Merged root "<<undo._Get_container()[i].rootNum<<" with seg "<<undo._Get_container()[i].segRootNum;
				break;

			case UNDO::PRUNE:

				s<<"  *Set Pruning Threshold to "<<undo._Get_container()[i].threshold<<"*";
				break;
		}

		if(i<undo.size()-1) s<<"\r\n";
	}

	SetDlgItemText(hDlg, IDC_LOG, s.str().c_str());

	s<<"\r\n\r\nSmoothing Coefficient = "<<setprecision(2)<<minCoeff;

	for(int i=0; i<2*undo.size(); i++)
		SendMessage(GetDlgItem(hDlg, IDC_LOG), EM_SCROLL, SB_LINEDOWN, 0);

	//Copy content of log to the clupboard - DEBUGGING use
	HGLOBAL clipMem  = GlobalAlloc(GMEM_MOVEABLE, s.str().size()+1);
	char*   clipText = (char*)GlobalLock(clipMem);

	for(int i=0; i<s.str().size(); i++)
		clipText[i] = s.str()[i];
	clipText[s.str().size()] = '\0';

	GlobalUnlock(clipMem);

	OpenClipboard(hMainWnd);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, clipMem);
	CloseClipboard();
}

void scrollToMergedRoot(RootSys* rtsys, RootSys* mrgRoot, int level = 0)
{
	static int rootNum;

	if(!level) rootNum = 0;
	
	rootNum++;

	if(rtsys == mrgRoot) rootIndex.number = rootNum;

	for(int i=0; i<rtsys->latRoots.size(); i++)
		if(rtsys->latRoots[i]->size()>menuParam.prune)//DEBUG
			scrollToMergedRoot(rtsys->latRoots[i], mrgRoot, level+1);
}

INT_PTR CALLBACK CtrlProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int scrollpos = 15;

	switch(message)
	{
		case WM_INITDIALOG:
		{
			RECT rect = {};
			char str[256] = {};

			if(!hCtrlDlg)
			{
				hCtrlDlg = hDlg;

				GetWindowRect(hMainWnd, &rect);
				SetWindowPos(hDlg, NULL, rect.right+15, rect.top+5, 0, 0, SWP_NOSIZE + SWP_NOOWNERZORDER);
			}

			EnableMenuItem(GetMenu(hMainWnd), ID_TOOLS_CTRL, MF_BYCOMMAND + MF_DISABLED);

			CheckDlgButton(hDlg, IDC_LOCK, true);

			CheckDlgButton(hDlg, IDC_PAUSE,     display.spin);
			CheckDlgButton(hDlg, IDC_CURVE,     display.crvFitSkl);
			CheckDlgButton(hDlg, IDC_SNKPTS,    display.snakePoints);
			CheckDlgButton(hDlg, IDC_SNKSPL,    display.spline);
			CheckDlgButton(hDlg, IDC_HIGHLIGHT, display.highlightSeminals);

			EnableWindow(GetDlgItem(hDlg, IDC_HIGHLIGHT), display.spline);

			CheckDlgButton(hDlg, IDC_TOPDOWN,   display.ctrlPts.top);
			CheckDlgButton(hDlg, IDC_FRONTBACK, display.ctrlPts.front);
			CheckDlgButton(hDlg, IDC_LEFTRIGHT, display.ctrlPts.left);

			if(display.rootSys == DISPLAY::MESH)
				CheckRadioButton(hDlg, IDC_MESH, IDC_SKEL, IDC_MESH);
			else if(display.rootSys == DISPLAY::WALL)
				CheckRadioButton(hDlg, IDC_MESH, IDC_SKEL, IDC_WALL);
			else
				CheckRadioButton(hDlg, IDC_MESH, IDC_SKEL, IDC_SKEL);

			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)icon);

			SendMessage(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), PBM_SETRANGE, 0, MAKELONG(0, SCROLL_RANGE));

			if(analysed)
			{
				if(statusIndex == -1)
				{
					SendMessage(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), PBM_SETMARQUEE, 0, 0);

					SetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE, GetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE) & ~PBS_MARQUEE);
					SetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE, GetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE) | PBS_SMOOTH);

					statusIndex = 0;
				}
				
				SendMessage(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, SCROLL_RANGE, 0);
			}
			else if(statusIndex == 0)
			{
				SetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE, GetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE) & ~PBS_MARQUEE);
				SetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE, GetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE) | PBS_SMOOTH);

				SendMessage(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), PBM_SETPOS, 0, 0);
			}
			else if(statusIndex == 7)
			{
				SetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE, GetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE) & ~PBS_SMOOTH);
				SetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE, GetWindowLong(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), GWL_STYLE) | PBS_MARQUEE);

				SendMessage(GetDlgItem(hDlg, IDC_PROGRESSANALYSE), PBM_SETMARQUEE, 1, 10);
			}

			SendMessage(GetDlgItem(hDlg, IDC_PROGRESSMERGE), PBM_SETRANGE, 0, MAKELONG(0, SCROLL_RANGE));
			SendMessage(GetDlgItem(hDlg, IDC_PROGRESSMERGE), PBM_SETPOS, merged?SCROLL_RANGE:0, 0);

			SendMessage(GetDlgItem(hDlg, IDC_SIMULSTRENSTRTCH), TBM_SETRANGE, TRUE, MAKELONG(0, SCROLL_RANGE));
			SendMessage(GetDlgItem(hDlg, IDC_SIMULSTRENSTRTCH), TBM_SETPOS, TRUE, scrollpos);

			SendMessage(GetDlgItem(hDlg, IDC_STRETCHSPIN), UDM_SETRANGE, 0, MAKELONG(0, SPIN_RANGE));
			SendMessage(GetDlgItem(hDlg, IDC_STRETCHSPIN), UDM_SETPOS, 0, int(SPIN_RANGE-alpha));

			SendMessage(GetDlgItem(hDlg, IDC_STIFFNESSSPIN), UDM_SETRANGE, 0, MAKELONG(0, SPIN_RANGE));
			SendMessage(GetDlgItem(hDlg, IDC_STIFFNESSSPIN), UDM_SETPOS, 0, int(SPIN_RANGE-beta));

			SendMessage(GetDlgItem(hDlg, IDC_ATTRACTSPIN), UDM_SETRANGE, 0, MAKELONG(0, SPIN_RANGE));
			SendMessage(GetDlgItem(hDlg, IDC_ATTRACTSPIN), UDM_SETPOS, 0, int(SPIN_RANGE-gamma));

			SendMessage(GetDlgItem(hDlg, IDC_REPULSESPIN), UDM_SETRANGE, 0, MAKELONG(0, SPIN_RANGE));
			SendMessage(GetDlgItem(hDlg, IDC_REPULSESPIN), UDM_SETPOS, 0, int(SPIN_RANGE-delta));

			int partitions[2] = {345, -1};

			SendMessage(GetDlgItem(hDlg, IDS_STATUSBAR), SB_SETPARTS, 2, (LPARAM)partitions);
			SendMessage(GetDlgItem(hCtrlDlg, IDS_STATUSBAR), SB_SETTEXT, 0, (LPARAM)statusText[statusIndex]);

			SetDlgItemText(hDlg, IDC_AUTO, buttons.calibrate?"Terminate":"Calibrate");

			wsprintf(str, "%i", int(alpha));
			SetDlgItemText(hDlg, IDC_STRETCH, str);

			wsprintf(str, "%i", int(beta));
			SetDlgItemText(hDlg, IDC_STIFFNESS, str);

			wsprintf(str, "%i", int(gamma));
			SetDlgItemText(hDlg, IDC_ATTRACT, str);

			wsprintf(str, "%i", int(delta));
			SetDlgItemText(hDlg, IDC_REPULSE, str);

			if(buttons.classification && currRoot)
			{
				if(currRoot->type == RootSys::PRIMARY)
					CheckRadioButton(hDlg, IDC_PRIMARY, IDC_LATERAL, IDC_PRIMARY);
				else if(currRoot->type == RootSys::SEMINAL)
					CheckRadioButton(hDlg, IDC_PRIMARY, IDC_LATERAL, IDC_SEMINAL);
				else if(currRoot->type == RootSys::LATERAL)
					CheckRadioButton(hDlg, IDC_PRIMARY, IDC_LATERAL, IDC_LATERAL);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_PRIMARY, false);
				CheckDlgButton(hDlg, IDC_SEMINAL, false);
				CheckDlgButton(hDlg, IDC_LATERAL, false);
			}

			EnableWindow(GetDlgItem(hDlg, IDC_ANALYSE),  buttons.analyse);

			EnableWindow(GetDlgItem(hDlg,  IDC_PRIMARY), buttons.classification);
			EnableWindow(GetDlgItem(hDlg,  IDC_SEMINAL), buttons.classification);
			EnableWindow(GetDlgItem(hDlg,  IDC_LATERAL), buttons.classification);

			EnableWindow(GetDlgItem(hDlg, IDC_CHANGE), analysed && display.crvFitSkl);
			EnableWindow(GetDlgItem(hDlg, IDC_DELETE), (analysed && (display.crvFitSkl || display.spline)) || (buttons.classification && display.spline));
			EnableWindow(GetDlgItem(hDlg, IDC_LOCK),   analysed && display.crvFitSkl);
			EnableWindow(GetDlgItem(hDlg, IDC_UNDO),   analysed && display.crvFitSkl);

			EnableWindow(GetDlgItem(hDlg, IDC_MERGE), analysed && buttons.merge);
			EnableWindow(GetDlgItem(hDlg, IDC_START), analysed && buttons.start);
			EnableWindow(GetDlgItem(hDlg, IDC_STOP),  analysed && buttons.stop);
			EnableWindow(GetDlgItem(hDlg, IDC_AUTO),  analysed && buttons._auto);
			EnableWindow(GetDlgItem(hDlg, IDC_RESET), analysed && buttons.reset);

			EnableWindow(GetDlgItem(hDlg, IDC_STRETCH),			 analysed && buttons.acmParam);
			EnableWindow(GetDlgItem(hDlg, IDC_STRETCHSPIN),		 analysed && buttons.acmParam);
			EnableWindow(GetDlgItem(hDlg, IDC_STIFFNESS),		 analysed && buttons.acmParam);
			EnableWindow(GetDlgItem(hDlg, IDC_STIFFNESSSPIN),	 analysed && buttons.acmParam);
			EnableWindow(GetDlgItem(hDlg, IDC_ATTRACT),			 analysed && buttons.acmParam);
			EnableWindow(GetDlgItem(hDlg, IDC_ATTRACTSPIN),		 analysed && buttons.acmParam);
			EnableWindow(GetDlgItem(hDlg, IDC_REPULSE),		     analysed && buttons.acmParam);
			EnableWindow(GetDlgItem(hDlg, IDC_REPULSESPIN),	     analysed && buttons.acmParam);
			EnableWindow(GetDlgItem(hDlg, IDC_SIMULSTRENSTRTCH), analysed && buttons.acmParam);

			updateLog(hDlg);

			return (INT_PTR)TRUE;
		}

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
				
					hCtrlDlg = NULL;
					EnableMenuItem(GetMenu(hMainWnd), ID_TOOLS_CTRL, MF_BYCOMMAND + MF_ENABLED);
					EndDialog(hDlg, LOWORD(wParam));
					
					return (INT_PTR)TRUE;
			
				case IDC_TOPDOWN:

					display.ctrlPts.top = IsDlgButtonChecked(hDlg, IDC_TOPDOWN)!=0;//to disable warning C4800;
					break;

				case IDC_FRONTBACK:

					display.ctrlPts.front = IsDlgButtonChecked(hDlg, IDC_FRONTBACK)!=0;//to disable warning C4800;
					break;

				case IDC_LEFTRIGHT:

					display.ctrlPts.left = IsDlgButtonChecked(hDlg, IDC_LEFTRIGHT)!=0;//to disable warning C4800;
					break;

				case IDC_SNKPTS:
					
					display.snakePoints = IsDlgButtonChecked(hDlg, IDC_SNKPTS)!=0;//to disable warning C4800;
					break;

				case IDC_SNKSPL:
					
					display.spline = IsDlgButtonChecked(hDlg, IDC_SNKSPL)!=0;//to disable warning C4800;

					SendMessage(hDlg, WM_INITDIALOG, 0, 0);

					break;

				case IDC_CURVE:
					
					display.crvFitSkl = IsDlgButtonChecked(hDlg, IDC_CURVE)!=0;//to disable warning C4800;

					SendMessage(hDlg, WM_INITDIALOG, 0, 0);

					break;

				case IDC_HIGHLIGHT:

					display.highlightSeminals = IsDlgButtonChecked(hDlg, IDC_HIGHLIGHT)!=0;//to disable warning C4800;

					break;

				case IDC_MESH:

					if(IsDlgButtonChecked(hDlg, IDC_MESH))
					{
						display.rootSys = DISPLAY::MESH;

						if(glutWnd) glutDisplayFunc(drawWall);
					}
					
					goto updTitle;

				case IDC_WALL:

					if(IsDlgButtonChecked(hDlg, IDC_WALL))
					{
						display.rootSys = DISPLAY::WALL;
					
						if(glutWnd) glutDisplayFunc(drawWall);
					}
					
					goto updTitle;

				case IDC_SKEL:

					if(IsDlgButtonChecked(hDlg, IDC_SKEL))
					{
						display.rootSys = DISPLAY::SKEL;

						if(glutWnd) glutDisplayFunc(drawSkeleton);
					}
					
					goto updTitle;

				case IDC_PAUSE:

					display.spin = IsDlgButtonChecked(hDlg, IDC_PAUSE)!=0;//to disable warning C4800;

					break;

				case IDC_ROOTPRV:

					if(rootIndex.number>0) rootIndex.number--;
					goto updDlg;

				case IDC_ROOTNEXT:

					rootIndex.number++;
					goto updDlg;

				case IDC_SCROLLUP:

					wheel(NULL, -1, NULL, NULL);
					break;

				case IDC_SCROLLDOWN:

					wheel(NULL, +1, NULL, NULL);
					break;

				case IDC_SEGPRV:

					if(rootIndex.segment>0) rootIndex.segment--;
					goto updDlg;

				case IDC_SEGNEXT:

					rootIndex.segment++;
					goto updDlg;

				case IDC_LEVELUP:

					if(rootIndex.level) rootIndex.level--;
					goto updDlg;

				case IDC_LEVELDOWN:

					rootIndex.level++;

updDlg:			SendMessage(hDlg, WM_INITDIALOG, 0, 0);
updTitle:		if(glutWnd) glutTimerFunc(50, updateTitle, 0);
				break;

				case IDC_START:

					if(imgStackTop && rootSys)
					{
						if(!merged && !popupQuestion("Control points from different sweeps are not combined.\nRun ACM anyway?", true)) break;

						buttons.start = false;
						buttons._auto = false;
						buttons.reset = false;

						SendMessage(hDlg, WM_INITDIALOG, 0, 0);

						runACM = true;
						CreateThread(NULL, 0, ACMThreadIter, rootSys, 0, NULL);
					}

					break;

				case IDC_STOP:

					if(imgStackTop && rootSys)
					{
						buttons.start = true;
						buttons._auto = true;
						buttons.reset = true;

						SendMessage(hDlg, WM_INITDIALOG, 0, 0);
						if(glutWnd) glutTimerFunc(50, updateTitle, 0);

						runACM = false;
					}

					break;

				case IDC_AUTO:

					if(imgStackTop && rootSys)
					{
						if(buttons.acmParam)
						{
							if(!merged)
							{
								popupError("Control points from different sweeps are not combined.\n"\
										   "Please merge points before running Auto ACM");
								break;
							}

							EnableMenuItem(GetMenu(hMainWnd), ID_FILE_CLOSE, MF_BYCOMMAND + MF_DISABLED);

							runCalibrate = true;

							buttons.start = false;
							buttons.stop  = false;
							buttons.reset = false;

							buttons.acmParam = false;

							buttons.calibrate = BUTTONS::TERMINATE;
						
							SendMessage(hDlg, WM_INITDIALOG, 0, 0);

							CreateThread(NULL, 0, ACMThreadProc, rootSys, 0, NULL);
						}
						else
						{
							runCalibrate = false;

							buttons.start = true;
							buttons.stop  = true;
							buttons.reset = true;

							buttons.acmParam = true;

							buttons.calibrate = BUTTONS::CALIBRATE;
						
							SendMessage(hDlg, WM_INITDIALOG, 0, 0);
						}
					}

					break;

				case IDC_RESET:

					if(imgStackTop && rootSys)
					{
						iterCount = 0;
						acmInit(*rootSys);

						//Clear Status Bar Iteration Counter
						SendMessage(GetDlgItem(hCtrlDlg, IDS_STATUSBAR), SB_SETTEXT, MAKEWPARAM(1, 0), (LPARAM)"");

						//Update Connecting Lines Length Value
						statsOutput(imgStack, false, false, false);
						sendConsoleCommand("print");

						InvalidateRect(hMainWnd, NULL, false);
						UpdateWindow(hMainWnd);
					}

					break;

				case IDC_ANALYSE:

					buttons.classification = false;
					buttons.analyse = false;

					SendMessage(hDlg, WM_INITDIALOG, 0, 0);
					SendMessage(hMainWnd, UM_UPDATE, IDUM_ANALYSE, 0);

					EnableMenuItem(GetMenu(hMainWnd), ID_FILE_CLOSE, MF_BYCOMMAND + MF_DISABLED);

					break;

				case IDC_MERGE:

					if(imgStackTop && rootSys)
					{
						//Reset progress bar
						merged = false;
						SendMessage(hDlg, WM_INITDIALOG, 0, 0);
						
						CreateThread(NULL, 0, cmbCtrlPtsThread, rootSys, 0, NULL);
					}

					break;

				case IDC_CHANGE:

					if(imgStackTop && pskl && rootSys)
					{
						if(runACM || runCalibrate)
						{
							popupError("The Active Cotour Model is currently running.\nPlease stop it first before making further changes to the root system");
							break;
						}

						if(buttons.classification)
							if(popupQuestion("This action will reset manual root classification.\nContinue?", true))
							{
								buttons.classification = false;

								SendMessage(hDlg, WM_INITDIALOG, 0, 0);
							}
							else break;

						//Reset merge progress bar
						merged = false;
						SendMessage(hDlg, WM_INITDIALOG, 0, 0);

						if(currRoot && tchRoot)
						{
							WaitForSingleObject(skelMutex, INFINITE);

								currRoot->insert(currRoot->end(), tchRoot->latRoots[tchInd]->begin(),
																  tchRoot->latRoots[tchInd]->end());

								currRoot->latRoots.insert(currRoot->latRoots.begin(), tchRoot->latRoots[tchInd]->latRoots.begin(),
																					  tchRoot->latRoots[tchInd]->latRoots.end());

								currRoot->regs.insert(currRoot->regs.begin(), tchRoot->latRoots[tchInd]->regs.begin(),
																			  tchRoot->latRoots[tchInd]->regs.end());

								currRoot->sideCtrlPtsFront.insert(currRoot->sideCtrlPtsFront.begin(), tchRoot->latRoots[tchInd]->sideCtrlPtsFront.begin(),
																									  tchRoot->latRoots[tchInd]->sideCtrlPtsFront.end());

								currRoot->sideCtrlPtsLeft.insert(currRoot->sideCtrlPtsLeft.begin(), tchRoot->latRoots[tchInd]->sideCtrlPtsLeft.begin(),
																									tchRoot->latRoots[tchInd]->sideCtrlPtsLeft.end());
								
								tchRoot->latRoots[tchInd]->latRoots.clear();
								delete tchRoot->latRoots[tchInd];

								tchRoot->latRoots.erase(tchRoot->latRoots.begin() + tchInd);

								for(int r=0; r<currRoot->size(); r++)
									if((*currRoot)[r].pskl) (*currRoot)[r].pskl->prtsys = currRoot;

								manMrgRts.push(ManualMerge(currSkel, tchSkel, tchInd));
								undo.push(UNDO(UNDO::MERGE, rootIndex.number, rootIndex.length));
								scrollToMergedRoot(rootSys, currRoot);

							ReleaseMutex(skelMutex);
						}
						else
						{
							pskl->nbrSegChoice = pRegChoice;//rootSegToShow;
			
							//Lock the path of the root above the selection point
							if(prtsys && (hDlg && IsDlgButtonChecked(hDlg, IDC_LOCK) || !hDlg && glutWnd && glutGetModifiers()==GLUT_ACTIVE_SHIFT))
							{
								RootSys& rs = *prtsys;
								vector<Region::Skeleton*> lockedSkels;

								for(int i=0; i<rs.size(); i++)
								{
									if(rs[i].pskl == pskl) break;

									if(rs[i].pNextReg)
									{
										rs[i].pskl->nbrSegChoice = rs[i].pNextReg;
										lockedSkels.push_back(rs[i].pskl);
									}
								}

								undo.push(UNDO(pskl, lockedSkels, UNDO::LOCK, rootIndex.number, rootIndex.segment, rootIndex.length));
							}
							else
								undo.push(UNDO(pskl, UNDO::CHANGE, rootIndex.number, rootIndex.segment, rootIndex.length));

							aggregate(imgStack);
						}

						iterCount = 0;
						acmInit(*rootSys);

						updateLog(hDlg);
						statsOutput(imgStack);
						sendConsoleCommand("print");
					}

					break;

				case IDC_DELETE:

					if(rootSys)
					{
						if(runACM || runCalibrate)
						{
							popupError("The Active Cotour Model is currently running.\nPlease stop it first before making further changes to the root system");
							break;
						}

						if(currRoot)
						{
							WaitForSingleObject(skelMutex, INFINITE);
								
								currRoot->parSnkPt = VECTOR();
								currRoot->acmpts.clear();
								currRoot->curve.clear();
								currRoot->clear();

								if(currRoot->pParReg)
								{
									currRoot->pParReg->skl.latRootSkip.insert(currRoot->pInitReg);
									undo.push(UNDO(&currRoot->pParReg->skl, UNDO::DEL, rootIndex.number));
								}

							ReleaseMutex(skelMutex);

							statsOutput(imgStack, false, false, false);
							sendConsoleCommand("print");
						}
						else if(latRoot && latRoot->pParReg)
						{
							if(buttons.classification)
								if(popupQuestion("This action will reset manual root classification.\nContinue?", true))
								{
									buttons.classification = false;

									SendMessage(hDlg, WM_INITDIALOG, 0, 0);
								}
								else break;

							//Reset merge progress bar
							merged = false;
							SendMessage(hDlg, WM_INITDIALOG, 0, 0);

							undo.push(UNDO(&latRoot->pParReg->skl, UNDO::DISCONN, rootIndex.number, rootIndex.lateral));
							
							WaitForSingleObject(skelMutex, INFINITE);
								latRoot->pParReg->skl.latRootSkip.insert(latRoot->pInitReg);
							ReleaseMutex(skelMutex);

							aggregate(imgStack);

							iterCount = 0;
							acmInit(*rootSys);

							statsOutput(imgStack);
							sendConsoleCommand("print");
						}

						updateLog(hDlg);
					}

					break;

				case IDC_PRIMARY:

					if(currRoot) currRoot->type = RootSys::PRIMARY;
					SendMessage(hDlg, WM_INITDIALOG, 0, 0);
					break;

				case IDC_SEMINAL:

					if(currRoot) currRoot->type = RootSys::SEMINAL;
					SendMessage(hDlg, WM_INITDIALOG, 0, 0);
					break;

				case IDC_LATERAL:

					if(currRoot) currRoot->type = RootSys::LATERAL;
					SendMessage(hDlg, WM_INITDIALOG, 0, 0);
					break;

				case IDC_UNDO:

					if(undo.size() && rootSys)
					{
						if(runACM || runCalibrate)
						{
							popupError("The Active Cotour Model is currently running.\nPlease stop it first before making further changes to the root system");
							break;
						}

						if(buttons.classification)
							if(popupQuestion("This action will reset manual root classification.\nContinue?", true))
							{
								buttons.classification = false;

								SendMessage(hDlg, WM_INITDIALOG, 0, 0);
							}
							else break;

						//Reset progress bar
						merged = false;
						SendMessage(hDlg, WM_INITDIALOG, 0, 0);

						UNDO::ACT act;
						Region::Skeleton* cs = NULL;

						WaitForSingleObject(skelMutex, INFINITE);
							if(currSkel) cs = currSkel;
						ReleaseMutex(skelMutex);

						do
						{
							switch(act = undo.top().action)
							{
								case UNDO::LOCK:

									for(int i=0; i<undo.top().lckSkls.size(); i++)
										undo.top().lckSkls[i]->nbrSegChoice = NULL;

								case UNDO::CHANGE:
					
									undo.top().skl->nbrSegChoice = NULL;
									break;

								case UNDO::DEL:
								case UNDO::DISCONN:
						
									undo.top().skl->latRootSkip.clear();
									break;

								case UNDO::MERGE:

									manMrgRts.pop();
									break;
							}

							undo.pop();
						}
						while(act == UNDO::PRUNE && undo.size());

						aggregate(imgStack);

						iterCount = 0;
						acmInit(*rootSys);

						if(cs) scrollToMergedRoot(rootSys, cs->prtsys);

						updateLog(hDlg);
						statsOutput(imgStack);
						sendConsoleCommand("print");
					}

					break;
			}

			if(HIWORD(wParam)==EN_CHANGE)
			{
				char str[256] = {};

				switch(LOWORD(wParam))
				{
					case IDC_STRETCH:

						GetDlgItemText(hDlg, IDC_STRETCH, str, 256);
						alpha = atoi(str);
						SendMessage(GetDlgItem(hDlg, IDC_STRETCHSPIN), UDM_SETPOS, 0, int(SPIN_RANGE-alpha));

						break;

					case IDC_STIFFNESS:

						GetDlgItemText(hDlg, IDC_STIFFNESS, str, 256);
						beta = atoi(str);
						SendMessage(GetDlgItem(hDlg, IDC_STIFFNESSSPIN), UDM_SETPOS, 0, int(SPIN_RANGE-beta));

						break;

					case IDC_ATTRACT:
					
						GetDlgItemText(hDlg, IDC_ATTRACT, str, 256);
						gamma = atoi(str);
						SendMessage(GetDlgItem(hDlg, IDC_ATTRACTSPIN), UDM_SETPOS, 0, int(SPIN_RANGE-gamma));

						break;

					case IDC_REPULSE:

						GetDlgItemText(hDlg, IDC_REPULSE, str, 256);
						delta = atoi(str);
						SendMessage(GetDlgItem(hDlg, IDC_REPULSESPIN), UDM_SETPOS, 0, int(SPIN_RANGE-delta));

						break;
				}
			}

			break;
		}

		case WM_HSCROLL:
		{
			if(LOWORD(wParam) == TB_THUMBTRACK)
			{
				char str[256] = {};

				if((HWND)lParam == GetDlgItem(hDlg, IDC_SIMULSTRENSTRTCH))
				{
					LRESULT pos = HIWORD(wParam);

					alpha += pos-scrollpos;
					beta  += pos-scrollpos;

					scrollpos = int(pos);

					wsprintf(str, "%i", int(alpha));
					SetDlgItemText(hDlg, IDC_STRETCH, str);

					wsprintf(str, "%i", int(beta));
					SetDlgItemText(hDlg, IDC_STIFFNESS, str);
				}
			}

			break;
		}

		case WM_VSCROLL:
		{
			char str[256] = {};

			if((HWND)lParam == GetDlgItem(hDlg, IDC_STRETCHSPIN))
			{
				if(LOWORD(wParam) == SB_THUMBPOSITION)
				{
					alpha = SPIN_RANGE-HIWORD(wParam);

					wsprintf(str, "%i", int(alpha));
					SetDlgItemText(hDlg, IDC_STRETCH, str);
				}
			}
			else if((HWND)lParam == GetDlgItem(hDlg, IDC_STIFFNESSSPIN))
			{
				if(LOWORD(wParam) == SB_THUMBPOSITION)
				{
					beta = SPIN_RANGE-HIWORD(wParam);

					wsprintf(str, "%i", int(beta));
					SetDlgItemText(hDlg, IDC_STIFFNESS, str);
				}
			}
			else if((HWND)lParam == GetDlgItem(hDlg, IDC_ATTRACTSPIN))
			{
				if(LOWORD(wParam) == SB_THUMBPOSITION)
				{
					gamma = SPIN_RANGE-HIWORD(wParam);

					wsprintf(str, "%i", int(gamma));
					SetDlgItemText(hDlg, IDC_ATTRACT, str);
				}
			}
			else if((HWND)lParam == GetDlgItem(hDlg, IDC_REPULSESPIN))
			{
				if(LOWORD(wParam) == SB_THUMBPOSITION)
				{
					delta = SPIN_RANGE-HIWORD(wParam);

					wsprintf(str, "%i", int(delta));
					SetDlgItemText(hDlg, IDC_REPULSE, str);
				}
			}
				
			break;
		}

		case WM_MOUSEWHEEL:
		{
			for(int i=0; i<abs(GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA); i++)
				wheel(NULL, GET_WHEEL_DELTA_WPARAM(wParam), NULL, NULL);
			break;
		}
	}

	return (INT_PTR)FALSE;
}

DWORD WINAPI CtrlThread(LPVOID lpParam)
{
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CTRL), NULL, CtrlProc);
	return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static MENUFLAGS dlgMenuFlags;
	static MENUPARAM dlgMenuParam;

	switch(message)
	{
		case WM_INITDIALOG:
		{
			dlgMenuFlags = menuFlags;
			dlgMenuParam = menuParam;

			CheckDlgButton(hDlg, IDC_FILL, dlgMenuFlags.fill);
			CheckDlgButton(hDlg, IDC_AUTOSMTH, dlgMenuParam.autoSmthCoeff);

			EnableWindow(GetDlgItem(hDlg, IDC_SMOOTH), !dlgMenuParam.autoSmthCoeff);
			EnableWindow(GetDlgItem(hDlg, IDC_SMTHVAL), !dlgMenuParam.autoSmthCoeff);
			EnableWindow(GetDlgItem(hDlg, IDC_CROSSVAL), dlgMenuParam.autoSmthCoeff);
			EnableWindow(GetDlgItem(hDlg, IDC_K), dlgMenuParam.autoSmthCoeff);
			
			char str[256] = {};

			wsprintf(str, dlgMenuParam.smthCoeff==100?"1.00":dlgMenuParam.smthCoeff<10?"0.0%i":"0.%i", dlgMenuParam.smthCoeff);
			SetDlgItemText(hDlg, IDC_SMTHVAL, str);
			SendMessage(GetDlgItem(hDlg, IDC_SMOOTH), TBM_SETPOS, TRUE, dlgMenuParam.smthCoeff);

			wsprintf(str, "%i", dlgMenuParam.permDist);
			SetDlgItemText(hDlg, IDC_HEURISTICS, str);

			wsprintf(str, "%i", dlgMenuParam.K);
			SetDlgItemText(hDlg, IDC_K, str);
			SendMessage(GetDlgItem(hDlg, IDC_CROSSVAL), TBM_SETRANGE, TRUE, MAKELONG(1, 100));
			SendMessage(GetDlgItem(hDlg, IDC_CROSSVAL), TBM_SETPOS, TRUE, dlgMenuParam.K);

			wsprintf(str, "%i", dlgMenuParam.morph);
			SetDlgItemText(hDlg, IDC_MORPHVAL, str);
			SendMessage(GetDlgItem(hDlg, IDC_MORPH), TBM_SETRANGE, TRUE, MAKELONG(0, 15));
			SendMessage(GetDlgItem(hDlg, IDC_MORPH), TBM_SETPOS, TRUE, dlgMenuParam.morph);

			wsprintf(str, "%i", dlgMenuParam.prune);
			SetDlgItemText(hDlg, IDC_EDITPRUNE, str);

			wsprintf(str, "%i", dlgMenuParam.sideCtrlPtsThrld);
			SetDlgItemText(hDlg, IDC_SIDEPRUNE, str);

			wsprintf(str, "%i", dlgMenuParam.res);
			SetDlgItemText(hDlg, IDC_EDITRES, str);

			return (INT_PTR)TRUE;
		}

		case WM_COMMAND:
		{
			if(LOWORD(wParam) == IDC_OK || LOWORD(wParam) == IDC_APPLY)
			{
				char     str[256] = {};
				unsigned oldPrune = menuParam.prune;
				unsigned oldRes   = menuParam.res;

				Region::Skeleton* cs = NULL;

				if(glutWnd)
				{
					WaitForSingleObject(skelMutex, INFINITE);
						if(currSkel) cs = currSkel;
					ReleaseMutex(skelMutex);
				}
				
				GetDlgItemText(hDlg, IDC_EDITPRUNE, str, 255);
				menuParam.prune = (unsigned)atoi(str);

				GetDlgItemText(hDlg, IDC_EDITRES, str, 255);
				menuParam.res = (unsigned)atoi(str);

				if(rootSys)
				{
					if(menuParam.prune!=oldPrune)
					{
						if(runACM || runCalibrate)
						{
							popupError("The Active Cotour Model is currently running.\nPlease stop it first before changing the pruning threshold");
							
							menuParam.prune = oldPrune;
							menuParam.res   = oldRes;

							return (INT_PTR)TRUE;
						}

						if(buttons.classification)
							if(popupQuestion("Changing the Parasitic Roots Pruning Threshold\n"\
											 "will reset the ACM splines and manual roots classfication.\n"\
											 "Continue?", true))
								buttons.classification = false;
							else
							{
								menuParam.prune = oldPrune;
								menuParam.res   = oldRes;

								return (INT_PTR)TRUE;
							}

						SendMessage(hCtrlDlg, WM_INITDIALOG, 0, 0);

						undo.push(UNDO(UNDO::PRUNE, menuParam.prune));
						updateLog(hCtrlDlg);

						iterCount = 0;
						aggregate(imgStack);
						acmInit(*rootSys);
						statsOutput(imgStack);

						if(cs) scrollToMergedRoot(rootSys, cs->prtsys);
						updateTitle();
					}
					else if(menuParam.res!=oldRes) statsOutput(imgStack, false, false, false);
					
					if(menuParam.prune!=oldPrune || menuParam.res!=oldRes) sendConsoleCommand("print");
				}

				menuFlags = dlgMenuFlags;
				menuParam.autoSmthCoeff = dlgMenuParam.autoSmthCoeff;

				if(!dlgMenuParam.autoSmthCoeff)
					menuParam.smthCoeff = (unsigned)SendMessage(GetDlgItem(hDlg, IDC_SMOOTH), TBM_GETPOS, 0, 0);
				
				menuParam.K = (unsigned)SendMessage(GetDlgItem(hDlg, IDC_CROSSVAL), TBM_GETPOS, 0, 0);
				menuParam.morph = (unsigned)SendMessage(GetDlgItem(hDlg, IDC_MORPH), TBM_GETPOS, 0, 0);

				GetDlgItemText(hDlg, IDC_HEURISTICS, str, 255);
				menuParam.permDist = (unsigned)atoi(str);

				GetDlgItemText(hDlg, IDC_SIDEPRUNE, str, 255);
				menuParam.sideCtrlPtsThrld = (unsigned)atoi(str);

				if(LOWORD(wParam) == IDC_OK) EndDialog(hDlg, LOWORD(wParam));
				
				return (INT_PTR)TRUE;
			}

			if(LOWORD(wParam) == IDC_CANCEL || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}

			if(LOWORD(wParam) == IDC_FILL)
			{
				dlgMenuFlags.fill = IsDlgButtonChecked(hDlg, IDC_FILL)!=0;//to disable warning C4800
				
				break;
			}

			if(LOWORD(wParam) == IDC_AUTOSMTH)
			{
				dlgMenuParam.autoSmthCoeff = IsDlgButtonChecked(hDlg, IDC_AUTOSMTH)!=0;//to disable warning C4800

				EnableWindow(GetDlgItem(hDlg, IDC_SMOOTH), !dlgMenuParam.autoSmthCoeff);
				EnableWindow(GetDlgItem(hDlg, IDC_SMTHVAL), !dlgMenuParam.autoSmthCoeff);
				EnableWindow(GetDlgItem(hDlg, IDC_CROSSVAL), dlgMenuParam.autoSmthCoeff);
				EnableWindow(GetDlgItem(hDlg, IDC_K), dlgMenuParam.autoSmthCoeff);

				break;
			}
		}

		case WM_HSCROLL:
		{
			if(LOWORD(wParam) == TB_THUMBTRACK)
			{
				char str[256] = {};

				if((HWND)lParam == GetDlgItem(hDlg, IDC_SMOOTH))
				{
					LRESULT pos = HIWORD(wParam);
					wsprintf(str, pos==100?"1.00":pos<10?"0.0%i":"0.%i", pos);
					SetDlgItemText(hDlg, IDC_SMTHVAL, str);

					break;
				}

				if((HWND)lParam == GetDlgItem(hDlg, IDC_CROSSVAL))
				{
					LRESULT pos = HIWORD(wParam);
					wsprintf(str, "%i", pos);
					SetDlgItemText(hDlg, IDC_K, str);

					break;
				}

				if((HWND)lParam == GetDlgItem(hDlg, IDC_MORPH))
				{
					LRESULT pos = HIWORD(wParam);
					wsprintf(str, "%i", pos);
					SetDlgItemText(hDlg, IDC_MORPHVAL, str);

					break;
				}
			}
		}
	}

	return (INT_PTR)FALSE;
}

bool popupInfo(std::string text, bool confirm)
{
	if(confirm)
		return MessageBox(NULL, text.c_str(), title, MB_ICONINFORMATION + MB_OKCANCEL) == IDOK;
	else
	{
		MessageBox(NULL, text.c_str(), title, MB_ICONINFORMATION);
		return false;
	}
}

bool popupQuestion(std::string text, bool confirm)
{
	if(confirm)
		return MessageBox(NULL, text.c_str(), title, MB_ICONQUESTION + MB_YESNO + MB_TOPMOST) == IDYES;
	else
	{
		MessageBox(NULL, text.c_str(), title, MB_ICONQUESTION + MB_TOPMOST);
		return false;
	}
}

bool popupWarning(std::string text, bool confirm)
{
	if(confirm)
		return MessageBox(NULL, text.c_str(), title, MB_ICONWARNING + MB_CANCELTRYCONTINUE + MB_TOPMOST) == IDCONTINUE;
	else
	{
		MessageBox(NULL, text.c_str(), title, MB_ICONWARNING + MB_TOPMOST);
		return false;
	}
}

bool popupError(std::string text, bool confirm)
{
	if(confirm)
		return MessageBox(NULL, text.c_str(), title, MB_ICONERROR + MB_OKCANCEL + MB_TOPMOST) == IDOK;
	else
	{
		MessageBox(NULL, text.c_str(), title, MB_ICONERROR + MB_TOPMOST);
		return false;
	}
}