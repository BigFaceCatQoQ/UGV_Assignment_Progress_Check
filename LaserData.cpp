#using <System.dll>

#include <Windows.h>
#include <conio.h>
#include <iostream>
#include <array>
#include <string>
#include <SMObject.h>
#include <smstructs.h>

using namespace System;
using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;
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

	// LMS151 port number must be 23000
	int PortNumber = 23000;
	// Pointer to TcpClent type object on managed heap
	TcpClient^ Client;
	// arrays of unsigned chars to send and receive data
	array<unsigned char>^ SendData;
	array<unsigned char>^ ReadData;
	// String command to ask for Channel 1 analogue voltage from the PLC
	// These command are available on Galil RIO47122 command reference manual
	// available online
	String^ AskScan = gcnew String("sRN LMDscandata");
	String^ StudID = gcnew String("z5266234\n");
	// String to store received data for display
	String^ ResponseData;

	// Creat TcpClient object and connect to it
	Client = gcnew TcpClient("192.168.1.200", PortNumber);
	// Configure connection
	Client->NoDelay = true;
	Client->ReceiveTimeout = 500;//ms
	Client->SendTimeout = 500;//ms
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;

	// unsigned char arrays of 16 bytes each are created on managed heap
	SendData = gcnew array<unsigned char>(16);
	ReadData = gcnew array<unsigned char>(2500);


	// Get the network streab object associated with clien so we 
	// can use it to read and write
	NetworkStream^ Stream = Client->GetStream();

	// Authenticate User
	// Convert string command to an array of unsigned char
	SendData = System::Text::Encoding::ASCII->GetBytes(StudID);
	Stream->Write(SendData, 0, SendData->Length);
	// Wait for the server to prepare the data, 1 ms would be sufficient, but used 10 ms
	System::Threading::Thread::Sleep(10);
	// Read the incoming data
	Stream->Read(ReadData, 0, ReadData->Length);
	ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);
	// Print the received string on the screen
	//Console::WriteLine(ResponseData);
	//Console::ReadKey();

	SendData = System::Text::Encoding::ASCII->GetBytes(AskScan);

	//Loop
	while (1)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&Counter);
		TimeStamp = (double)Counter / (double)Frequency * 1000; //ms
		// Did PM put my flag down?
		if (PMData->Heartbeat.Flags.Laser == 0)
		{
			// True -> Put my flag up
			PMData->Heartbeat.Flags.Laser = 1;

			// Write command asking for data
			Stream->WriteByte(0x02);
			Stream->Write(SendData, 0, SendData->Length);
			Stream->WriteByte(0x03);
			// Wait for the server to prepare the data, 1 ms would be sufficient, but used 10 ms
			System::Threading::Thread::Sleep(10);
			// Read the incoming data
			Stream->Read(ReadData, 0, ReadData->Length);
			// Convert incoming data from an array of unsigned char bytes to an ASCII string
			ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);
			// Print the received string on the screen
			std::array<std::string> ResultDaTa{};
			int counter = 0;
			for (int i = 0; i < ResponseData->Length; i++)
			{
				if (ResponseData[i] == ' ')
				{
					counter++;
				}
				else
				{
					ResultData[counter] += ResponseData[i];
				}
			}
			
			Console::WriteLine(ResponseData);

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

		if (PMData->Shutdown.Status)
			break;
		if (_kbhit())
			break;
	}

	Stream->Close();
	Client->Close();

	Console::ReadKey();
	Console::ReadKey();


	return 0;
}