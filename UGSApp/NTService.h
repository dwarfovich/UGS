// ntservice.h
//
// Definitions for CNTService
//

#ifndef _NTSERVICE_H_
#define _NTSERVICE_H_

#ifndef countof
#   define countof(a)	    (sizeof(a)/sizeof(a[0]))
#endif

#define SERVICE_CONTROL_USER 128

class CNTService
{
public:
    CNTService(const char* szServiceName);
    virtual ~CNTService();
    BOOL ParseStandardArgs(int argc, char* argv[]);
    BOOL IsInstalled();
    BOOL Install( LPCTSTR lpServiceStartName = NULL, // account name 
				  LPCTSTR lpPassword = NULL);        // account password
				
    BOOL Uninstall();
	BOOL RegEventMessagesFile();
    void LogEvent(WORD wType, DWORD dwID,
                  const char* pszS1 = NULL,
                  const char* pszS2 = NULL,
                  const char* pszS3 = NULL);
    BOOL StartService();
	void SetWorkingDir(const char * szPath);
    void SetStatus(DWORD dwState);
    BOOL Initialize();
    virtual void Run();
	virtual BOOL OnInit();
    virtual void OnStop();
    virtual void OnInterrogate();
    virtual void OnPause();
    virtual void OnContinue();
    virtual void OnShutdown();
    virtual BOOL OnUserControl(DWORD dwOpcode);
    void DebugMsg(const char* pszFormat, ...);
    
    // static member functions
    static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI Handler(DWORD dwOpcode);

	HRESULT GrantPrivilege(
		 LPCTSTR pszMachineName,
		 LPCTSTR pszUserName,
		 LPCTSTR pszPrivilegeName
		);
    // data members
    char m_szServiceName[64];
	char m_szWorkingDir[_MAX_DIR];
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_Status;
	BOOL m_bIsRunning;

    // static data
    static CNTService* m_pThis; // nasty hack to get object ptr

private:
    HANDLE m_hEventSource;

};

#endif // _NTSERVICE_H_
