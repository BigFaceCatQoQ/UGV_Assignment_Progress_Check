#using <System.dll>

#include <conio.h>

#include <SMObject.h>
#include <smstructs.h>


using namespace System;
using namespace System::Diagnostics;
using namespace System::Threading;

value struct UGVProcesses
{
	String^ ModuleName;
	int Critical;
	int CrashCount;
	int CrashCountLimit;
	Process^ ProcessName;
};

int main()
{
	// Tele-operation
	//Declaration + Initialization
	unsigned char status_counter;
	unsigned char reference;
	unsigned char flip_reference;
	double PMTimeStamp;
	__int64 Frequency, Counter;
	SMObject PMObj(TEXT("ProcessManagement"), sizeof(ProcessManagement));

	array<UGVProcesses>^ ProcessList = gcnew array<UGVProcesses>
	{
		{ "Laser",   1, 0, 10, gcnew Process },
		{ "Display", 1, 0, 10, gcnew Process },
		{ "Vehicle", 1, 0, 10, gcnew Process },
		{ "GPS",     0, 0, 10, gcnew Process },
		{ "Camera",  0, 0, 10, gcnew Process }
	};

	//array<String^>^ ModuleList = gcnew array<String^>{"Laser", "Display", "Vehicle", "GPS", "Camera"};
	//array<int>^ Critical = gcnew array<int>(ModuleList->Length) { 1, 1, 1, 0, 0 };
	//array<Process^>^ ProcessList = gcnew array<Process^>(ModuleList->Length);
	QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);
	//SM Creation and Seeking access
	PMObj.SMCreate();
	PMObj.SMAccess();

	//Setup SM_Laser
	SMObject LaserObj(_TEXT("SM_Laser"), sizeof(SM_Laser));
	LaserObj.SMCreate();
	LaserObj.SMAccess();

	//Setup SM_GPS
	SMObject GPSObj(_TEXT("SM_GPS"), sizeof(SM_GPS));
	GPSObj.SMCreate();
	GPSObj.SMAccess();

	//Setup SM_VehicleControl
	SMObject VehicleControlObj(_TEXT("SM_VehicleControl"), sizeof(SM_VehicleControl));
	VehicleControlObj.SMCreate();
	VehicleControlObj.SMAccess();

	ProcessManagement* PMData = (ProcessManagement*)PMObj.pData;

	for (int i = 0; i < ProcessList->Length; i++)
	{
		if (Process::GetProcessesByName(ProcessList[i].ModuleName)->Length == 0)
		{
			ProcessList[i].ProcessName = gcnew Process;
			ProcessList[i].ProcessName->StartInfo->WorkingDirectory = "C:\\Users\\Cjl\\source\\repos\\UGV_Assignment\\Executable";
			ProcessList[i].ProcessName->StartInfo->FileName = ProcessList[i].ModuleName;
			ProcessList[i].ProcessName->Start();
			Console::WriteLine("The process " + ProcessList[i].ModuleName + ".exe started");

		}
	}



	// Main loop

	while (PMData->Shutdown.Status != 0xFF)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&Counter);
		PMData->PMTimeStamp = PMTimeStamp = (double)Counter / (double)(Frequency) * 1000; //ms
		PMData->Ready = true;
		// Checking for heartbeats
		// iterate through all the processes

		// Test to show heartbeat flags
		//Console::WriteLine(PMData->Heartbeat.Flags.Laser + " " + PMData->Heartbeat.Flags.Display + " " + PMData->Heartbeat.Flags.Vehicle + " " + PMData->Heartbeat.Flags.GPS + " " + PMData->Heartbeat.Flags.Camera);


		status_counter = 1;
		for (int i = 0; i < ProcessList->Length; i++)
		{
			reference = (status_counter << i);
			// is the heartbeat bit for process[i] up?
			if ((PMData->Heartbeat.Status & reference) != 0)
			{
				// True -> put the bit of process[i] down 
				flip_reference = (0xFF ^ reference);
				PMData->Heartbeat.Status = (PMData->Heartbeat.Status & flip_reference);
				ProcessList[i].CrashCount = 0;
			}
			else
			{
				// False -> Increment the counter (heartbeat lost counter)
				ProcessList[i].CrashCount++;
				// is the counter passed the limit for process[i]
				if (ProcessList[i].CrashCount > ProcessList[i].CrashCountLimit)
				{
					// True -> is the process[i] critical?
					if (ProcessList[i].Critical == 1)
					{
						// True -> Shutdown all
						PMData->Shutdown.Status = 0xFF;
						break;
					}
					else
					{
						// False -> Has process[i] exites the operating system (HasExited())
						if (ProcessList[i].ProcessName->HasExited)
						{
							//True -> Start();
							ProcessList[i].ProcessName->Start();
							ProcessList[i].CrashCount = 0;
						}
						else
						{
							// False -> Kill(); Start();
							ProcessList[i].ProcessName->Kill();
							ProcessList[i].ProcessName->Start();
							ProcessList[i].CrashCount = 0;
						}
					}
				}

			}
		}


		Thread::Sleep(25);
	}

	PMData->Shutdown.Status = 0xFF;


	return 0;
}