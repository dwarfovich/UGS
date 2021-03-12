// NTService.cpp
//
// Implementation of CNTService

#include "stdafx.h"
//#include <windows.h>
//#include <stdio.h>
#include "NTService.h"

#include "NTSECAPI.H"
#include "CRTDBG.H"
#include "STDDEF.H"
#include <atlbase.h>
#include <atlconv.h>


#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)
// static variables

CNTService* CNTService::m_pThis = NULL;

CNTService::CNTService(const char* szServiceName)
{
    // copy the address of the current object so we can access it from
    // the static member callback functions. 
    // WARNING: This limits the application to only one CNTService object. 
    m_pThis = this;
    
    // Set the default service name and version
    strncpy_s(m_szServiceName, sizeof(m_szServiceName), szServiceName, sizeof(m_szServiceName)-1);
    m_hEventSource = NULL;

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_Status.dwCurrentState = SERVICE_STOPPED;
    m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_Status.dwWin32ExitCode = 0;
    m_Status.dwServiceSpecificExitCode = 0;
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    m_bIsRunning = FALSE;
}

CNTService::~CNTService()
{
    DebugMsg("CNTService::~CNTService()");
    if (m_hEventSource) {
        ::DeregisterEventSource(m_hEventSource);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// Default command line argument parsing

// Returns TRUE if it found an arg it recognised, FALSE if not
// Note: processing some arguments causes output to stdout to be generated.
BOOL CNTService::ParseStandardArgs(int argc, char* argv[])
{
    // See if we have any command line args we recognise
    if (argc <= 1) return FALSE;

    if (_stricmp(argv[1], "-v") == 0) {

        // Spit out version info
		printf("%s\n",VERSION_STRING);
        printf("The service is %s installed\n",
               IsInstalled() ? "currently" : "not");
        return TRUE; // say we processed the argument

    } else if (_stricmp(argv[1], "-i") == 0) 
	{
        // Request to install.
        if (IsInstalled()) {
            printf("\"%s\" is already installed\n", m_szServiceName);
        } else {
            // Try and install the copy that's running
			BOOL bRet;
			if (argc == 4)
			{
				bRet = Install(argv[2], argv[3]);
			} else if (argc == 3)
			{
				bRet = Install(argv[2]);
			} else
				bRet = Install();
            if (bRet) {
				if (argc >= 3)
				{

					HRESULT hRes;
					// grant privilege to this account
					hRes = GrantPrivilege(NULL, argv[2],
										  SE_SERVICE_LOGON_NAME);

					if (hRes == S_OK)
					{
						printf("The account %s has been granted the Log On As A Service right\n", argv[2]);
					} else if (hRes == S_FALSE)
					{
						printf("The Log On As A Service right already granted to the %s account\n", argv[2]);
					} else
					{
						printf("Can't grant Log On As A Service right to the %s account\n", argv[2]);
					}
				}

                printf("\"%s\" installed\n", m_szServiceName);
            } else {
				LPVOID lpMsgBuf;
				FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL 
				);
                printf("\"%s\" failed to install. Error \"%s\"\n", m_szServiceName, lpMsgBuf);
				LocalFree( lpMsgBuf );
            }
        }
        return TRUE; // say we processed the argument

    } else if (_stricmp(argv[1], "-u") == 0) {

        // Request to uninstall.
        if (!IsInstalled()) {
            printf("\"%s\" is not installed\n", m_szServiceName);
        } else {
            // Try and remove the copy that's installed
            if (Uninstall()) {
                // Get the executable file path
                //char szFilePath[_MAX_PATH];
                //::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));
                printf("\"%s\" removed.\n", m_szServiceName);
            } else {
				LPVOID lpMsgBuf;
				FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL 
				);
                printf("Could not remove \"%s\". Error \"%s\"\n", m_szServiceName, lpMsgBuf);
				LocalFree( lpMsgBuf );
            }
        }
        return TRUE; // say we processed the argument
    
    }
         
    // Don't recognise the args
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////
// Install/uninstall routines

// Test if the service is currently installed
BOOL CNTService::IsInstalled()
{
    BOOL bResult = FALSE;

    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (hSCM) {

        // Try to open the service
        SC_HANDLE hService = ::OpenService(hSCM,
                                           m_szServiceName,
                                           SERVICE_QUERY_CONFIG);
        if (hService) {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }

        ::CloseServiceHandle(hSCM);
    }
    
    return bResult;
}

BOOL CNTService::Install(LPCTSTR lpServiceStartName, LPCTSTR lpPassword)
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) return FALSE;

    // Get the executable file path
    char szFilePath[_MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));

    // Create the service
    SC_HANDLE hService = ::CreateService(hSCM,
                                         m_szServiceName,
                                         m_szServiceName,
                                         SERVICE_ALL_ACCESS,
                                         SERVICE_WIN32_OWN_PROCESS,
                                         SERVICE_AUTO_START,        // start condition
                                         SERVICE_ERROR_NORMAL,
                                         szFilePath,
                                         NULL,
                                         NULL,
                                         NULL,
                                         lpServiceStartName,
                                         lpPassword);
    if (!hService) {
        ::CloseServiceHandle(hSCM);
        return FALSE;
    }

	if (!RegEventMessagesFile()) {
        ::CloseServiceHandle(hService);
        ::CloseServiceHandle(hSCM);
		return FALSE;
	}

    LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_INSTALLED, m_szServiceName);

    // tidy up
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);
    return TRUE;
}

//---------------------------------------------------------------------------
// GrantPrivilege
//
//  Grants the specified privilege to the specified user account.
//
//  Parameters:
//	  pszMachineName   - name of the target machine
//	  pszUserName	   - user name
//	  pszPrivilegeName - name of the privilege
//
//  Returns:
//	  * S_OK, if the privilege was granted;
//	  * S_FALSE, if the privilege already granted to the user;
//	  * any standard COM error code in the case of error.
//

HRESULT

CNTService::GrantPrivilege(
	LPCTSTR pszMachineName,
	LPCTSTR pszUserName,
	LPCTSTR pszPrivilegeName
	)
{
	//ASSERT(pszUserName != NULL);
	//ASSERT(pszPrivilegeName != NULL);

	if (pszUserName[0] == ('.') &&
		pszUserName[1] == ('\\'))
		pszUserName += 2;

    BYTE bSid[8 + 4 * SID_MAX_SUB_AUTHORITIES];
    DWORD cbSid = sizeof(bSid);
	TCHAR szDomainName[_MAX_PATH];
	DWORD cchDomainName = countof(szDomainName);
	SID_NAME_USE Use;

	// lookup user account SID
	if (!LookupAccountName(pszMachineName, pszUserName, (PSID)bSid,
						   &cbSid, szDomainName, &cchDomainName, &Use))
		return HRESULT_FROM_WIN32(GetLastError());

	USES_CONVERSION;

	NTSTATUS Status;
	LSA_HANDLE hPolicy;
	LSA_UNICODE_STRING SystemName;
	LSA_UNICODE_STRING UserRight;
	PLSA_UNICODE_STRING pSystemName = NULL;
	PLSA_UNICODE_STRING pCurrentRights;
	LSA_OBJECT_ATTRIBUTES ObjAttr;
	ULONG cRights = 0;

	if (pszMachineName != NULL)
	{
		SystemName.Buffer = (PWSTR)T2W(pszMachineName);
		SystemName.Length = (USHORT)(lstrlenW(SystemName.Buffer) * sizeof(WCHAR));
		SystemName.MaximumLength = (USHORT)(SystemName.Length + sizeof(WCHAR));

		pSystemName = &SystemName;
	}

	UserRight.Buffer = (PWSTR)T2W(pszPrivilegeName);
	UserRight.Length = (USHORT)(lstrlenW(UserRight.Buffer) * sizeof(WCHAR));
	UserRight.MaximumLength = (USHORT)(UserRight.Length + sizeof(WCHAR));
	
	memset(&ObjAttr, 0, sizeof(ObjAttr));

	// open policy object
	Status = LsaOpenPolicy(pSystemName, &ObjAttr, 
						   POLICY_LOOKUP_NAMES|POLICY_CREATE_ACCOUNT,
						   &hPolicy);
	if (!LSA_SUCCESS(Status))
		return HRESULT_FROM_NT(Status);

	// detemine which rights the user currently has
	Status = LsaEnumerateAccountRights(hPolicy, (PSID)bSid, &pCurrentRights, 
									   &cRights);
	if (Status != STATUS_OBJECT_NAME_NOT_FOUND && !LSA_SUCCESS(Status))
	{
		LsaClose(hPolicy);
		return HRESULT_FROM_NT(Status);
	}

	// check if this right is already granted
	for (ULONG i = 0; i < cRights; i++)
	{
		if (UserRight.Length == pCurrentRights[i].Length &&
			memcmp(UserRight.Buffer, pCurrentRights[i].Buffer, 
				   UserRight.Length) == 0)
		{
			LsaFreeMemory(pCurrentRights);
			LsaClose(hPolicy);
			return S_FALSE;
		}
	}

	// add privilege to the account
	Status = LsaAddAccountRights(hPolicy, (PSID)bSid, &UserRight, 1);

	// close policy handle
	LsaFreeMemory(pCurrentRights);
	LsaClose(hPolicy);

	if (!LSA_SUCCESS(Status))
		return HRESULT_FROM_NT(Status);

	return S_OK;
}

BOOL CNTService::Uninstall()
{
    // Open the Service Control Manager
    SC_HANDLE hSCM = ::OpenSCManager(NULL, // local machine
                                     NULL, // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS); // full access
    if (!hSCM) return FALSE;

    BOOL bResult = FALSE;
    SC_HANDLE hService = ::OpenService(hSCM,
                                       m_szServiceName,
                                       DELETE);
    if (hService) {
        if (::DeleteService(hService)) {
            LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_REMOVED, m_szServiceName);
            bResult = TRUE;
        } else {
            LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_NOTREMOVED, m_szServiceName);
        }
        ::CloseServiceHandle(hService);
    }
    
    ::CloseServiceHandle(hSCM);
    return bResult;
}

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions

BOOL CNTService::RegEventMessagesFile()
{
    // Get the executable file path
    char szFilePath[_MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));

    // make registry entries to support logging messages
    // Add the source name as a subkey under the Application
    // key in the EventLog service portion of the registry.
    char szKey[256];
    HKEY hKey = NULL;
    strcpy_s(szKey, sizeof(szKey), "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
    strcat_s(szKey, sizeof(szKey), m_szServiceName);
    if (::RegCreateKey(HKEY_LOCAL_MACHINE, szKey, &hKey) != ERROR_SUCCESS)
        return FALSE;

    // Add the Event ID message-file name to the 'EventMessageFile' subkey.
    ::RegSetValueEx(hKey,
                    "EventMessageFile",
                    0,
                    REG_EXPAND_SZ, 
                    (CONST BYTE*)szFilePath,
                    strlen(szFilePath) + 1);     

    // Set the supported types flags.
    DWORD dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    ::RegSetValueEx(hKey,
                    "TypesSupported",
                    0,
                    REG_DWORD,
                    (CONST BYTE*)&dwData,
                     sizeof(DWORD));
    ::RegCloseKey(hKey);

	return TRUE;
}

// This function makes an entry into the application event log
void CNTService::LogEvent(WORD wType, DWORD dwID,
                          const char* pszS1,
                          const char* pszS2,
                          const char* pszS3)
{
    const char* ps[3];
    ps[0] = pszS1;
    ps[1] = pszS2;
    ps[2] = pszS3;

    int iStr = 0;
    for (int i = 0; i < 3; i++) {
        if (ps[i] != NULL) iStr++;
    }
        
    // Check the event source has been registered and if
    // not then register it now
    if (!m_hEventSource) {
        m_hEventSource = ::RegisterEventSource(NULL,  // local machine
                                               m_szServiceName); // source name
    }

    if (m_hEventSource) {
        ::ReportEvent(m_hEventSource,
                      wType,
                      0,
                      dwID,
                      NULL, // sid
                      iStr,
                      0,
                      ps,
                      NULL);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration

BOOL CNTService::StartService()
{
    SERVICE_TABLE_ENTRY st[] = {
        {m_szServiceName, ServiceMain},
        {NULL, NULL}
    };

    DebugMsg("Calling StartServiceCtrlDispatcher()");
    BOOL b = ::StartServiceCtrlDispatcher(st);
    DebugMsg("Returned from StartServiceCtrlDispatcher()");
    return b;
}

// static member function (callback)
void CNTService::ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    // Get a pointer to the C++ object
    CNTService* pService = m_pThis;
    
    pService->DebugMsg("Entering CNTService::ServiceMain()");
    // Register the control request handler
    pService->m_Status.dwCurrentState = SERVICE_START_PENDING;
    pService->m_hServiceStatus = RegisterServiceCtrlHandler(pService->m_szServiceName,
                                                           Handler);
    if (pService->m_hServiceStatus == NULL) {
        pService->LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_CTRLHANDLERNOTINSTALLED);
        return;
    }

    // Start the initialisation
    if (pService->Initialize()) {

        // Do the real work. 
        // When the Run function returns, the service has stopped.
        pService->m_bIsRunning = TRUE;
        pService->m_Status.dwWin32ExitCode = 0;
        pService->m_Status.dwCheckPoint = 0;
        pService->m_Status.dwWaitHint = 0;
        pService->Run();
    }

    // Tell the service manager we are stopped
    pService->SetStatus(SERVICE_STOPPED);

    pService->DebugMsg("Leaving CNTService::ServiceMain()");
}

void CNTService::SetWorkingDir(const char * szPath)
{
	char path[_MAX_PATH], szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
	strcpy_s(path, sizeof(path), szPath);

	int iLength = __min(strlen(szPath), _MAX_PATH - 1), iNewLength = 0;
	for (int i = 0; i < iLength; i++)
	{
		char c = *(szPath + i);
		if (c != '\"')
			*(path + iNewLength++) = c;
	}

	*(path + iNewLength) = '\0';

	_splitpath_s(path, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, NULL, 0, NULL, 0);
	_makepath_s(m_szWorkingDir, _MAX_DIR, szDrive, szDir, NULL, NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////
// status functions

void CNTService::SetStatus(DWORD dwState)
{
    DebugMsg("CNTService::SetStatus(%lu, %lu)", m_hServiceStatus, dwState);
    m_Status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_Status);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Service initialization

BOOL CNTService::Initialize()
{
    DebugMsg("Entering CNTService::Initialize()");

    // Start the initialization
    SetStatus(SERVICE_START_PENDING);
    
    // Perform the actual initialization
    BOOL bResult = OnInit(); 
    
    // Set final state
    m_Status.dwWin32ExitCode = GetLastError();
    m_Status.dwCheckPoint = 0;
    m_Status.dwWaitHint = 0;
    if (!bResult) {
        LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_FAILEDINIT);
        SetStatus(SERVICE_STOPPED);
        return FALSE;    
    }
    
    LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STARTED);
    SetStatus(SERVICE_RUNNING);

    DebugMsg("Leaving CNTService::Initialize()");
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// main function to do the real work of the service

// This function performs the main work of the service. 
// When this function returns the service has stopped.
void CNTService::Run()
{
    DebugMsg("Entering CNTService::Run()");

    while (m_bIsRunning) {
        DebugMsg("Sleeping...");
        Sleep(5000);
    }

    // nothing more to do
    DebugMsg("Leaving CNTService::Run()");
}

//////////////////////////////////////////////////////////////////////////////////////
// Control request handlers

// static member function (callback) to handle commands from the
// service control manager
void CNTService::Handler(DWORD dwOpcode)
{
    // Get a pointer to the object
    CNTService* pService = m_pThis;
    
    pService->DebugMsg("CNTService::Handler(%lu)", dwOpcode);
    switch (dwOpcode) {
    case SERVICE_CONTROL_STOP: // 1
        pService->SetStatus(SERVICE_STOP_PENDING);
        pService->OnStop();
        pService->m_bIsRunning = FALSE;
        pService->LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STOPPED);
        break;

    case SERVICE_CONTROL_PAUSE: // 2
        pService->OnPause();
        break;

    case SERVICE_CONTROL_CONTINUE: // 3
        pService->OnContinue();
        break;

    case SERVICE_CONTROL_INTERROGATE: // 4
        pService->OnInterrogate();
        break;

    case SERVICE_CONTROL_SHUTDOWN: // 5
        pService->OnShutdown();
        break;

    default:
        if (dwOpcode >= SERVICE_CONTROL_USER) {
            if (!pService->OnUserControl(dwOpcode)) {
                pService->LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_BADREQUEST);
            }
        } else {
            pService->LogEvent(EVENTLOG_ERROR_TYPE, EVMSG_BADREQUEST);
        }
        break;
    }

    // Report current status
    pService->DebugMsg("Updating status (%lu, %lu)",
                       pService->m_hServiceStatus,
                       pService->m_Status.dwCurrentState);
    ::SetServiceStatus(pService->m_hServiceStatus, &pService->m_Status);
}
        
// Called when the service is first initialized
BOOL CNTService::OnInit()
{
    DebugMsg("CNTService::OnInit()");
	return TRUE;
}

// Called when the service control manager wants to stop the service
void CNTService::OnStop()
{
    DebugMsg("CNTService::OnStop()");
}

// called when the service is interrogated
void CNTService::OnInterrogate()
{
    DebugMsg("CNTService::OnInterrogate()");
}

// called when the service is paused
void CNTService::OnPause()
{
    DebugMsg("CNTService::OnPause()");
}

// called when the service is continued
void CNTService::OnContinue()
{
    DebugMsg("CNTService::OnContinue()");
}

// called when the service is shut down
void CNTService::OnShutdown()
{
    DebugMsg("CNTService::OnShutdown()");
}

// called when the service gets a user control message
BOOL CNTService::OnUserControl(DWORD dwOpcode)
{
    DebugMsg("CNTService::OnUserControl(%8.8lXH)", dwOpcode);
    return FALSE; // say not handled
}


////////////////////////////////////////////////////////////////////////////////////////////
// Debugging support

void CNTService::DebugMsg(const char* pszFormat, ...)
{
    char buf[1024];
    sprintf_s(buf, sizeof(buf), "[%s](%lu): ", m_szServiceName, GetCurrentThreadId());
	va_list arglist;
	va_start(arglist, pszFormat);
    size_t currentLength = strlen(buf);
    vsprintf_s(&buf[currentLength], sizeof(buf) - currentLength, pszFormat, arglist);
	va_end(arglist);
    strcat_s(buf, sizeof(buf), "\n");
    OutputDebugString(buf);
}
