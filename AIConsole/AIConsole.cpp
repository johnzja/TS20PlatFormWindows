#define UNICODE
#define _UNICODE

#define ID_LISTVIEW 1
#define ID_ADDBUTTON 2
#define ID_STOPBUTTON 3
#define ID_RUNAIBUTTON 4
#define ID_DELETEBUTTON 5
#define ID_AUTOCHECKBOX 6

//Status codes
#define PROCESS_RUNNING 0
#define PROCESS_SUSPENDED 1
#define PROCESS_RUNNING_DETACHED 2
#define PROCESS_RUNNING_ATTACHED 3
#define PROCESS_RUNNING_AI 4
#define PROCESS_TERMINATED 5

//User`s message.
#define WAIT_TIME_MS 50


#include <stdlib.h>
#include <Windows.h>
#include "..\\AI_API\AI_API.h"
#include <CommCtrl.h>
#include <commdlg.h>
#include <vector>
#include <mutex>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' \
name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' processorArchitecture='x86' \
publicKeyToken='6595b64144ccf1df' language='*'\"")


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND GetPlayerAIHwnd(DWORD dwPid);
void ResizeListView(HWND hwndListView);


//Player Info.
const Player* vctPlayer = nullptr;

//Here: Process info storage.
typedef struct
{
	TCHAR szExePath[MAX_PATH];
	HANDLE hProcess;		
	DWORD ProcessId;	//Using ProcessId instead of ProcessHandle to specify a process.
	volatile int iTimeElapsed_us;
	volatile int iStatus;
	volatile int cRun;
	PLAYER_ID player_id[MAX_PLAYERS_EACH_GROUP];
}PROCESSINFO, *PPROCESSINFO;

typedef struct tagSelect
{
	DWORD dwPid;
	volatile bool bSelected;
}SELECT;

void InitializePrcInfo(PPROCESSINFO pProcInfo)
{
	pProcInfo->szExePath[0] = TEXT('\0');
	pProcInfo->hProcess = 0;
	pProcInfo->ProcessId = -1;
	pProcInfo->iTimeElapsed_us = -1;
	pProcInfo->iStatus = PROCESS_TERMINATED;
	pProcInfo->cRun = 0;
	for (int i = 0; i < MAX_PLAYERS_EACH_GROUP; i++)
	{
		(pProcInfo->player_id)[i] = -1;
	}
}

//Global Variables
const TCHAR g_szAppName[] = TEXT("AI Console");
int cxChar, cyChar;

HINSTANCE hInstance;
HBRUSH hbrBackGround;
HWND hwnd;
TCHAR szBuffer[300];

std::vector<PROCESSINFO> vctProcInfo;	//Process info stored here.


#ifdef _DEBUG
HANDLE hIn, hOut;
void Out(const TCHAR* pszData)
{
	WriteConsole(hOut, szBuffer, wsprintf(szBuffer, TEXT("%s\n"), pszData), NULL, NULL);
	return;
}

void Out(const int& iData)
{
	WriteConsole(hOut, szBuffer, wsprintf(szBuffer, TEXT("%i\n"), iData), NULL, NULL);
	return;
}

#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG msg;
	WNDCLASS wndclass;

	::hInstance = hInstance;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = 0;	//Use default application icon.
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);

	DWORD dwColor = GetSysColor(COLOR_BTNFACE);
	hbrBackGround = CreateSolidBrush(dwColor);

	wndclass.hbrBackground = hbrBackGround;	//Use default window color.
	wndclass.lpszMenuName = 0;
	wndclass.lpszClassName = g_szAppName;
										//10 variables. All variables need to be  initialized.
	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"), g_szAppName, MB_ICONERROR);
		return 0;
	}

	//Using console as debug output.
#ifdef _DEBUG
	AllocConsole();
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hIn = GetStdHandle(STD_INPUT_HANDLE);
	//WriteConsole(hOut, szBuffer, wsprintf(szBuffer, TEXT("%s"), TEXT("Fck")), NULL, NULL);
#endif
	hwnd = CreateWindow(g_szAppName, g_szAppName,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	if (!SetHwndConsole(hwnd))		//Calling AI_API setting functions.
	{
		MessageBox(hwnd, TEXT("Program has already been running!"), TEXT("Error"), MB_OK);
		return 0;
	}
	

	if (hwnd == 0)
	{
		MessageBox(NULL, TEXT("Handle Error! Window creation failure."), TEXT("Error"), MB_OK);
		return 0;
	}
	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	//Connect with AI_API.dll.
	vctPlayer = GetPlayerArrayPointer();

	while (GetMessage(&msg, NULL, 0, 0))	//Message loop. Similar as .exec() in Qt.
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	DeleteObject(hbrBackGround);
	return msg.wParam;
}

TCHAR* StatusCodeToState(int iStatusCode)
{
	static TCHAR szStates[6][20] = {
		TEXT("running"),TEXT("suspended"), TEXT("detached"),TEXT("attached"),TEXT("AI running"),TEXT("terminated")
	};
	switch (iStatusCode)
	{
	case PROCESS_RUNNING:
	case PROCESS_SUSPENDED:
	case PROCESS_RUNNING_DETACHED:
	case PROCESS_RUNNING_ATTACHED:
	case PROCESS_RUNNING_AI:
	case PROCESS_TERMINATED:
		return szStates[iStatusCode];
	default:
		return (TCHAR*)TEXT("error");
	}
}

//Initialize controls.

HWND InitListView(HWND hwndParentWindow)
{
	DWORD dwStyle = //WS_TABSTOP |
		WS_CHILD |
		WS_BORDER |
		WS_VISIBLE |
		LVS_AUTOARRANGE |
		LVS_REPORT |
		LVS_SHOWSELALWAYS;

	HWND hwndList = CreateWindow(WC_LISTVIEW, TEXT(""),
		dwStyle,
		0, 0,
		0, 0,	//Not properly defined.
		hwndParentWindow, (HMENU)ID_LISTVIEW,
		hInstance, NULL);
		
	if (!hwndList)
	{
		//Error Creating Control Window!
#ifdef _DEBUG
		OutputDebugString(TEXT("Error Creating ListView!"));
#endif
		return 0;
	}

	//Initialize ListView.
	ResizeListView(hwndList);
	LV_COLUMN lvColumn;
	TCHAR szCaptions[6][20] = { TEXT("进程名"),
	TEXT("进程ID"),TEXT("玩家ID"),TEXT("状态") ,TEXT("AI耗时") ,TEXT("AI运行次数") };

	//InsertColumns.

	ListView_DeleteAllItems(hwndList);	//Clear!
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_RIGHT;
	lvColumn.cx = 100;

	for (int i = 0; i < 6; i++)
	{
		lvColumn.pszText = szCaptions[i];
		ListView_InsertColumn(hwndList, i, &lvColumn);
	}

	return hwndList;
}

HWND InitAddButton(HWND hwndParentWindow)
{
	HWND hwndButton = CreateWindow(
		TEXT("button"), TEXT("Add EXE"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		cxChar, cyChar / 2, 15 * cxChar, 7 * cyChar / 4,
		hwndParentWindow, (HMENU)ID_ADDBUTTON,
		hInstance, NULL);

	return hwndButton;
}

HWND InitStartButton(HWND hwndParentWindow)
{
	return 0;
}

HWND InitStopButton(HWND hwndParentWindow)
{
	HWND hwndButton = CreateWindow(
		TEXT("button"), TEXT("Stop Process"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		20 * cxChar, cyChar / 2, 15 * cxChar, 7 * cyChar / 4,
		hwndParentWindow, (HMENU)ID_STOPBUTTON,
		hInstance, NULL);

	return hwndButton;
}

HWND InitRunAIButton(HWND hwndParentWindow)
{
	HWND hwndButton = CreateWindow(
		TEXT("button"), TEXT("Run AI"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		40 * cxChar, cyChar / 2, 15 * cxChar, 7 * cyChar / 4,
		hwndParentWindow, (HMENU)ID_RUNAIBUTTON,
		hInstance, NULL);

	return hwndButton;
}

HWND InitDeleteButton(HWND hwndParentWindow)
{
	HWND hwndButton = CreateWindow(
		TEXT("button"), TEXT("Delete Process"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		60 * cxChar, cyChar / 2, 15 * cxChar, 7 * cyChar / 4,
		hwndParentWindow, (HMENU)ID_DELETEBUTTON,
		hInstance, NULL);

	return hwndButton;
}

HWND InitAutoCheckbox(HWND hwndParentWindow)
{
	HWND hwndCheckbox = CreateWindow(TEXT("button"),
		TEXT("AutoRun"),
		WS_CHILD | BS_AUTOCHECKBOX | WS_VISIBLE,
		80 * cxChar, cyChar / 2, 15 * cxChar, 7 * cyChar / 4,
		hwndParentWindow, (HMENU)ID_AUTOCHECKBOX, hInstance, NULL);

	return hwndCheckbox;
}

void ResizeListView(HWND hwndListView)
{
	RECT rc;
	GetClientRect(GetParent(hwndListView), &rc);

	int yTop = 4 * cyChar;
	MoveWindow(hwndListView, 0, yTop, rc.right - rc.left, rc.bottom - rc.top - yTop, TRUE);
}

void UpdateProcessVector(void)
{
	DWORD dwExitCode;
	//Check process status.
	for (PROCESSINFO& pi : vctProcInfo)
	{
		if (!GetExitCodeProcess(pi.hProcess, &dwExitCode))
		{
#ifdef _DEBUG
			OutputDebugString(TEXT("进程权限不够！"));
#endif
		}
		if (dwExitCode != STILL_ACTIVE)
		{
			pi.iStatus = PROCESS_TERMINATED;
		}
		else
		{
			pi.iStatus = PROCESS_RUNNING_DETACHED;
			//Find player block according to pid given.
			int iResult = GetPlayerBlock(pi.ProcessId);
			if (iResult != -1)
			{
				//Block found. Copy player_id data to PROCESSINFO structure.
				for (int i = 0; i < MAX_PLAYERS_EACH_GROUP; i++)
				{
					int index = iResult * MAX_PLAYERS_EACH_GROUP + i;
					if (vctPlayer[index].bUsed)
						pi.player_id[i] = vctPlayer[index].player_id;
					else
						pi.player_id[i] = -1;
					
				}
				pi.iStatus = PROCESS_RUNNING_ATTACHED;
			}
			
		}
	}

}


//About Getting Selection info.

SELECT* GetSelectionInfo(HWND hwndListView, int* cSelected)
{
	(*cSelected) = ListView_GetItemCount(hwndListView);
	SELECT* pSelect = nullptr;

	if (*cSelected != 0)
	{
		pSelect = new SELECT[*cSelected];
		for (int i = 0; i < *cSelected; i++)
		{
			ListView_GetItemText(hwndListView, i, 1, szBuffer, sizeof(szBuffer));	//Get  process pid.
			swscanf_s(szBuffer, TEXT("%d"), &pSelect[i].dwPid);	//Ignore "0x".
			pSelect[i].bSelected = (bool)ListView_GetItemState(hwndListView, i, LVIS_SELECTED);
		}
	}
	return pSelect;
}

void UpdateListView(HWND hwndListView, bool DeletionFlag = false)	//Must be called after calling UpdateProcessVector.
{
	//Check if there is selected item.
	//Identify each item by process pid.
	int icItem;

	SELECT* pSelect = nullptr;
	if (!DeletionFlag)
		pSelect = GetSelectionInfo(hwndListView, &icItem);	//If there is item deleted, GetSelection info will cause fetching data from an unexisting vector item.

	ListView_DeleteAllItems(hwndListView);	//Unefficient update.
	LVITEM lvItem;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	lvItem.mask = LVIF_TEXT | LVIF_STATE;
	lvItem.stateMask = 0;
	lvItem.iSubItem = 0;
	lvItem.state = 0;

	int i = 0;
	for (PROCESSINFO& pi : vctProcInfo)
	{
		lvItem.iItem = i++;
		if (ListView_InsertItem(hwndListView, &lvItem) == -1)
		{
#ifdef _DEBUG
			OutputDebugString(TEXT("Error Inserting item!"));
#endif
			return;
		}
	}

	if (pSelect != nullptr)
	{
		for (int j = 0; j < i; j++)	//i: number of items in vct.
		{
			for (int k = 0; k < icItem; k++)	//icItem: previous number.
			{
				if ((vctProcInfo[j].ProcessId == pSelect[k].dwPid) && pSelect[k].bSelected)
				{
					ListView_SetItemState(hwndListView, j, LVIS_SELECTED, LVIS_SELECTED);
					break;
				}
			}
		}
	}
	delete[] pSelect;
}

TCHAR* OpenFile(void)	//May cause problems.
{
	static OPENFILENAME ofn;      // 公共对话框结构。   
	static TCHAR szFile[MAX_PATH]; // 保存获取文件名称的缓冲区。           
	_wgetcwd(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
	// 初始化选择文件对话框。   
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = TEXT("All(*.*)\0*.*\0Executables(*.exe)\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = (LPWSTR)TEXT("Open an executive file.");
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = szBuffer;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// 显示打开选择文件对话框。   
	if (GetOpenFileName(&ofn))
	{
		return szFile;
	}
	return nullptr;
}

//Critical code.
TCHAR* PrintPlayerID(PROCESSINFO& pi)
{
	//Operate szBuffer.
	TCHAR szBuffer1[20];
	szBuffer[0] = TEXT('\0');
	for (int i = 0; i < MAX_PLAYERS_EACH_GROUP; i++)
	{
		if (pi.player_id[i] != -1)
		{
			wsprintf(szBuffer1, TEXT("%d,"), pi.player_id[i]);
			lstrcat(szBuffer, szBuffer1);
		}
	}
	int iLength = 0;
	if ((iLength = lstrlen(szBuffer)) != 0)szBuffer[iLength - 1] = TEXT('\0');
	return szBuffer;
}

void HandleWM_NOTIFY(LPARAM lParam)
{
	NMLVDISPINFO* plvdi;
	TCHAR* pStr;
	int iLength;

	switch (((LPNMHDR)lParam)->code)
	{
	case LVN_GETDISPINFO:
		plvdi = (NMLVDISPINFO*)lParam;

		switch (plvdi->item.iSubItem)
		{
		case 0:	//Process name.
			iLength = lstrlen(vctProcInfo[plvdi->item.iItem].szExePath);
			pStr = vctProcInfo[plvdi->item.iItem].szExePath + iLength;	//Point to '\0'
			while (*pStr != TEXT('\\'))pStr--;	//Find the last '\'.
			pStr++;
			plvdi->item.pszText = (LPWSTR)pStr;
			break;

		case 1:	//Process ID.
			wsprintf(szBuffer, TEXT("%d"), vctProcInfo[plvdi->item.iItem].ProcessId);
			plvdi->item.pszText = szBuffer;
			break;

		case 2:	//Player ID.
			plvdi->item.pszText = PrintPlayerID(vctProcInfo[plvdi->item.iItem]);
			break;

		case 3:
			plvdi->item.pszText = StatusCodeToState(vctProcInfo[plvdi->item.iItem].iStatus);//Display process status.
			break;

		case 4:	//Time elapsed.
			if (vctProcInfo[plvdi->item.iItem].iTimeElapsed_us == -1)
				lstrcpy(szBuffer, TEXT("nodata"));
			else
				wsprintf(szBuffer, TEXT("%d us"), vctProcInfo[plvdi->item.iItem].iTimeElapsed_us);
			plvdi->item.pszText = szBuffer;
			break;

		case 5:	//AI running counter.
			wsprintf(szBuffer, TEXT("%d"), vctProcInfo[plvdi->item.iItem].cRun);
			plvdi->item.pszText = szBuffer;
			break;

		default:
			break;
		}
		break;
	}
	return;
}

//WCV
void HandleAddButtonClicked()	
{
	TCHAR* szFilePath = OpenFile();
	int iLength;
	if ((iLength = lstrlen(szFilePath)) == 0)
		return;
	else
	{
		PROCESSINFO pi;
		InitializePrcInfo(&pi);

		lstrcpy(pi.szExePath, szFilePath);

		//CreateProcess.
		PROCESS_INFORMATION piInfo;
		STARTUPINFO stStartUpInfo;
		memset(&stStartUpInfo, 0, sizeof(stStartUpInfo));
		memset(&piInfo, 0, sizeof(piInfo));	//Initialize two info structures.
		
		BOOL bCreationStatus = CreateProcess(szFilePath, NULL, NULL,
			NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &stStartUpInfo, &piInfo);
		if (bCreationStatus)
		{
			pi.hProcess = piInfo.hProcess;	//Used for terminating process.
			pi.ProcessId = piInfo.dwProcessId;
			pi.iStatus = PROCESS_RUNNING;
			//pi.iTimeElapsed_us = -1;	//-1 means undefined.	//Initial value is -1.
			//pi.player_id = -1;	//-1 means undefined.
			vctProcInfo.push_back(pi);
		}
		else
		{
			//Errors happened.
			MessageBox(hwnd, TEXT("Error! Cannot start this process."),
				TEXT("Error"), MB_OK);
		}
	}
}

void HandleStopButtonClicked(HWND hwndListView)
{
	//Suppose that index in Listview is the same as index of vctProcInfo.
	//You should first send a stop message, then stop the process.
	int cItemCnt = 0;
	SELECT* pSelect = GetSelectionInfo(hwndListView, &cItemCnt);
	if (cItemCnt == 0)return;

	//Posting Stop messages.
	for (int i = 0; i < cItemCnt; i++)
	{
		HWND hwndPlayerAI = 0;
		if (pSelect[i].bSelected && (hwndPlayerAI = GetPlayerAIHwnd(pSelect[i].dwPid)))
		{
			PostMessage(hwndPlayerAI, WM_EXIT, (WPARAM)pSelect[i].dwPid, 0);
		}
	}

	UpdateProcessVector();
	//UpdateListView(hwndListView);
	delete[] pSelect;
}

HWND GetPlayerAIHwnd(DWORD dwPid)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (vctPlayer[i].dwPid == dwPid)return vctPlayer[i].hwDLLManagedWindow;
	}
	return 0;
}

void HandleDeleteProcessButtonClicked(HWND hwndListView)
{
	int cItemCnt = 0;
	SELECT* pSelect = GetSelectionInfo(hwndListView, &cItemCnt);
	auto it = vctProcInfo.begin();
	for (int i = 0; i < cItemCnt; i++)
	{
		if (pSelect[i].bSelected && ((it->iStatus == PROCESS_TERMINATED) || (it->iStatus == PROCESS_RUNNING_DETACHED)))
		{
			it = vctProcInfo.erase(it);
		}
		else
		{
			it++;
		}
	}
	delete[] pSelect;
}


//AutoRun: Required functions.
struct hwndStruct
{
	volatile HWND hwnd;
	volatile DWORD ProcessId;
	volatile bool bSelected;
};

volatile bool bStatus_Auto = false;
volatile int iCycleCnt = -1;
volatile bool it_valid = false;
volatile size_t i = -1;
volatile HANDLE hEvent = 0;

std::vector<hwndStruct> vctSelect;
//End of volatile variables definition.


DWORD WINAPI ThreadProc(LPVOID lParam)
{
	std::vector<hwndStruct>::iterator vct_it;
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	//Out((int)hEvent);
	iCycleCnt = 0;
	while (bStatus_Auto && vctSelect.size()!=0)
	{
		//keep this vector valid.
		size_t L = vctSelect.size();
		for (i = 0; i < L; i++)
		{
			ResetEvent(hEvent);
			it_valid = true;
			SendMessage(vctSelect[i].hwnd, WM_STARTAI, 0, 0);
			WaitForSingleObject(hEvent, WAIT_TIME_MS + 1);
			if (!it_valid)vctSelect[i].bSelected = true;
			it_valid = false;	//Using mutex here may cause deadlock.
			ResetEvent(hEvent);
		}
		
		for (vct_it = vctSelect.begin(); vct_it != vctSelect.end();)
		{
			if (!vct_it->bSelected)
			{
				wsprintf(szBuffer, TEXT("Process %d has failed."), vct_it->ProcessId);
				MessageBox(hwnd, szBuffer, TEXT("Info"), MB_OK);
				vct_it = vctSelect.erase(vct_it);
			}
			else vct_it++;
		}

		PyCalling();	//Update game info.
		iCycleCnt++;
	}
	DeleteObject(hEvent);
	hEvent = 0;
	iCycleCnt = -1;
	return 2;
}

//WCV
void HandleRunAIButtonClicked(HWND hwndListView)
{
	static HANDLE hThread;
	//Status : auto-running.
	//Two different modes.
	if (bStatus_Auto)//Auto running.
	{
		SetEvent(hEvent);
		bStatus_Auto = false;	//Tell the thread to suicide.
		WaitForSingleObject(hThread, INFINITE);

		hThread = 0;
		hEvent = 0;
		vctSelect.clear();
		iCycleCnt = -1;
		//Stop auto mode
		EnableWindow(GetDlgItem(hwnd, ID_AUTOCHECKBOX), TRUE);
		SetWindowText(GetDlgItem(hwnd, ID_RUNAIBUTTON), TEXT("Run AI"));
	}
	else
	{
		bool bAuto = SendMessage(GetDlgItem(hwnd, ID_AUTOCHECKBOX), BM_GETCHECK, 0, 0);
		if (!bAuto)
		{
			int icItem;
			SELECT* pSelect = GetSelectionInfo(hwndListView, &icItem);
			for (int i = 0; i < icItem; i++)
			{
				HWND hwndPlayerAI = 0;
				if (pSelect[i].bSelected && (hwndPlayerAI = GetPlayerAIHwnd(pSelect[i].dwPid)))
				{
					//Registered.
					PostMessage(hwndPlayerAI, WM_STARTAI, 0, 0);
					vctProcInfo[i].iStatus = PROCESS_RUNNING_AI;
				}
			}
			delete[] pSelect;
			return;
		}
		else
		{
			bStatus_Auto = true;
			SetWindowText(GetDlgItem(hwnd, ID_RUNAIBUTTON), TEXT("Stop AutoRun"));
			EnableWindow(GetDlgItem(hwnd, ID_AUTOCHECKBOX), FALSE);
			//Auto mode.
			vctSelect.clear();
			int cItem = 0;
			SELECT* pSelect = GetSelectionInfo(hwndListView, &cItem);
			if (cItem == 0)return;

			for (int i = 0; i < cItem; i++)
			{
				HWND hwndPlayerAI = 0;
				if (pSelect[i].bSelected && (hwndPlayerAI = GetPlayerAIHwnd(pSelect[i].dwPid)))
				{
					hwndStruct temp;
					temp.hwnd = hwndPlayerAI;
					temp.ProcessId = pSelect[i].dwPid;
					temp.bSelected = false;
					vctSelect.push_back(temp);
				}
			}
			delete[] pSelect;

			//Start the thread. The thread handles everything.
			DWORD dwThreadId;
			hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &dwThreadId);
			while (iCycleCnt == -1);	//Wait until the thread is started.
			//1. create a thread. Redirect reporttime messages.
			//2. ..when all ai`s finished:refresh ListView.
			//3. if after 50ms`s wait, there still are ai`s running, then do not run it in the next cycle. && report this error in ListView.
		}
	}
}

//WCV
void HandleReportTime(WPARAM wParam, LPARAM lParam)
{
	//wParam: time in us. lParam: pid.	
	if (bStatus_Auto && (hEvent != 0))
	{
		if (it_valid)it_valid = false;	//Using it_valid to convey messages.
		SetEvent(hEvent);
	}
	for (PROCESSINFO& pi : vctProcInfo)
	{
		if (pi.ProcessId == (DWORD)lParam)
		{
			pi.iTimeElapsed_us = (int)wParam;
			pi.iStatus = PROCESS_RUNNING_ATTACHED;
			pi.cRun++;
			return;
		}
	}
	return;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
	WPARAM wParam, LPARAM lParam)
{
	//Handle to child-windows.
	static HWND hwndListView, hwndAddButton, hwndStopButton, hwndAIButton, hwndDeleteButton, hwndCheckbox;
	
	//Basic measurements.
	HDC hdc;
	PAINTSTRUCT ps;

	//About initializing.
	INITCOMMONCONTROLSEX icex;

	switch (message)
	{
	case WM_CREATE:
		cxChar = LOWORD(GetDialogBaseUnits());
		cyChar = HIWORD(GetDialogBaseUnits());

		icex.dwICC = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&icex);

		hwndListView = InitListView(hwnd);
		hwndAddButton = InitAddButton(hwnd);
		hwndStopButton = InitStopButton(hwnd);
		hwndAIButton = InitRunAIButton(hwnd);
		hwndDeleteButton = InitDeleteButton(hwnd);
		hwndCheckbox = InitAutoCheckbox(hwnd);

		SetTimer(hwnd, 1, 700, NULL);
		return 0;

	case WM_NOTIFY:
		HandleWM_NOTIFY(lParam);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_ADDBUTTON:
			HandleAddButtonClicked();
			UpdateListView(hwndListView);
			break;
			
		case ID_STOPBUTTON:
			HandleStopButtonClicked(hwndListView);
			UpdateListView(hwndListView);
			break;

		case ID_RUNAIBUTTON:
			HandleRunAIButtonClicked(hwndListView);
			UpdateListView(hwndListView);
			break;

		case ID_DELETEBUTTON:
			HandleDeleteProcessButtonClicked(hwndListView);
			UpdateListView(hwndListView, true);
			break;

		}

		return 0;

	case WM_SIZE:
		ResizeListView(hwndListView);
		return 0;

	case WM_TIMER:
		UpdateProcessVector();
		UpdateListView(hwndListView);
		return 0;

	case WM_REPORTTIME:
		HandleReportTime(wParam, lParam);
		UpdateListView(hwndListView);
		return 0;


	case WM_DESTROY:
#ifdef _DEBUG
		FreeConsole();
#endif
		KillTimer(hwnd, 1);
		ReleaseHwndConsole();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}





