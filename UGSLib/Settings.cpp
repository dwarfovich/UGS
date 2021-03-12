#include "stdafx.h"
#include "Settings.h"
#include "ParsingUtils.h"

#include <QtCore/QFile>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

CSettings g_Settings;

CSettings::CSettings()
{
	m_dwPollingPeriod = 1000;
	m_bTestLoopback = FALSE;
	m_bShowSIOMessages = FALSE;
	m_bShowMessageErrors = FALSE;
	m_bShowCOMErrors = FALSE;
	m_strSettingsReportPath = "ugs.rep";

	m_nBufferSize = 0x90000;

	m_nIncomingPort = 11112;

	m_strCOMSetup = "COM1: baud=9600 data=8 parity=N stop=1";
	m_iCOMRttc = 0;
	m_iCOMWttc = 0;
	m_iCOMRit = -1;

	m_arPrefix.clear();

	m_wComposedType = 0x000003;
	m_wOutputComposedType = 0x0000;
	m_wCRC16Init = 0xFFFF;

	m_wCPAddr = 0x0000;
	m_wPUAddr = 0x0000;

	m_bUnpackAll = FALSE;
	m_bMarkAll = FALSE;

	m_nStatusPeriod = 0;
	m_iSendStatTO = 1000000;
	m_StatusHdr = MESSAGETYPE(0x0000, 0x20);
	m_StatusMsg = MESSAGETYPE(m_wComposedType);
	m_MarkNestedMask = MESSAGETYPE();
	m_MarkComposedMask = MESSAGETYPE();
	m_arStatusData.clear();
	MakeStatusMsg();
	UpdateStatusMsg(0);
	m_TUType = 0x000002;
	m_TUSrcMask = 0x0000;
	m_TUSrcComMsgIndex = FALSE;
	m_TUPrimToSecSrc = 1;
	m_TUSecToPrimSrc = 1;

	m_bKeepLog = FALSE;
	m_wLogComposedType = 0x0000;
	m_bLogUnpackAll = FALSE;

	m_wLogComposedTypeToPack = 0x0000;

	m_wSourceID = 0x000020;
	m_wStatusRequestMessageType = 0x0001;

	MESSAGETYPE typeStatus(0x0001, 0x1000);
	m_mapMsgTypes[0x0001] = typeStatus;
}

CSettings::~CSettings()
{
}

void replDef(unsigned short& expl, const unsigned short& def)
{
	expl = expl ? expl : def;
}

QString TstrToQString(LPCTSTR str) {
#ifdef _UNICODE
	return QString::fromWCharArray(str);
#else
	return QString::fromLocal8Bit(str);
#endif // _UNICODE
}

BOOL CSettings::Load(LPCTSTR lpszProfileName)
{
	using parsing::LogHelper;

	auto filename = TstrToQString(lpszProfileName);;
	QFile file { filename };
	bool success = file.open(QIODevice::Text | QIODevice::ReadOnly);
	if (!success) {
		return false;
	}

	QTextStream stream{ &file };
	return Load(stream);
}

BOOL CSettings::Save(LPCTSTR lpszProfileName)
{
	auto filename = TstrToQString(lpszProfileName);;
	QFile file{ filename };
	bool success = file.open(QIODevice::Text | QIODevice::WriteOnly);
	if (!success) {
		return false;
	}

	QTextStream stream{ &file };
	return Save(stream);
}

BOOL CSettings::Load(QTextStream& stream)
{
	using namespace parsing;

	LogHelper log;
	bool opened = log.LoadFromStream(stream);
	if (!opened) {
		return false;
	}

	log.SetCurrentSection("General");
	log.Parse("PollingPeriod", m_dwPollingPeriod);
	log.Parse("TestLoopback", m_bTestLoopback);
	log.Parse("ShowSIOMessages", m_bShowSIOMessages);
	log.Parse("ShowMessageErrors", m_bShowMessageErrors);
	log.Parse("ShowCOMErrors", m_bShowCOMErrors);
	log.Parse("SettingsReportPath", m_strSettingsReportPath);
	log.Parse<16>("BufferSize", m_nBufferSize);

	log.SetCurrentSection("UDP");
	log.Parse("IncomingPort", m_nIncomingPort);
	LoadOutgoingIp(log);

	log.SetCurrentSection("COM");
	log.Parse("SetupParams", m_strCOMSetup);
	log.Parse("rttc", m_iCOMRttc);
	log.Parse("wttc", m_iCOMWttc);
	log.Parse("rit", m_iCOMRit);

	log.SetCurrentSection("Message");
	log.Parse<16>("CPAddr", m_wCPAddr);
	log.Parse<16>("PUAddr", m_wPUAddr);
	log.Parse("Prefix", m_arPrefix);
	log.Parse("OutPrefix", m_arOutPrefix);
	log.Parse("CRC16Init", m_wCRC16Init);
	log.Parse<16>("ComposedType", m_wComposedType);
	log.Parse<16>("OutputComposedType", m_wOutputComposedType);
	LoadTypesToUnpack(log);
	LoadMarkComposedMessageMask(log);
	LoadMarkMessageMask(log);
	LoadTypesToMark(log);
	LoadMessageTypes(log);

	MESSAGETYPE typeStatus(0x0001, 0x1000);
	m_mapMsgTypes[0x0001] = typeStatus;

	log.Parse("StatusPeriod", m_nStatusPeriod);
	log.Parse("SendStatusTO", m_iSendStatTO);

	LoadMessageStatus(log);

	log.Parse<16>("TUType", m_TUType);
	log.Parse("TUSrcComMsgIndex", m_TUSrcComMsgIndex);
	log.Parse("TUPrimToSecSrc", m_TUPrimToSecSrc);
	log.Parse("TUSecToPrimSrc", m_TUSecToPrimSrc);
	log.Parse("TUType", m_TUType);

	log.SetCurrentSection("Log");
	log.Parse("KeepLog", m_bKeepLog);
	LoadLogIp(log);
	log.Parse<16>("LogComposedType", m_wLogComposedType);
	LoadLogTypesToUnPack(log);
	log.Parse<16>("LogComposedTypeToPack", m_wLogComposedTypeToPack);
	LoadLogTypesToPack(log);

	log.SetCurrentSection("Status");
	log.Parse<16>("SourceIndex", m_wSourceID);
	log.Parse<16>("StatusRequestMessageType", m_wStatusRequestMessageType);

	return true;
}

BOOL CSettings::Save(QTextStream& stream)
{
	stream << "[General]\n";
	stream << QString("PollingPeriod = %1\n").arg(m_dwPollingPeriod);
	stream << QString("TestLoopback = %1\n").arg(m_bTestLoopback);
	stream << QString("ShowSIOMessages = %1\n").arg(m_bShowSIOMessages);
	stream << QString("ShowMessageErrors = %1\n").arg(m_bShowMessageErrors);
	stream << QString("ShowCOMErrors = %1\n").arg(m_bShowCOMErrors);
	stream << QString("SettingsReportPath = \"%1\"\n").arg(m_strSettingsReportPath);
	stream << QString("BufferSize = 0x%1\n").arg(QString::number(m_nBufferSize, 16).toUpper());

	stream << "[UDP]\n";
	stream << QString("IncomingPort = %1\n").arg(m_nIncomingPort);

	stream << "[COM]\n";
	stream << QString("SetupParams = \"%1\"\n").arg(m_strCOMSetup);
	stream << QString("rttc = %1\n").arg(m_iCOMRttc);
	stream << QString("wttc = %1\n").arg(m_iCOMWttc);
	stream << QString("rit = %1\n").arg(m_iCOMRit);

	stream << "[Message]\n";
	stream << QString("CPAddr = 0x%1\n").arg(QString{ "%1" }).arg(m_wCPAddr, 4, 16, QChar('0').toUpper());
	stream << QString("PUAddr = 0x%1\n").arg(QString{ "%1" }).arg(m_wPUAddr, 4, 16, QChar('0').toUpper());
	stream << "Prefix = \"" << m_arPrefix.toHex() << "\"\n";
	stream << QString("CRC16Init = 0x%1\n").arg(QString::number(m_wCRC16Init, 16).toUpper());
	stream << QString("ComposedType = 0x%1\n").arg(QString{ "%1" }).arg(m_wComposedType, 4, 16, QChar('0').toUpper());
	stream << QString("OutputComposedType = 0x%1\n").arg(QString{ "%1" }).arg(m_wOutputComposedType, 4, 16, QChar('0').toUpper());
	stream << "TypesToUnPack = \"";
	if (m_bUnpackAll) {
		stream << "*";
	}
	else {
		QString tempStr;
		for (const auto& [wType, pTemp] : m_mapMsgTypesToUnpack) {
			tempStr += QString("0x%1 ").arg(wType, 4, 16, QChar('0').toUpper());
		}
		stream << tempStr.trimmed();
	}
	stream << "\"\n";
	stream << QString("MarkComposedMessageMask = \"0x%1 0x%2\"\n")
		.arg(QString{ "%1" }).arg(m_MarkComposedMask.m_wDestMask, 4, 16, QChar('0').toUpper())
		.arg(QString{ "%1" }).arg(m_MarkComposedMask.m_wSrcMask, 4, 16, QChar('0').toUpper());
	stream << QString("MarkMessageMask = \"0x%1 0x%2\"\n")
		.arg(QString{ "%1" }).arg(m_MarkNestedMask.m_wDestMask, 4, 16, QChar('0').toUpper())
		.arg(QString{ "%1" }).arg(m_MarkNestedMask.m_wSrcMask, 4, 16, QChar('0').toUpper());
	stream << "TypesToMark = \"";
	if (m_bMarkAll) {
		stream << "*";
	}
	else
	{
		QString tempStr;
		for (const auto& [wType, pTemp] : m_mapMsgTypesToMark) {
			tempStr += QString("0x%1 ").arg(wType, 4, 16, QChar('0').toUpper());
		}
		stream << tempStr.trimmed();
	}
	stream << "\"\n";

	int counter = 0;
	for (const auto& [wType, type] : m_mapMsgTypes) {
		stream << QString("Type%1 = \"0x%2 0x%3 0x%4 0x%5 0x%6 0x%7\"\n")
			.arg(counter++)
			.arg(QString("%1").arg(type.m_wType, 4, 16, QChar('0')).toUpper())
			.arg(QString("%1").arg(type.m_wMaxLength, 4, 16, QChar('0')).toUpper())
			.arg(QString("%1").arg(type.m_wDestination, 4, 16, QChar('0')).toUpper())
			.arg(QString("%1").arg(type.m_wSource, 4, 16, QChar('0')).toUpper())
			.arg(QString("%1").arg(type.m_wDestMask, 4, 16, QChar('0')).toUpper())
			.arg(QString("%1").arg(type.m_wSrcMask, 4, 16, QChar('0')).toUpper());
	}

	stream << QString("StatusPeriod = %1\n").arg(m_nStatusPeriod);
	stream << QString("SendStatusTO = %1\n").arg(m_iSendStatTO);
	stream << QString("StatusMsg = \"0x%1 0x%2 0x%3 0x%4 0x%5 0x%6")
		.arg(QString("%1").arg(m_StatusHdr.m_wType, 4, 16, QChar('0')).toUpper())
		.arg(QString("%1").arg(m_StatusHdr.m_wDestination, 4, 16, QChar('0')).toUpper())
		.arg(QString("%1").arg(m_StatusHdr.m_wSource, 4, 16, QChar('0')).toUpper())
		.arg(QString("%1").arg(m_StatusMsg.m_wType, 4, 16, QChar('0')).toUpper())
		.arg(QString("%1").arg(m_StatusMsg.m_wDestination, 4, 16, QChar('0')).toUpper())
		.arg(QString("%1").arg(m_StatusMsg.m_wSource, 4, 16, QChar('0')).toUpper());
	if (!m_arStatusData.isEmpty()) {
		stream << ' ';
	}
	stream << m_arStatusData.toHex().toUpper() << "\"\n";

	stream << QString("TUType = 0x%1\n").arg(QString{ "%1" }).arg(m_TUType, 4, 16, QChar('0').toUpper());
	stream << QString("TUSrcMask = 0x%1\n").arg(QString{ "%1" }).arg(m_TUSrcMask, 4, 16, QChar('0').toUpper());
	stream << QString("TUSrcComMsgIndex = %1\n").arg(m_TUSrcComMsgIndex);
	stream << QString("TUPrimToSecSrc = %1\n").arg(m_TUPrimToSecSrc);
	stream << QString("TUSecToPrimSrc = %1\n").arg(m_TUSecToPrimSrc);
	stream << "OutPrefix = \"" << m_arOutPrefix.toHex().toUpper() << "\"\n";

	stream << "[Log]\n";
	stream << QString("KeepLog = %1\n").arg(m_bKeepLog);
	stream << QString("LogComposedType = 0x%1\n").arg(QString{ "%1" }).arg(m_wLogComposedType, 4, 16, QChar('0').toUpper());
	stream << "LogTypesToUnPack = \"";
	if (m_bLogUnpackAll)
		stream << "*";
	else
	{
		QString tempStr;
		for (const auto& [wType, pTemp] : m_mapLogMsgTypesToUnpack) {
			tempStr += QString("0x%1 ").arg(wType, 4, 16, QChar('0').toUpper());
		}
		stream << tempStr.trimmed();
	}
	stream << "\"\n";

	stream << QString("LogComposedTypeToPack = 0x%1\n").arg(QString{ "%1" }).arg(m_wLogComposedTypeToPack, 4, 16, QChar('0').toUpper());
	stream << "LogTypesToPack = \"";

	if (m_bLogPackAll)
		stream << "*";
	else
	{
		QString tempStr;
		for (const auto& [wType, pTemp] : m_mapLogMsgTypesToPack) {
			tempStr += QString("0x%1 ").arg(wType, 4, 16, QChar('0').toUpper());
		}
		stream << tempStr.trimmed();
	}
	stream << "\"\n";

	stream << "[Status]\n";
	stream << QString("SourceIndex = 0x%1\n").arg(QString{ "%1" }).arg(m_wSourceID, 4, 16, QChar('0').toUpper());
	stream << QString("StatusRequestMessageType = 0x%1\n").arg(QString{ "%1" }).arg(m_wStatusRequestMessageType, 4, 16, QChar('0').toUpper());

	return true;
}

void CSettings::MakeStatusMsg()
{
	m_StatusMsg.m_wMaxLength = 16 + (WORD)m_arStatusData.size();
	m_StatusHdr.m_wMaxLength = 16 + m_StatusMsg.m_wMaxLength;
	m_arStatusMsg.resize(m_StatusHdr.m_wMaxLength);
	BYTE* pData = (BYTE*) m_arStatusMsg.data();
	::ZeroMemory(pData, m_StatusHdr.m_wMaxLength);
	WORD* pHeader = (WORD*)pData;
	pHeader[0] = m_StatusHdr.m_wMaxLength;
	pHeader[1] = m_StatusHdr.m_wType;
	pHeader[2] = m_StatusHdr.m_wDestination;
	pHeader[3] = m_StatusHdr.m_wSource;
	pHeader[7] = m_StatusMsg.m_wMaxLength;
	pHeader[8] = m_StatusMsg.m_wType;
	pHeader[9] = m_StatusMsg.m_wDestination;
	pHeader[10] = m_StatusMsg.m_wSource;
	memcpy(pData + 28, m_arStatusData.data(), m_arStatusData.size());
}

void CSettings::UpdateStatusMsg(unsigned char ind, DWORD timestamp)
{
	BYTE* pData = (BYTE*) m_arStatusMsg.data();
	UINT nLength = (UINT)m_arStatusMsg.size();
	*((DWORD*)(pData + 8)) = *((DWORD*)(pData + 22)) = timestamp;
	pData[12] = ind;
	pData[26] = ind;
	*((WORD*)(pData + nLength - sizeof(DWORD))) = CRC16(pData + 14, nLength - 14 - sizeof(DWORD), m_wCRC16Init);
	*((WORD*)(pData + nLength - sizeof(WORD))) = CRC16(pData, nLength - sizeof(WORD), m_wCRC16Init);
}

void CSettings::LoadTypesToUnpack(const parsing::LogHelper& log)
{
	auto temp = log.Value("Message", "TypesToUnPack");
	if (!temp.isNull()) {
		m_mapMsgTypesToUnpack.clear();
		if (temp == QStringLiteral("*")) {
			m_mapMsgTypesToUnpack[0x0000] = NULL;
			m_bUnpackAll = TRUE;
		}
		else {
			const auto& tokens = temp.split(' ', Qt::SkipEmptyParts);
			for (auto token : tokens) {
				WORD tempValue;
				auto success = parsing::convert<16>(token, tempValue);
				if (success) {
					m_mapMsgTypesToUnpack[tempValue] = NULL;
				}
			}
			m_bUnpackAll = FALSE;
		}
	}
}

void CSettings::LoadMarkComposedMessageMask(const parsing::LogHelper& log)
{
	const auto& tokens = log.Value("Message", "MarkComposedMessageMask").split(' ', Qt::SkipEmptyParts);
	if (tokens.size() >= 1) {
		parsing::convert<16>(tokens[0], m_MarkComposedMask.m_wDestMask);
	}
	if (tokens.size() >= 2) {
		parsing::convert<16>(tokens[1], m_MarkComposedMask.m_wSrcMask);
	}
}

void CSettings::LoadMarkMessageMask(const parsing::LogHelper& log)
{
	const auto& tokens = log.Value("Message", "MarkMessageMask").split(' ', Qt::SkipEmptyParts);
	if (tokens.size() >= 1) {
		parsing::convert<16>(tokens[0], m_MarkNestedMask.m_wDestMask);
	}
	if (tokens.size() >= 2) {
		parsing::convert<16>(tokens[1], m_MarkNestedMask.m_wSrcMask);
	}
}

void CSettings::LoadTypesToMark(const parsing::LogHelper& log)
{
	auto temp = log.Value("Message", "TypesToMark");
	if (!temp.isNull()) {
		m_mapMsgTypesToMark.clear();
		if (temp == QStringLiteral("*")) {
			m_mapMsgTypesToMark[0x0000] = NULL;
			m_bMarkAll = TRUE;
		}
		else {
			const auto& tokens = temp.split(' ', Qt::SkipEmptyParts);
			for (auto token : tokens) {
				WORD temp;
				bool success = parsing::convert<16>(token, temp);
				if (success) {
					m_mapMsgTypesToMark[temp] = NULL;
				}
			}
			m_bMarkAll = FALSE;
		}
	}
}

void CSettings::LoadMessageTypes(const parsing::LogHelper& log)
{
	m_mapMsgTypes.clear();
	for (size_t i = 1; i < 10; ++i) {
		QString expectedValueStr = QString("Type%1").arg(i);
		auto valueStr = log.Value("Message", expectedValueStr);
		if (valueStr.isNull()) {
			continue;
		}

		const auto& tokens = valueStr.split(' ', Qt::SkipEmptyParts);
		static const int messageFieldsCount = 6;
		if (tokens.size() != messageFieldsCount) {
			continue;
		}

		using parsing::convert;
		MESSAGETYPE msgType;
		convert<16>(tokens[0], msgType.m_wType);
		convert<16>(tokens[1], msgType.m_wMaxLength);
		convert<16>(tokens[2], msgType.m_wDestination);
		convert<16>(tokens[3], msgType.m_wSource);
		convert<16>(tokens[4], msgType.m_wDestMask);
		convert<16>(tokens[5], msgType.m_wSrcMask);
		
		if (msgType.m_wType == 0x0505) {
			replDef(msgType.m_wSource, m_wPUAddr);
			replDef(msgType.m_wDestination, m_wCPAddr);
		}
		else if (msgType.m_wType == 0x0521 || msgType.m_wType == 0x0532) {
			replDef(msgType.m_wSource, m_wCPAddr);
		}
		else {
			replDef(msgType.m_wSource, m_wCPAddr);
			replDef(msgType.m_wDestination, m_wPUAddr);
		}
		m_mapMsgTypes[msgType.m_wType] = msgType;
	}
}

void CSettings::LoadMessageStatus(const parsing::LogHelper& log)
{
	auto valueStr = log.Value("Message", "StatusMsg");
	if (valueStr.isNull()) {
		return;
	}
	auto tokens = valueStr.split(' ', Qt::SkipEmptyParts);

	std::vector<std::reference_wrapper<WORD>> targets = { 
		m_StatusHdr.m_wType,
		m_StatusHdr.m_wDestination,
		m_StatusHdr.m_wSource,
		m_StatusMsg.m_wType,
		m_StatusMsg.m_wDestination,
		m_StatusMsg.m_wSource
	};
	
	using parsing::convert;
	for (int i = 0; i < std::min(tokens.size(), 6); ++i) {
		convert<16>(tokens[i], targets[i].get());
	}
	if (tokens.size() == 7) {
		convert(tokens[6], m_arStatusData);
	}

	replDef(m_StatusMsg.m_wSource, m_wCPAddr);
	replDef(m_StatusMsg.m_wDestination, m_wPUAddr);
	
	MakeStatusMsg();
	UpdateStatusMsg(0);
}

void CSettings::LoadLogIp(const parsing::LogHelper& log)
{
	auto valueStr = log.Value("Log", "LogIP");
	if (!valueStr.isNull()) {
		DWORD dwTemp;
		bool success = parsing::convert(valueStr, dwTemp);
		if (!success) {
			return;
		}

		if (INADDR_NONE == dwTemp)
		{
			LPHOSTENT lphost;
			const auto& stdStr = valueStr.toString().toStdString();
			lphost = gethostbyname(stdStr.c_str());
			if (lphost != NULL)
				dwTemp = ((LPIN_ADDR)lphost->h_addr)->s_addr;
			else
			{
				::WSASetLastError(WSAEINVAL);
				dwTemp = (DWORD)ntohl((u_long)INADDR_LOOPBACK);
			}
		}
	}
}

void CSettings::LoadLogTypesToUnPack(const parsing::LogHelper& log)
{
	auto valueStr = log.Value("Log", "LogTypesToUnPack");
	if (valueStr.isNull()) {
		return;
	}

	m_mapLogMsgTypesToUnpack.clear();
	if (valueStr == QStringLiteral("*"))
	{
		m_mapLogMsgTypesToUnpack[0x0000] = NULL;
		m_bLogUnpackAll = TRUE;
	}
	else
	{
		const auto& tokens = valueStr.split(' ', Qt::SkipEmptyParts);
		for (int i = 0; i < tokens.size(); ++i) {
			WORD value;
			bool success = parsing::convert<16>(tokens[i], value);
			if (success) {
				m_mapLogMsgTypesToUnpack[value] = NULL;
			}
		}
		m_bLogUnpackAll = FALSE;
	}
}

void CSettings::LoadLogTypesToPack(const parsing::LogHelper& log)
{
	auto valueStr = log.Value("Log", "LogTypesToPack");
	if (valueStr.isNull()) {
		return;
	}

	m_mapLogMsgTypesToPack.clear();
	if (valueStr == QStringLiteral("*"))
	{
		m_mapLogMsgTypesToPack[0x0000] = NULL;
		m_bLogPackAll = TRUE;
	}
	else
	{
		const auto& tokens = valueStr.split(' ', Qt::SkipEmptyParts);
		for (int i = 0; i < tokens.size(); ++i) {
			WORD value;
			bool success = parsing::convert<16>(tokens[i], value);
			if (success) {
				m_mapLogMsgTypesToPack[value] = NULL;
			}
		}
		m_bLogPackAll = FALSE;
	}
}

void CSettings::LoadOutgoingIp(const parsing::LogHelper& log)
{
	auto valueStr = log.Value("UDP", "OutgoingIP");
	if (valueStr.isNull()) {
		return;
	}

	const auto& stdStr = valueStr.toString().toStdString();
	DWORD dwTemp = (DWORD)inet_addr(stdStr.c_str());
	if (INADDR_NONE == dwTemp)
	{
		LPHOSTENT lphost;
		lphost = gethostbyname(stdStr.c_str());
		if (lphost != NULL)
			dwTemp = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		else
		{
			::WSASetLastError(WSAEINVAL);
			dwTemp = (DWORD)ntohl((u_long)INADDR_LOOPBACK);
		}
	}
}
