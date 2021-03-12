// crc16.h
////////////////////////////////////////////////////////////////////////
#ifndef _CRC16_H_
#define _CRC16_H_

#include <windows.h>
#include <tchar.h>

WORD CRC16(PVOID pData, ULONG nLength, WORD wCRC16 = (WORD)-1);

#endif // _CRC16_H_
