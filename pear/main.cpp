#include <windows.h>

// Redefine RtlZeroMemory() to avoid link to CRT
#undef RtlZeroMemory
EXTERN_C NTSYSAPI VOID NTAPI RtlZeroMemory (LPVOID UNALIGNED Dst, SIZE_T Length);

// Shortcut for WriteConsole("[pear.exe] {lpTitle}: {lpMessage}\n")
void WriteMessage(LPTSTR lpTitle, LPTSTR lpMessage) {
	HANDLE hStdOutput;
	CONSOLE_SCREEN_BUFFER_INFO screenBuffer;
	DWORD dwWriteByte;

	hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hStdOutput, &screenBuffer);
	WriteConsole(hStdOutput, L"[pear.exe] ", 11, &dwWriteByte, NULL);
	WriteConsole(hStdOutput, lpTitle, lstrlen(lpTitle), &dwWriteByte, NULL);
	WriteConsole(hStdOutput, L": ", 2, &dwWriteByte, NULL);
	WriteConsole(hStdOutput, lpMessage, lstrlen(lpMessage), &dwWriteByte, NULL);
	WriteConsole(hStdOutput, L"\n", 1, &dwWriteByte, NULL);
}

void WriteError() {
	LPTSTR lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);
	WriteMessage(L"error detail", lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// Build command line that calls pear.bat instead of pear.exe
void BuildCommandLine(LPTSTR lpBatFilename, LPTSTR lpCommandLine) {
	// Get commandline args
	LPTSTR lpOrgCommandLine = GetCommandLine();

	// Search for arguments
	wchar_t *lpArgs = NULL;
	if (lpOrgCommandLine[0] == L'"') {
		// Don't use strchr() because that function use CRT
		for (wchar_t *p = lpOrgCommandLine + 1; *p != '\0'; p++) {
			if (*p == L'"') {
				lpArgs = p + 1;
				break;
			}
		}
	} else {
		for (wchar_t *p = lpOrgCommandLine + 1; *p != '\0'; p++) {
			if (*p == L' ') {
				lpArgs = p;
				break;
			}
		}
	}

	// "cmd /c call ..." is workaround to get %ERRORLEVEL% propery
	// see http://stackoverflow.com/questions/769631/getting-batch-script-error-code
	lstrcpy(lpCommandLine, L"cmd /c call \"");
	lstrcat(lpCommandLine, lpBatFilename);
	lstrcat(lpCommandLine, L"\"");
	if (lpArgs) {
		lstrcat(lpCommandLine, lpArgs);
	}
}

int main(int argc, char *argv[])
{
	int iRtn = 0;

	// Get the full path of current process(pear.exe)
	wchar_t lpExeFilename[_MAX_PATH];
	DWORD nSize = GetModuleFileName(NULL, lpExeFilename, _MAX_PATH);

	// Set path of pear.ini to %PHP_PEAR_SYSCONF_DIR% if not set
	if (!GetEnvironmentVariable(L"PHP_PEAR_SYSCONF_DIR", NULL, 0)) {
		// Don't use _wsplitpath() and _wmakepath() because those functions use CRT
		wchar_t lpIniFilename[_MAX_PATH];
		lstrcpy(lpIniFilename, lpExeFilename);
		lstrcpy(lpIniFilename + nSize - lstrlen(L"\\pear.exe"), L"");
		SetEnvironmentVariable(L"PHP_PEAR_SYSCONF_DIR", lpIniFilename);
	}

	// Get path of pear.bat
	wchar_t lpBatFilename[_MAX_PATH];
	lstrcpy(lpBatFilename, lpExeFilename);
	lstrcpy(lpBatFilename + nSize - 3, L"bat");

	// Get commandline args
	wchar_t lpCommandLine[_MAX_PATH];
	BuildCommandLine(lpBatFilename, lpCommandLine);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	RtlZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	RtlZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(
		NULL,             // No module name (use command line). 
		lpCommandLine,    // Command line. 
		NULL,             // Process handle not inheritable. 
		NULL,             // Thread handle not inheritable. 
		FALSE,            // Set handle inheritance to FALSE. 
		0,                // No creation flags.
		NULL,             // Use parent's environment block.
		NULL,             // Use parent's starting directory.
		&si,              // Pointer to STARTUPINFO structure.
		&pi )             // Pointer to PROCESS_INFORMATION structure.
		)
	{
		WriteMessage(L"error", L"CreateProcess failed.");
		WriteError();
		WriteMessage(L"command line", lpCommandLine);
		iRtn = -1;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	if (iRtn == 0) {
		DWORD exitCode;
		if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
			iRtn = exitCode;
		}
		else {
			WriteMessage(L"error", L"GetExitCodeProcess failed.");
			WriteError();
			WriteMessage(L"command line", lpCommandLine);
			iRtn = -2;
		}
	}

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return iRtn;
}
