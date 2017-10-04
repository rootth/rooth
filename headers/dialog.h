#pragma once

//Macros
#undef UNICODE

//System Dependencies
#include<Windows.h>
#include<string>

//Header Files
#include "../headers/distancemap.h"
#include "../headers/fractalise.h"

//Structure Definitions
struct BUTTONS
{
	bool merge;
	bool start;
	bool stop;
	bool _auto;
	bool reset;
	bool acmParam;
	enum {CALIBRATE, TERMINATE} calibrate;
	bool classification;
	bool analyse;
};

struct UNDO
{
	Region::Skeleton* skl;
	vector<Region::Skeleton*> lckSkls;
	enum ACT {DEL, DISCONN, LOCK, CHANGE, MERGE, PRUNE} action;
	int rootNum, latRootNum, segRootNum;
	int threshold;

	UNDO(UNDO::ACT a, int t): action(a), threshold(t){}
	UNDO(UNDO::ACT a, int rn, int segn): action(a), skl(NULL), rootNum(rn), latRootNum(0), segRootNum(segn){}
	UNDO(Region::Skeleton* s, UNDO::ACT a, int rn=0, int lrn=0, int segn=0): skl(s), action(a), rootNum(rn), latRootNum(lrn), segRootNum(segn){}
	UNDO(Region::Skeleton* s, vector<Region::Skeleton*>& ls, UNDO::ACT a, int rn, int lrn, int segn):
	skl(s), lckSkls(ls), action(a), rootNum(rn), latRootNum(lrn), segRootNum(segn){}
};

//Global Variables
extern HWND hCtrlDlg;

extern bool analysed;
extern bool merged;
extern BUTTONS buttons;
extern stack<UNDO> undo;
extern int statusIndex;

const int SPIN_RANGE = 1000;
const int SCROLL_RANGE = 100;

//Global Function Declarations
INT_PTR CALLBACK CtrlProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

bool popupInfo(std::string text, bool confirm = false);
bool popupQuestion(std::string text, bool confirm = false);
bool popupWarning(std::string text, bool confirm = false);
bool popupError(std::string text, bool confirm = false);