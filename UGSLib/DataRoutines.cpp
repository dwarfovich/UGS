#include "stdafx.h"
#include "DataRoutines.h"

BOOL ByteArrayFromString(const CString & strData, CByteArray & arResult, LPCTSTR lpszPrefix)
{
	CString strTempData(strData);
	strTempData.Delete(0, (int)_tcslen(lpszPrefix));

	int iLength = strTempData.GetLength();
	if((iLength & 1) != 0)
		return FALSE;

	iLength >>= 1;
	CByteArray arTemp;
	arTemp.SetSize(iLength);
	for(int i = 0; i < iLength; i++)
	{
		int iByte = 0, iPos = i << 1;
		CString strTemp;
		strTemp.Format(_T("0x%c%c"), strTempData[iPos], strTempData[iPos + 1]);
		if(!::StrToIntEx(strTemp, STIF_SUPPORT_HEX, &iByte))
			return FALSE;

		arTemp[i] = (BYTE)iByte;
	}

	arResult.Copy(arTemp);
	return TRUE;
}

BOOL ByteArrayFromString(LPCTSTR lpszData, PBYTE pResult, int iResult, LPCTSTR lpszPrefix)
{
	CString strTempData(lpszData);
	strTempData.Delete(0, (int)_tcslen(lpszPrefix));

	int iLength = strTempData.GetLength();
	if((iLength & 1) != 0)
		return FALSE;

	iLength >>= 1;
	static CByteArray arTemp;
	arTemp.SetSize(iLength);
	for(int i = 0; i < iLength; i++)
	{
		int iByte = 0, iPos = i << 1;
		CString strTemp;
		strTemp.Format(_T("0x%c%c"), strTempData[iPos], strTempData[iPos + 1]);
		if(!::StrToIntEx(strTemp, STIF_SUPPORT_HEX, &iByte))
			return FALSE;

		arTemp[i] = (BYTE)iByte;
	}

	pResult = arTemp.GetData();
	iResult = (int)arTemp.GetSize();
	return TRUE;
}

int CalculateIncrements(int * pResult, size_t Length, size_t Size)
{
	int p1 = 1, p2 = 1, p3 = 1;
	
	int i = -1;
	do
	{
		if(++i % 2)
		{
			pResult[i] = (8 * p1 - 6 * p2 + 1) * (int)Size;
		}
		else
		{
			pResult[i] = (9 * p1 - 9 * p3 + 1) * (int)Size;
			p2 <<= 1;
			p3 <<= 1;
		}

		p1 <<= 1;
	} while(3 * pResult[i] < (int)Length);  

	return i > 0 ? --i : 0;
}

void ShellSort(void * pData, int iCount, size_t Size, int (__cdecl *pfnCompare)(const void *elem1, const void *elem2))
{
	int iLength = iCount * (int)Size;

	int iIncrement, arIncrements[40];
	::memset(arIncrements, 0, sizeof(arIncrements));
	// ���������� ������������������ ����������
	int iPosition = CalculateIncrements(arIncrements, iLength, Size);

	void * pBuffer = new BYTE[Size];

	while(iPosition >= 0)
	{
		// ���������� ��������� � ������������ 
		iIncrement = arIncrements[iPosition--];

		for(int i = iIncrement; i < iLength; i += (int)Size)
		{
			::memcpy(pBuffer, (PBYTE)pData + i, Size);
			int j;
			for(j = i - iIncrement; j >= 0; j -= iIncrement)
			{
				void * pCurrent = (PBYTE)pData + j;
				if(pfnCompare(pCurrent, pBuffer) <= 0)
					break;

				::memcpy((PBYTE)pCurrent + iIncrement, pCurrent, Size);
			}

			::memcpy((PBYTE)pData + j + iIncrement, pBuffer, Size);
		}
	}

	delete [] (PBYTE)pBuffer;
}
