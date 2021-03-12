#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <QtCore/QString>

#include <unordered_map>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace parsing {
	class LogHelper;
}

class CSettings
{
public:
	CSettings();
	virtual ~CSettings();

public:
	BOOL Load(LPCTSTR lpszProfileName);
	BOOL Save(LPCTSTR lpszProfileName);
	BOOL Load(QTextStream& stream);
	BOOL Save(QTextStream& stream);
	void MakeStatusMsg();
	void UpdateStatusMsg(unsigned char ind, DWORD timestamp = time(NULL));

private:
	void LoadTypesToUnpack(const parsing::LogHelper& log);
	void LoadMarkComposedMessageMask(const parsing::LogHelper& log);
	void LoadMarkMessageMask(const parsing::LogHelper& log);
	void LoadTypesToMark(const parsing::LogHelper& log);
	void LoadMessageTypes(const parsing::LogHelper& log);
	void LoadMessageStatus(const parsing::LogHelper& log);
	void LoadLogIp(const parsing::LogHelper& log);
	void LoadLogTypesToUnPack(const parsing::LogHelper& log);
	void LoadLogTypesToPack(const parsing::LogHelper& log);
	void LoadOutgoingIp(const parsing::LogHelper& log);

public:
	DWORD m_dwPollingPeriod;
	BOOL m_bTestLoopback;
	BOOL m_bShowSIOMessages;
	BOOL m_bShowMessageErrors;
	BOOL m_bShowCOMErrors;
	QString m_strSettingsReportPath;

	UINT m_nBufferSize;

	QString m_strIncomingAddress;
	UINT m_nIncomingPort;

	QString m_strCOMSetup;
	int m_iCOMRttc;
	int m_iCOMWttc;
	int m_iCOMRit;

	QByteArray m_arPrefix;
	QByteArray m_arOutPrefix;

	WORD m_wComposedType;
	WORD m_wOutputComposedType;
	WORD m_wCRC16Init;
	WORD m_wCPAddr;
	WORD m_wPUAddr;
	typedef struct tagMESSAGETYPE
	{
		WORD m_wType;			//���
		WORD m_wMaxLength;		//max ����� ��������� ��� ����
		WORD m_wDestination;	//����������
		WORD m_wSource;			//�����������
		WORD m_wDestMask;		//����� ���������� (������� ����������)
		WORD m_wSrcMask;		//����� ����������� (������� ����������)
		tagMESSAGETYPE(WORD wType = 0, WORD wMaxLength = 0, WORD wDestination = 0, WORD wSource = 0, WORD wDestMask = 0, WORD wSrcMask = 0)
		{
			m_wType = wType;
			m_wMaxLength = wMaxLength;
			m_wDestination = wDestination;
			m_wSource = wSource;
			m_wDestMask = wDestMask;
			m_wSrcMask = wSrcMask;
		}
	} MESSAGETYPE;

	std::unordered_map<WORD, MESSAGETYPE> m_mapMsgTypes;
	std::unordered_map<WORD, void*> m_mapMsgTypesToUnpack;
	BOOL m_bUnpackAll;
	std::unordered_map<WORD, void*> m_mapMsgTypesToMark;
	BOOL m_bMarkAll;

	UINT m_nStatusPeriod;
	int m_iSendStatTO;
	MESSAGETYPE m_StatusHdr;
	MESSAGETYPE m_StatusMsg;
	MESSAGETYPE m_MarkNestedMask;
	MESSAGETYPE m_MarkComposedMask;
	WORD m_TUType;
	WORD m_TUSrcMask;
	BOOL m_TUSrcComMsgIndex;
	UINT m_TUPrimToSecSrc;
	UINT m_TUSecToPrimSrc;

	QByteArray m_arStatusData;
	QByteArray m_arStatusMsg;

	BOOL m_bKeepLog;
	WORD m_wLogComposedType;
	std::unordered_map<WORD, void*> m_mapLogMsgTypesToUnpack;
	BOOL m_bLogUnpackAll;

	WORD m_wLogComposedTypeToPack;
	std::unordered_map<WORD, void*> m_mapLogMsgTypesToPack;
	BOOL m_bLogPackAll;
	
	//������ ugs
	WORD m_wSourceID;
	WORD m_wStatusRequestMessageType;
};

extern CSettings g_Settings;

#endif // _SETTINGS_H_
