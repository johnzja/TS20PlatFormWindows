// AI_API.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "AI_API.h"
//Only concerning registeration.


#pragma data_seg ("shared")
//Indicate that the following memory section is shared among processes.
//Vector structure may store the data on heaps. (May cause problems.) AIConsole application should not read this data segment.
//Needn`t export. Just return pointers of vctPlayer.

Player vctPlayer[MAX_PLAYERS] = { false };	//While detach: Destructor may be called.
std::timed_mutex mtxPlayer;
HWND hwndConsole = 0;
int icAttach = 0;
#pragma data_seg ()
#pragma comment(linker, "/SECTION:shared,RWS")

//Unshared Region.
int  iCurrGroupIndex = -1;

HWND hwDLLManagedWindow = 0;
HANDLE hMessageLoopThread = 0;
LARGE_INTEGER t1, t2, tc;

//Function declaration.
LRESULT WINAPI DllManagedWndProc(HWND, UINT, WPARAM, LPARAM);
int GetGroupProcessID(int iGroupIndex);
void ClearPlayer(Player* player);


//Function bodies.

DWORD WINAPI MessageLoopThreadProc(LPVOID lpParam)
{
	volatile Player* pPlayer = (Player*)lpParam;

	//Establish message loop for CallBack-Calling.
	static TCHAR szHiddenWindowName[30];
	wsprintf(szHiddenWindowName, TEXT("DLLCallBack_%i"), pPlayer->player_id);

	WNDCLASS wndclass;	//Register a window class for message loop.

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = DllManagedWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = (HINSTANCE)pPlayer->hPlayerProcess;
	wndclass.hIcon = NULL;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szHiddenWindowName;

	if (!RegisterClass(&wndclass))	//Class registered.
		return -1;//Fatal error.

	MSG msg;

	QueryPerformanceFrequency(&tc);	//Initialize Timer.

	assert(hwDLLManagedWindow == 0);
	//Create a hidden window with WndProc = DllManagedWndProc. This window is registered to the MAIN THREAD!!!
	if ((pPlayer->hwDLLManagedWindow = CreateWindow(szHiddenWindowName, TEXT(""),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, wndclass.hInstance, NULL)) == 0)
		return -1;

	hwDLLManagedWindow = pPlayer->hwDLLManagedWindow;
	pPlayer->bThreadKill = false;	//Indicate that this thread has been successfully started.

	
	while (GetMessage(&msg, hwDLLManagedWindow, 0, 0))	//Message loop. Similar as .exec() in Qt.
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	//std::cout << "Message loop thread exit." << std::endl;
	//HERE:Deregister the player and exit the whole program!	//However you can`t call Deregister player yourself.
	//Clear all playerinfo about this process.
	DWORD dwCurrProcessPid = GetCurrentProcessId();
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (vctPlayer[i].dwPid == dwCurrProcessPid)
		{
			ClearPlayer(vctPlayer + i);
		}
	}
	exit(1);	//Exit the whole process.
	return 0;
}

LRESULT WINAPI DllManagedWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)	//Executing thread = MessageLoopThread
{
	switch (message)
	{
	case WM_CREATE:	

		return 0;

	case WM_STARTAI:
		QueryPerformanceCounter(&t1);	//Start timing.
		//4 threads dispatched here.??!!
		for (int i = iCurrGroupIndex * MAX_PLAYERS_EACH_GROUP; i < (iCurrGroupIndex + 1)*MAX_PLAYERS_EACH_GROUP; i++)
		{
			if (vctPlayer[i].bUsed)
			{
				(*vctPlayer[i].pfnCallBackFunc)((PLAYER_ID)i);	//Call the callback func.
			}
		}
		QueryPerformanceCounter(&t2);	//Stop timing.

		if (hwndConsole)PostMessage(hwndConsole, WM_REPORTTIME,
			(int)(float(t2.QuadPart - t1.QuadPart) * float(1000000) / tc.QuadPart),
			(LPARAM)GetGroupProcessID(iCurrGroupIndex));	//in us.
		return 0;
		// user`s message: call the callback function.(pfn). Parameter: playerID.
	case WM_EXIT:
		if (GetCurrentProcessId() == (DWORD)wParam)
		{
			PostQuitMessage(0);	//Thread exit...
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

int GetGroupProcessID(int iGroupIndex)
{
	if (iGroupIndex < 0 || iGroupIndex >= MAX_GROUPS)return -1;	//Index failure.
	for (int i = iGroupIndex * MAX_PLAYERS_EACH_GROUP; i < ((iGroupIndex + 1)*MAX_PLAYERS_EACH_GROUP); i++)
	{
		if (vctPlayer[i].bUsed)return vctPlayer[i].dwPid;	//return the pid of the process who had occupied the group block.
	}
	//This group has not been occupied.
	return -2;	
}

//Found: return index of block. //Not found:return -1.
EXPORT int WINAPI GetPlayerBlock(DWORD ProcessId)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (vctPlayer[i].dwPid == ProcessId)return (i / MAX_PLAYERS_EACH_GROUP);
	}
	return -1;
}

//if not found, return index to a new player position in a  blank group. (if all groups are occupied, return MAX_PLAYERS)...
//if found and the group is not full ,return the availible player index. if the group is full, return -1. 
int FindGroupByProcessID(DWORD dwPid)	
{
	for (int i = 0; i < MAX_GROUPS; i++)
	{
		int iResult = GetGroupProcessID(i);
		if (iResult == dwPid)
		{
			for (int j = i * MAX_PLAYERS_EACH_GROUP; j < (i + 1)*MAX_PLAYERS_EACH_GROUP; j++)
			{
				if (!vctPlayer[j].bUsed)
				{
					return j;
				}
			}
			return -1;
		}
		else if (iResult == -2)
		{
			return i * MAX_PLAYERS_EACH_GROUP;
		}
	}
	return MAX_PLAYERS;
}

EXPORT PLAYER_ID WINAPI RegisterPlayer(const TCHAR* szPlayerName, PCBFUN pfnCallBackFunction)
{
	if (!mtxPlayer.try_lock_for(std::chrono::microseconds(500)))return -1;
	//Allocate a PlayerID.
	PLAYER_ID player_id;
	DWORD dwCurrProcId = GetCurrentProcessId();
	int iResult = FindGroupByProcessID(dwCurrProcId);
	if (iResult == MAX_PLAYERS || iResult == -1)return -1;
	player_id = iResult;	//This player_id must be valid.

	if (iCurrGroupIndex == -1)iCurrGroupIndex = player_id / MAX_PLAYERS_EACH_GROUP;		//Initialize only once.

	//Initialize player structure.
	vctPlayer[player_id].pfnCallBackFunc = pfnCallBackFunction;
	Player* pCurrPlayer = vctPlayer + player_id;
	vctPlayer[player_id].hPlayerProcess = GetModuleHandle(NULL);
	vctPlayer[player_id].dwPid = dwCurrProcId;
	vctPlayer[player_id].player_id = player_id;
	vctPlayer[player_id].bThreadKill = true;	//Disable the message thread.

	if (!hMessageLoopThread)
	{
		if (!(vctPlayer[player_id].hMessageLoopThread = CreateThread(NULL, 0, MessageLoopThreadProc, &vctPlayer[player_id], 0, NULL)))
			return -1;	//Create a MessageLoopThread to execute commands from AIConsole and execute CallBackFuncs indirectly.
		while (pCurrPlayer->bThreadKill);	//Wait until ThreadProc is executed.
		hMessageLoopThread = vctPlayer[player_id].hMessageLoopThread;
	}
	else
	{
		vctPlayer[player_id].bThreadKill = false;
		vctPlayer[player_id].hMessageLoopThread = hMessageLoopThread;
		vctPlayer[player_id].hwDLLManagedWindow = hwDLLManagedWindow;
		//Copy some data.
	}
	lstrcpy(vctPlayer[player_id].szPlayerName, szPlayerName);
	vctPlayer[player_id].bUsed = true;
	mtxPlayer.unlock();
	return player_id;
}

bool CheckPid(PLAYER_ID pid)
{
	if (pid < 0 || pid >= MAX_PLAYERS || vctPlayer[pid].bUsed == false
		|| vctPlayer[pid].dwPid != GetCurrentProcessId())	//Check for cheating : you can`t use the module handle.
		return false;
	return true;
}

void ClearPlayer(Player* player)
{
	player->bThreadKill = false;
	player->bUsed = false;
	player->player_id = -1;	//Unused.
	//player->vctCommand.clear();	//Clear command vector.
	player->hMessageLoopThread = 0;
	player->hwDLLManagedWindow = 0;
	player->pfnCallBackFunc = 0;
	player->hPlayerProcess = 0;
	player->dwPid = 0;
	//Clear VData
	//Clear PlayerStatus.
	return;
}

EXPORT BOOL WINAPI DeRegisterPlayer(PLAYER_ID pid)	//Current Thread = MainThread.
{
	if (!CheckPid(pid))return FALSE;

	BOOL bReturnValue = TRUE;
	if (!mtxPlayer.try_lock_for(std::chrono::microseconds(500)))return -1;
	ClearPlayer(vctPlayer + pid);
	mtxPlayer.unlock();
	//pCurrPlayer = NULL;
	return bReturnValue;
}

EXPORT BOOL WINAPI GetPlayerStatus(PLAYER_ID pid, PPLAYER_STATUS pPS)
{
	if (!CheckPid(pid))return FALSE;
	*pPS = vctPlayer[pid].ps;
	return TRUE;
}

EXPORT BOOL WINAPI GetVisualData(PLAYER_ID pid, PVDATA pvData)
{
	if (!CheckPid(pid))return FALSE;
	*pvData = vctPlayer[pid].vdata;
	return TRUE;

}

EXPORT BOOL WINAPI PostCommand(PLAYER_ID pid, PCOMMAND pCommand)
{
	if (!CheckPid(pid))return FALSE;
	///Check for command grammar.

	//vctPlayer[pid].vctCommand.push_back(*pCommand); Never Use vectors!!
	///Check for vector overflow.
	return TRUE;
}

EXPORT int WINAPI Idle(PLAYER_ID pid)
{
	if (!CheckPid(pid))
	{
		//Cheating!
		return 0;	//Exit the program immediately.
	}

	//Here: pid must be valid.
	//Enter the message loop (in Main Thread).
	MSG msg;
	while (GetMessage(&msg, hwDLLManagedWindow, 0, 0))	//Message loop. Similar as .exec() in Qt.
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}


//Programmer`s API.
EXPORT BOOL WINAPI RegisterProgrammer(void)
{
	return TRUE;
}

EXPORT Player* WINAPI GetPlayerArrayPointer(void)
{
	return vctPlayer;
}

EXPORT BOOL WINAPI SetHwndConsole(HWND hwndConsole)
{
	if (::hwndConsole == 0)
	{
		::hwndConsole = hwndConsole;
		return TRUE;
	}
	else
		return FALSE;
}

EXPORT void WINAPI ReleaseHwndConsole(void)
{
	hwndConsole = 0;
}


//Connecting with logic group.
EXPORT void WINAPI PyCalling(void)
{
	//Further code required.
	return;
}




