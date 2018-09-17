#pragma once
//One process each player!!
#include <Windows.h>
#include <vector>
#include <cassert>
#ifndef __AI_API
#define __AI_API

#ifdef __cplusplus
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT __declspec (dllexport)
#endif

//User`s Message Type.
#define WM_IDLE					(WM_USER + 1)
#define WM_STARTAI			(WM_USER + 2)
#define WM_REPORTTIME	(WM_USER + 3)
#define WM_EXIT					(WM_USER + 4)



//Definition of constants.
#define MAX_GROUPS 16
#define MAX_PLAYERS_EACH_GROUP 4
#define MAX_PLAYERS (MAX_GROUPS * MAX_PLAYERS_EACH_GROUP)

//TypeDefs.
typedef int PLAYER_ID;
typedef BOOL(WINAPI *PCBFUN)(PLAYER_ID);

typedef struct
{
	PLAYER_ID pid;
	UINT uHealth;
	int iData;
	DWORD dwData2;
} PLAYER_STATUS, *PPLAYER_STATUS;

typedef struct
{
	PLAYER_ID pid;
	DWORD dwCommandCode;
	
}COMMAND, *PCOMMAND;

typedef struct
{
	PLAYER_ID pid;
	DWORD dwVisualData;
}VDATA, *PVDATA;


//Player`s APIs 
class Player
{
public:
	volatile bool bUsed;
	volatile bool bThreadKill;

	TCHAR szPlayerName[20];

	HANDLE hPlayerProcess;
	HWND hwDLLManagedWindow;
	HANDLE hMessageLoopThread;
	DWORD dwPid;

	PCBFUN pfnCallBackFunc;
	PLAYER_ID player_id;
	PLAYER_STATUS ps;	//Can be stored because it won`t change during the 50ms.
	VDATA vdata;	//
	//std::vector<COMMAND> vctCommand;
};

EXPORT PLAYER_ID WINAPI RegisterPlayer(const TCHAR* szPlayerName, PCBFUN pfnCallBackFunction);
EXPORT BOOL WINAPI DeRegisterPlayer(PLAYER_ID pid);
EXPORT BOOL WINAPI GetPlayerStatus(PLAYER_ID pid, PPLAYER_STATUS pPS);
EXPORT BOOL WINAPI GetVisualData(PLAYER_ID pid, PVDATA pvData);
EXPORT BOOL WINAPI PostCommand(PLAYER_ID pid, PCOMMAND pCommand);
EXPORT int WINAPI Idle(PLAYER_ID pid);	//Will not return.


//Programmer`s API.
EXPORT BOOL WINAPI RegisterProgrammer(void);
EXPORT Player* WINAPI GetPlayerArrayPointer(void);
EXPORT BOOL WINAPI SetHwndConsole(HWND hwndConsole);
EXPORT void WINAPI ReleaseHwndConsole(void);
EXPORT int WINAPI GetPlayerBlock(DWORD ProcessId);
EXPORT void WINAPI PyCalling(void);







#endif

