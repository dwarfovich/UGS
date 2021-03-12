// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT _WIN32_WINNT_WINXP	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#include <iostream>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

//#include <afx.h>
//#include <afxwin.h>         // MFC core and standard components
//#include <afxext.h>         // MFC extensions
//#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
//#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <afxsock.h>
#include <afxtempl.h>
#include <shlwapi.h>
#include <winsvc.h>

#include "evlogmsg.h" // Event message ids

#include "eventlog.h"

#include "ntservice.h"
#include "ntservapp.h"

#define __VERSION_PREFIX__	_T("UGS")
#define __VERSION_NUMBER__	_T("4.03F")
#define __VERSION_FULL__	__VERSION_PREFIX__ _T("_") __VERSION_NUMBER__

#define VERSION_STRING _T("Version \"") __VERSION_FULL__ _T(" ") _T(__DATE__) _T(" ") _T(__TIME__) _T("\"")

extern LPCTSTR g_lpszVersionStamp;
