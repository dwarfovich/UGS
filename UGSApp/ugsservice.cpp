// myservice.cpp

#include "stdafx.h"
#include "NTServApp.h"
#include "UGSservice.h"
#include "ugs.h"
#include "../UGSLib/Settings.h"

CUGSService::CUGSService(const char * szServiceName)
:CNTService(szServiceName)
{
	m_iStartParam = 0;
	m_iState = m_iStartParam;
}

BOOL CUGSService::OnInit()
{
	// Read the registry parameters
    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\Parameters
    HKEY hkey;
	char szKey[1024];
	strcpy_s(szKey, sizeof(szKey), "SYSTEM\\CurrentControlSet\\Services\\");
	strcat_s(szKey, sizeof(szKey), m_szServiceName);
	strcat_s(szKey, sizeof(szKey), "\\Parameters");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szKey,
                     0,
                     KEY_QUERY_VALUE,
                     &hkey) == ERROR_SUCCESS) {
        // Yes we are installed
        DWORD dwType = 0;
        DWORD dwSize = sizeof(m_iStartParam);
        RegQueryValueEx(hkey,
                        "Start",
                        NULL,
                        &dwType,
                        (BYTE*)&m_iStartParam,
                        &dwSize);
        RegCloseKey(hkey);
    }

	// Set the initial state
	m_iState = m_iStartParam;

	return TRUE;
}


void CUGSService::Run()
{
	char szFilePath[_MAX_PATH];
	::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));
	SetWorkingDir(szFilePath);


	_tprintf(_T("%s\n"), VERSION_STRING);		//����� ������ ���������
	el = new EventLog;

   //set highest priority to thread, and high to process:
	DWORD threadId = ::GetCurrentThreadId();
	DWORD procId = ::GetCurrentProcessId();
    HANDLE hProcess;
    if((hProcess = ::OpenProcess(PROCESS_SET_INFORMATION,
                                FALSE,
                                procId))==NULL)
      el->log("OpenProcess: " + CString(el->winAPIerrorMessage()), EVENTLOG_ERROR_TYPE);

    HANDLE hThread;
    if((hThread = ::OpenThread(THREAD_SET_INFORMATION, FALSE, threadId))==NULL)
      el->log("OpenThread: " + CString(el->winAPIerrorMessage()), EVENTLOG_ERROR_TYPE);

    if(!::SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
      el->log("SetPriorityClass: " + CString(el->winAPIerrorMessage()), EVENTLOG_ERROR_TYPE);

    if(!::SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST))
      el->log("SetThreadPriority: " + CString(el->winAPIerrorMessage()), EVENTLOG_ERROR_TYPE);

    if(!::CloseHandle(hThread))
		el->log("CloseHandle: " + CString(el->winAPIerrorMessage()), EVENTLOG_ERROR_TYPE);

	if(!::CloseHandle(hProcess))
      el->log("CloseHandle: " + CString(el->winAPIerrorMessage()), EVENTLOG_ERROR_TYPE);


	WSAData wsaData;
	::WSAStartup(MAKEWORD(2, 2), &wsaData);

	TCHAR szIniPath[_MAX_PATH];
	_tmakepath_s(szIniPath, _MAX_PATH, NULL, m_szWorkingDir, m_szServiceName, "ini");

	g_Settings.Load(szIniPath);

	TCHAR szReportPath[_MAX_PATH], szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szName[_MAX_FNAME], szExt[_MAX_EXT];
	_splitpath_s(m_szWorkingDir, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, NULL, 0, NULL, 0);
	const auto& stdStr = g_Settings.m_strSettingsReportPath.toStdString();
	_splitpath_s(stdStr.c_str(), NULL, 0, NULL, 0, szName, _MAX_FNAME, szExt, _MAX_EXT);
	_makepath_s(szReportPath, _MAX_PATH, szDrive, szDir, szName, szExt);

	g_Settings.Save(szReportPath);


	try
	{
		// Main loop init
		SOCKADDR_IN sockAddr;
		int iSockAddrLen = sizeof(sockAddr);

		int iBufferLength, iValueSize = sizeof(iBufferLength);
			iBufferLength = MAXWORD;

		char * pBuffer = new char[iBufferLength];
		memset(pBuffer, 0, iBufferLength);

		BOOL bRun = TRUE;

		LARGE_INTEGER liDueTime;
		liDueTime.QuadPart = -10000 * g_Settings.m_dwPollingPeriod;
		HANDLE hTimer = ::CreateWaitableTimer(NULL,	FALSE, NULL);
		if (NULL == hTimer || !::SetWaitableTimer(hTimer, &liDueTime, g_Settings.m_dwPollingPeriod, NULL, NULL, 0))
			bRun = FALSE;

		g_Settings.Save(szReportPath);

		arDataBuffer = new BYTE[g_Settings.m_nBufferSize];

		// Main loop
		UINT sendStatusPeriodCounter = 0;
		for (int statusPeriodCounter = 0; bRun && m_bIsRunning;)
		{
			_tprintf(_T("."));

			if(::WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
				continue;

			statusPeriodCounter += g_Settings.m_dwPollingPeriod;
		}

		delete [] pBuffer;

		if(hTimer != NULL)
			::CloseHandle(hTimer);
	}
	catch(int iError)
	{
		CString erMsg;
		switch(iError)
		{
		case 1:
			erMsg = "Fatal Error: unable to create UDP status sender";
			_tprintf(_T(erMsg + '\n')); 
			el->log(erMsg, EVENTLOG_ERROR_TYPE);
			break;	
		case 2: 
			erMsg = "Fatal Error: unable to create UDP listener";	
			_tprintf(_T(erMsg + '\n')); 
			el->log(erMsg, EVENTLOG_ERROR_TYPE);
			break;
		case 3: 
			erMsg = "Fatal Error: unable to create UDP sender";	
			_tprintf(_T(erMsg + '\n')); 
			el->log(erMsg, EVENTLOG_ERROR_TYPE);
			break;
		case 4:
			erMsg = "Fatal Error: unable to create logger";	
			_tprintf(_T(erMsg + '\n')); 
			el->log(erMsg, EVENTLOG_ERROR_TYPE);
			break;
		case 5: 
			erMsg = "Fatal Error: unable to open COM port";	
			_tprintf(_T(erMsg + '\n')); 
			el->log(erMsg, EVENTLOG_ERROR_TYPE);
			break;
		case 6: 
			erMsg = "Fatal Error: unable to create Status sender";	
			_tprintf(_T(erMsg + '\n')); 
			el->log(erMsg, EVENTLOG_ERROR_TYPE);
			break;

		}
	}

	if (hProcess != NULL)
		::CloseHandle(hProcess);

	if (hThread != NULL)
		::CloseHandle(hThread);

	::WSACleanup();
	delete [] arDataBuffer;
	delete el;

	m_Status.dwWin32ExitCode = 0;
	return;
}

// Process user control requests
BOOL CUGSService::OnUserControl(DWORD dwOpcode)
{
    switch (dwOpcode) {
    case SERVICE_CONTROL_USER + 0:

        // Save the current status in the registry
        SaveStatus();
        return TRUE;

    default:
        break;
    }
    return FALSE; // say not handled
}

// Save the current status in the registry
void CUGSService::SaveStatus()
{
    DebugMsg("Saving current status");
    // Try opening the registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\<AppName>\...
    HKEY hkey = NULL;
	char szKey[1024];
	strcpy_s(szKey, 1024, "SYSTEM\\CurrentControlSet\\Services\\");
	strcat_s(szKey, 1024, m_szServiceName);
	strcat_s(szKey, 1024, "\\Status");
    DWORD dwDisp;
	DWORD dwErr;
    DebugMsg("Creating key: %s", szKey);
    dwErr = RegCreateKeyEx(	HKEY_LOCAL_MACHINE,
                           	szKey,
                   			0,
                   			"",
                   			REG_OPTION_NON_VOLATILE,
                   			KEY_WRITE,
                   			NULL,
                   			&hkey,
                   			&dwDisp);
	if (dwErr != ERROR_SUCCESS) {
		DebugMsg("Failed to create Status key (%lu)", dwErr);
		return;
	}	

    // Set the registry values
	DebugMsg("Saving 'Current' as %ld", m_iState); 
    RegSetValueEx(hkey,
                  "Current",
                  0,
                  REG_DWORD,
                  (BYTE*)&m_iState,
                  sizeof(m_iState));


    // Finished with key
    RegCloseKey(hkey);

}
