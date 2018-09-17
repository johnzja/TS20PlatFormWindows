//Requirement: Must include DLL`s header file.	(The following AI_API.h)

#include <windows.h>
#include <iostream>
#include "..\\AI_API\AI_API.h"	//Must include this file to import APIs.
using namespace std;
BOOL WINAPI CallBackFunc(PLAYER_ID);

#define UNICODE
#define _UNICODE

int main(void)
{
	PLAYER_ID player_id[5];

	for (int i = 0; i < 5; i++)
	{
		player_id[i] = RegisterPlayer(TEXT("EESAST"), CallBackFunc);
	}


	//CallBackFunc will be called every 50ms.

	//Your initializing code here!

	//Test.
	
	cout << "Current Process ID:\t" << GetCurrentProcessId() << endl;
	for (int i = 0; i < 5; i++)
		cout << player_id[i] << endl;


	
	int iExitCode = Idle(player_id[0]);	//	  To keep your program running.  If you want to exit the game:\
															call DeRegisterPlayer(player_id) to deregister all your players,\
															and then execute "return 0" in main. 
	
	return iExitCode;
}

BOOL WINAPI CallBackFunc(PLAYER_ID player_id)
{
	PLAYER_STATUS ps;
	VDATA vd;
	COMMAND cmd;

	GetVisualData(player_id, &vd);
	GetPlayerStatus(player_id, &ps);

	//Your AI Algorithm code here!

	// a longtime task.
	for (int j = 0; j < 10, 000; j++)
		for (int i = 0; i < 10, 000, 000; i++)
		{
			i -= 1;
			i += 1;
		}
	cout << "Callback called successfully! Player ID = " << player_id << endl;

	//PostCommand(player_id, &cmd);	//This may cause problems.
	return TRUE;
}

