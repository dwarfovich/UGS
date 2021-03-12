// NTService.cpp
// 
// This is the main program file containing the entry point.

#include "stdafx.h"
#include "NTServApp.h"
#include "UGSservice.h"

CUGSService *UGSService;
CNTService * g_pNTService;

int main(int argc, char* argv[])
{
	CHAR szFileTitle[_MAX_PATH];
	::GetFileTitle(argv[0],szFileTitle,_MAX_PATH);
	// ������� ���������� �� ����� �������, �.�. ���� ���������� �������� "�� ���������� ����������
	// ��� ������������������ ����� ������", ���������� �� ����� � ������ szFileTitle, � ���� ����
	// �������� �� ����������, �� ��� ����� ����� ������������ �������� GetFileTitle() � �����������.
	_tcslwr_s(szFileTitle, _MAX_PATH);
	char *pExt = _tcsstr(szFileTitle, ".exe");
	if (pExt != NULL)
		*pExt = 0;

	UGSService = new CUGSService(szFileTitle);
	g_pNTService = UGSService;

    // Create the service object
    if ((argc > 1)&&(_stricmp(argv[1], "-start") == 0)) {

		UGSService->m_bIsRunning = TRUE;
		UGSService->RegEventMessagesFile();
		UGSService->LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STARTED);
		UGSService->Run();
		UGSService->LogEvent(EVENTLOG_INFORMATION_TYPE, EVMSG_STOPPED);
    } 
   
    // Parse for standard arguments (install, uninstall, version etc.)
    if (!UGSService->ParseStandardArgs(argc, argv)) {

        // Didn't find any standard args so start the service
        // Uncomment the DebugBreak line below to enter the debugger
        // when the service is started.
        //DebugBreak();
        UGSService->StartService();
    }

    // When we get here, the service has been stopped
    return UGSService->m_Status.dwWin32ExitCode;
}
