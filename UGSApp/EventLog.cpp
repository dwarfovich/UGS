#include "stdafx.h"
#include "string.h"
#include "EventLog.h"

EventLog::EventLog()
{
    HKEY hk;
    unsigned long dwData;
    unsigned char szBuf[1024];

	hEventSource = 0;

	appName = CString(g_pNTService->m_szServiceName);

//    appName = ChangeFileExt(ExtractFileName(Application->ExeName), "");

    // Add your source name as a subkey under the Application
    // key in the EventLog registry key.

    if (::RegCreateKey(HKEY_LOCAL_MACHINE,
            ("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" + CString(appName)), &hk))
       /*AfxMessageBox("Could not create the registry key.")*/;

    // Set the name of the message file.
	CString file(g_pNTService->m_szWorkingDir);
	file = file.Left(file.ReverseFind('\\'));
	//cl.Delete(0);
	file = file + "\\evlogmsg.dll";

    strcpy_s((char *)szBuf, sizeof(szBuf), file);

    // Add the name to the EventMessageFile subkey. 

    if (::RegSetValueEx(hk,             // subkey handle
            "EventMessageFile",       // value name
            0,                        // must be zero 
            REG_EXPAND_SZ,            // value type
            (LPBYTE) szBuf,           // pointer to value data 
            (DWORD)strlen((char*)szBuf) + 1))       // length of value data
        /*AfxMessageBox("Could not set the event message file.")*/;

    // Set the supported event types in the TypesSupported subkey. 

    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
        EVENTLOG_INFORMATION_TYPE;
 
    if (::RegSetValueEx(hk,      // subkey handle
            "TypesSupported",  // value name
            0,                 // must be zero
            REG_DWORD,         // value type
            (LPBYTE) &dwData,  // pointer to value data
            sizeof(DWORD)))    // length of value data
        /*AfxMessageBox("Could not set the supported types.")*/;

    ::RegCloseKey(hk);
}
//---------------------------------------------------------------------------

void EventLog::log(const char* msg, unsigned short type, unsigned long id)
{

  if(type!=EVENTLOG_INFORMATION_TYPE) {
	  ::MessageBeep(-1);
  }
    // Check the event source has been registered and if
    // not then register it now
	if (!hEventSource) {
        hEventSource = ::RegisterEventSource(NULL,  // local machine
                                              g_pNTService->m_szServiceName); // source name
  }
  const char* ps[1];
  ps[0] = msg;
  char* m;


  if (hEventSource) {
        ::ReportEvent(hEventSource,
                      type,
                      0,
                      id,
                      NULL, // sid
                      1,
                      0,
                      ps,
                      NULL);
  }
  else
		m = (char*)winAPIerrorMessage();

}
//---------------------------------------------------------------------------

const char* EventLog::winAPIerrorMessage()
{
  LPVOID lpMsgBuf;
  DWORD errID;
  CString tmp;

  if(!::FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    errID = ::GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL
	)) {
	tmp.FormatMessage("%u",errID);
    errMsg = "(" + tmp + ")";

  }
  else {
	tmp.FormatMessage("%u",errID);
    errMsg = CString((const char*)lpMsgBuf) + "(" + tmp + ")";
  }
  // Free the buffer.
  LocalFree( lpMsgBuf );
  return errMsg;

}
//---------------------------------------------------------------------------

