#ifndef _DATAROUTINES_H_
#define _DATAROUTINES_H_

#include <windows.h>
#include <tchar.h>

void ChangeByteOrder(char * pData, int iLength);

CString ByteToString(BYTE byByte);
void ByteArrayToString(PBYTE pData, int iLength, CString & strResult, LPCTSTR lpszPrefix = _T(""));

BOOL ByteArrayFromString(const CString & strData, CByteArray & arResult, LPCTSTR lpszPrefix = _T(""));

inline void ChangeByteOrder(char * pData, int iLength)
{
	for(int i = 0; i < iLength - 1; i += 2)
	{
		char temp = pData[i];
		pData[i] = pData[i + 1];
		pData[i + 1] = temp;
	}
}

inline CString ByteToString(BYTE byByte)
{
	CString strResult(_T("00"));
	strResult.Format(_T("%02X"), byByte);
	return strResult;
}

inline void ByteArrayToString(PBYTE pData, int iLength, CString & strResult, LPCTSTR lpszPrefix)
{
	strResult = lpszPrefix;
	for(int i = 0; i < iLength; i++)
		strResult += ByteToString(pData[i]);
}

BOOL ByteArrayFromString(LPCTSTR lpszData, PBYTE pResult, int iResult, LPCTSTR lpszPrefix = _T(""));

void ShellSort(void * pData, int iCount, size_t Size, int (__cdecl *pfnCompare)(const void *elem1, const void *elem2));

#endif // _DATAROUTINES_H_
