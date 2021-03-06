// Used by resource.rc
#pragma once

#define START_STR "Start &Rendering Video...\tCtrl+R"
#define STOP_STR  "Stop &Rendering Video\tCtrl+R"

//Menu
#define IDI_ICON						100

#define IDR_MENU                        40001
#define ID_FILE_OPEN                    40002
#define ID_FILE_SAVEAS                  40003
#define ID_FILE_CLOSE                   40004
#define ID_FILE_IMPORT					40005
#define ID_FILE_EXPORT					40006
#define ID_FILE_TRAITS					40007
#define ID_FILE_MESH					40008
#define ID_FILE_EXIT                    40009
#define ID_VIEW_ORIGINAL                40010
#define ID_VIEW_ANALYSED                40011
#define	ID_VIEW_REGRESSION				40012
#define ID_VIEW_MSE						40013
#define ID_VIEW_UNDETECTED				40014

#define ID_TOOLS_VIEWER					40016
#define ID_TOOLS_CTRL					40017
#define ID_TOOLS_CONSOLE				40018
#define	ID_TOOLS_OPTIONS				40019
#define ID_HELP_ABOUT                   40020

#define IDD_OPTIONS						40021
#define IDC_FILL						40022
#define IDC_AUTOSMTH					40023
#define IDC_SMOOTH						40024
#define IDC_SMTHVAL						40025
#define IDC_MORPH						40026
#define IDC_MORPHVAL					40027
#define	IDC_CROSSVAL					40028
#define IDC_K							40029
#define IDC_HEURISTICS					40030
//#define IDC_H							40031
#define IDC_EDITPRUNE					40032
#define IDC_EDITRES						40033
#define	IDC_OK							40034
#define IDC_CANCEL						40035

#define IDD_CTRL						40036
#define IDC_TOPDOWN						40037
#define IDC_FRONTBACK					40038
#define IDC_LEFTRIGHT					40039
#define IDC_CURVE						40040
#define IDC_SNKPTS						40041
#define IDC_SNKSPL						40042
#define IDC_MESH						40043
#define IDC_WALL						40044
#define IDC_SKEL						40045
#define IDC_ROOTPRV						40046
#define IDC_ROOTNEXT					40047
#define IDC_SEGPRV						40048
#define IDC_SEGNEXT						40049
#define IDC_LEVELUP						40050
#define IDC_LEVELDOWN					40051
#define IDC_START						40052
#define IDC_STOP						40053
#define IDC_AUTO						40054
#define IDC_RESET						40055
#define IDC_STRETCH						40056
#define IDC_STRETCHSPIN					40057
#define IDC_STIFFNESS					40058
#define IDC_STIFFNESSSPIN				40059
#define IDC_SIMULSTRENSTRTCH			40060
#define IDC_ATTRACT						40061
#define IDC_ATTRACTSPIN					40062
#define IDC_REPULSE						40063
#define IDC_REPULSESPIN					40064
#define IDC_PROGRESSMERGE				40065
#define IDC_MERGE						40066
#define IDC_CHANGE						40067
#define IDC_DELETE						40068
#define IDC_LOCK						40069
#define IDC_LOG							40070
#define IDC_UNDO						40071
#define IDC_REDO						40072

#define IDC_PAUSE						40073
#define IDC_HIGHLIGHT					40074
#define IDC_PRIMARY						40075
#define IDC_SEMINAL						40076
#define IDC_LATERAL						40077
#define IDC_SIDEPRUNE					40078
#define ID_FILE_RENDER					40079
#define IDC_APPLY						40080

#define IDR_ACCELERATOR					40081
#define IDA_CTRL_O						40082
#define IDA_CTRL_M						40083
#define IDA_CTRL_S						40084
#define IDA_CTRL_I						40085
#define IDA_CTRL_E						40086
#define IDA_CTRL_T						40087
#define IDA_CTRL_R						40088
#define IDA_CTRL_P						40089
#define IDA_ALT_C						40090
#define IDA_ALT_X						40091
#define IDA_F1							40092

#define IDC_ANALYSE						40093
#define IDC_PROGRESSANALYSE				40094

#define IDS_STATUSBAR					40095

#define ID_FILE_SKELETON				40096
#define ID_FILE_BOX						40097
#define ID_FILE_POLYLINE				40098

#define IDC_SCROLLUP					40099
#define IDC_SCROLLDOWN					40100

#define IDC_STATIC						-1

//User Messages
#define UM_UPDATE						WM_USER
#define IDUM_ANALYSE					WM_USER+1
#define IDUM_HOURGLASS					WM_USER+2
#define IDUM_UPDATESCREEN				WM_USER+3
#define IDUM_REFRESH					WM_USER+4
//#define IDUM_UPDATEREGRESSIONIMG		WM_USER+5
