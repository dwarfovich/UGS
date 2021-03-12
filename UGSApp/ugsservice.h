#include "ntservice.h"

class CUGSService : public CNTService
{
public:
	CUGSService(const char * szServiceName);
	virtual BOOL OnInit();
    virtual void Run();
    virtual BOOL OnUserControl(DWORD dwOpcode);

    void SaveStatus();

	// Control parameters
	int m_iStartParam;

	// Current state
	int m_iState;
};
