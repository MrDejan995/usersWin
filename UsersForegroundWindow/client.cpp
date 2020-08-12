#include <windows.h>
#include <wmlss.h>
#include <sstream>
#include <iostream>

#define assert(cond, msg) errorFatal(cond, msg, __FILE__, __LINE__)

void errorFatal(bool cond, const char* msg, const char* file, int line)
{
	if (!cond)
		std::cerr << file << " : " << line << " " << msg << std::endl;
}

int main()
{
	HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	assert(outputHandle != INVALID_HANDLE_VALUE, "GetStdHandle failed");
	char lpbuffer[1024];
	HWND window = GetForegroundWindow();
	assert(GetWindowTextA(window, lpbuffer, 1024) != 0, "GetWindowText failed");
	std::ostringstream os;

	os << "Active window: " << lpbuffer;

	WriteFile(outputHandle, os.str().c_str(), os.str().length(), NULL, NULL);
}