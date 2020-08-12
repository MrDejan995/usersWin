#include <windows.h>
#pragma comment( lib, "Wtsapi32.lib")
#include <iostream>
#include <sstream>
#include <NTSecAPI.h>
#include <WtsApi32.h>

#define assert(cond, msg) errorFatal(cond, msg, __FILE__, __LINE__)

void errorFatal(bool cond, const char *msg, const char *file, int line)
{
	if (!cond)
		std::cerr << file << " : " << line << " " << msg << std::endl;
}

int main()
{
	SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES), NULL, FALSE}; // outer security attributes
	HANDLE logfile = CreateFileA("C:\\log-app.txt", GENERIC_WRITE, 0,
							&sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); // Create log-app.txt file
	assert(logfile != INVALID_HANDLE_VALUE, "Create log file failed");

	PWTS_SESSION_INFO_1A sessions[1024];
	DWORD counts;
	assert(WTSEnumerateSessionsExA(WTS_CURRENT_SERVER_HANDLE, 0, 1, sessions, &counts) != 0, "WTSEnumerateSession failed");
	
	for(auto i = 0; i < counts; i++)
	{
		std::ostringstream os;
		os << sessions[i]->pUserName << ": ";
		HANDLE readEnd, writeEnd;
		SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), NULL, TRUE }; // inner security attributes
		assert(CreatePipe(&readEnd, &writeEnd, &sa, 0) != 0, "CreatePipe failed"); // inner SA
		HANDLE userToken;
		assert(WTSQueryUserToken(sessions[i]->SessionId, &userToken) != 0, "WTSQueryUserToken failed");
		LPSTARTUPINFOA si;
		LPPROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));

		HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		assert(outputHandle != INVALID_HANDLE_VALUE, "GetStdHandle failed");
		assert(SetStdHandle(STD_OUTPUT_HANDLE, writeEnd)!= 0, "SetStdHandle failed");
		char appname[] = "UsersForegroundWindow.exe";
		
		DWORD rc = CreateProcessAsUserA(userToken,
			0,
			appname,
			0,
			0,
			TRUE,
			NORMAL_PRIORITY_CLASS,
			NULL,
			0,
			si,
			pi); // create process as user

		if (!rc)
		{
			DWORD rc = GetLastError();
			RevertToSelf();
			CloseHandle(userToken);
			assert(false, "CreateProcessAsUser failed");
		}
		assert(WaitForInputIdle(pi->hProcess, 1000) == 0, "WaitForInputIdle failed");

		char lpbuffer[512];
		DWORD nBytesRead = 0;
		assert(ReadFile(readEnd, lpbuffer, sizeof lpbuffer, &nBytesRead, NULL) != 0, "ReadFile failed"); // read users input
		os << lpbuffer << std::endl;
		assert(WriteFile(logfile, os.str().c_str(), nBytesRead, NULL, NULL) != 0, "WriteFile failed"); // write users input to log file
		CloseHandle(readEnd); // close pipe
		CloseHandle(writeEnd); // close pipe
		assert(SetStdHandle(STD_OUTPUT_HANDLE, outputHandle) != 0, "SetStdHandle failed");
	}

	CloseHandle(logfile);
}