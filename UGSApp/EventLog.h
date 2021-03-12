#ifndef _EVENT_LOG_H_
#define _EVENT_LOG_H_

#include <windows.h>

class EventLog {
    HANDLE hEventSource;
    CString appName;
	CString errMsg;
  public:
    EventLog();
    void log(const char* msg, unsigned short type=EVENTLOG_INFORMATION_TYPE, unsigned long id=1);
	const char* winAPIerrorMessage();
};





#endif
