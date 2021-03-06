// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "AI_API.h"

//External variables.
extern Player vctPlayer[MAX_PLAYERS];
extern  int icAttach;

//Main func.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (icAttach == 0)
		{
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				vctPlayer[i].bUsed = false;
				vctPlayer[i].bThreadKill = false;
				vctPlayer[i].pfnCallBackFunc = NULL;
				vctPlayer[i].hMessageLoopThread = 0;
				vctPlayer[i].hwDLLManagedWindow = 0;
				vctPlayer[i].player_id = -1;	//Unused PlayerID.	//ClearPlayer call??
			}
		}
		icAttach++;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		icAttach--;
		break;
	}
	return TRUE;
}

