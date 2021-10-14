#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include <SMObject.h>
#include <smstructs.h>

using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

int main()
{
	SMObject PMObj(TEXT("ProcessManagement"), sizeof(ProcessManagement));
	//SM Creation and Seeking access

	//Declaration
	double TimeStamp;
	__int64 Frequency, Counter;
	int Shutdown = 0x00;

	QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);
	PMObj.SMCreate();
	PMObj.SMAccess();
	ProcessManagement* PMData = (ProcessManagement*)PMObj.pData;

	while (!PMData->Ready)
	{
		Thread::Sleep(25);
	}

	while (1)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&Counter);
		TimeStamp = (double)Counter / (double)Frequency * 1000; //ms

		// Did PM put my flag down?
		if (PMData->Heartbeat.Flags.Display == 0)
		{
			// True -> Put my flag up
			PMData->Heartbeat.Flags.Display = 1;
		}
		else
		{
			// False -> If the PM time stamp older agreed time gap
			if ((TimeStamp - PMData->PMTimeStamp) > 250)
			{
				// True -> Shutdown all
				PMData->Shutdown.Status = 0xFF;
			}
		}

		Console::WriteLine("Display time stamp : {0,12:F3} {1,12:X2}", TimeStamp, Shutdown);
		Thread::Sleep(25);
		if (PMData->Shutdown.Status)
			break;
		if (_kbhit())
			break;
	}


	return 0;
}