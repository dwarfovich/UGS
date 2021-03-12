#include "pch.h"
#include "../../UGSLib/Settings.h"
#include "../../UGSLib/crc16.h"
#include "../../UGSLib/DataRoutines.h"
#include "../../UGSLib/ParsingUtils.h"

#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#define DEB qDebug()

#include <string>
#include <numeric>
#include <limits>

QT_BEGIN_NAMESPACE
inline void PrintTo(const QString& qString, ::std::ostream* os)
{
	*os << qUtf8Printable(qString);
}
QT_END_NAMESPACE

std::tuple<WORD, WORD, WORD, WORD, WORD, WORD> makeTuple(const CSettings::MESSAGETYPE& m1) {
	return std::tie(m1.m_wType, m1.m_wMaxLength, m1.m_wDestination, m1.m_wSource, m1.m_wDestMask, m1.m_wSrcMask);
}
bool operator== (const CSettings::MESSAGETYPE& m1, const CSettings::MESSAGETYPE& m2) {
	return (makeTuple(m1) == makeTuple(m2));
}

TEST(CSettingsTest, Test_tagMESSAGETYPE_Default_Constructor) {
	CSettings::tagMESSAGETYPE messageType;
	ASSERT_EQ(messageType.m_wType, 0);
	ASSERT_EQ(messageType.m_wMaxLength, 0);
	ASSERT_EQ(messageType.m_wDestination, 0);
	ASSERT_EQ(messageType.m_wSource, 0);
	ASSERT_EQ(messageType.m_wDestMask, 0);
	ASSERT_EQ(messageType.m_wSrcMask, 0);
}

TEST(CSettingsTest, Test_CSettings_Default_Constructor) {
	CSettings s;

	ASSERT_EQ(s.m_dwPollingPeriod, 1000);
	ASSERT_FALSE(s.m_bTestLoopback);
	ASSERT_FALSE(s.m_bShowSIOMessages);
	ASSERT_FALSE(s.m_bShowMessageErrors);
	ASSERT_FALSE(s.m_bShowCOMErrors);
	ASSERT_EQ(s.m_strSettingsReportPath, "ugs.rep");

	ASSERT_EQ(s.m_nBufferSize, 0x90000);
	ASSERT_EQ(s.m_nIncomingPort, 11112);

	ASSERT_EQ(s.m_strCOMSetup, "COM1: baud=9600 data=8 parity=N stop=1");
	ASSERT_EQ(s.m_iCOMRttc, 0);
	ASSERT_EQ(s.m_iCOMWttc, 0);
	ASSERT_EQ(s.m_iCOMRit, -1);

	ASSERT_TRUE(s.m_arPrefix.isEmpty());

	ASSERT_EQ(s.m_wComposedType, 0x000003);
	ASSERT_EQ(s.m_wOutputComposedType, 0x0000);
	ASSERT_EQ(s.m_wCRC16Init, 0xFFFF);

	ASSERT_EQ(s.m_wCPAddr, 0x0000);
	ASSERT_EQ(s.m_wPUAddr, 0x0000);

	ASSERT_FALSE(s.m_bUnpackAll);
	ASSERT_FALSE(s.m_bMarkAll);

	ASSERT_EQ(s.m_nStatusPeriod, 0);
	ASSERT_EQ(s.m_iSendStatTO, 1000000);

	ASSERT_EQ(s.m_StatusHdr, CSettings::MESSAGETYPE(0x0000, 0x20));
	ASSERT_EQ(s.m_MarkNestedMask, CSettings::MESSAGETYPE());
	ASSERT_EQ(s.m_MarkComposedMask, CSettings::MESSAGETYPE());
	ASSERT_TRUE(s.m_arStatusData.isEmpty());

	ASSERT_EQ(s.m_TUType, 0x000002);
	ASSERT_EQ(s.m_TUSrcMask, 0x0000);
	ASSERT_FALSE(s.m_TUSrcComMsgIndex);
	ASSERT_EQ(s.m_TUPrimToSecSrc, 1);
	ASSERT_EQ(s.m_TUSecToPrimSrc, 1);

	ASSERT_FALSE(s.m_bKeepLog);
	ASSERT_FALSE(s.m_bLogUnpackAll);

	ASSERT_EQ(s.m_wLogComposedTypeToPack, 0x0000);

	ASSERT_EQ(s.m_wSourceID, 0x000020);
	ASSERT_EQ(s.m_wStatusRequestMessageType, 0x0001);

	ASSERT_EQ(s.m_mapMsgTypes.size(), 1);
	ASSERT_EQ(s.m_StatusMsg, CSettings::MESSAGETYPE(s.m_wComposedType, 16));
}

bool operator== (const CByteArray& a1, const CByteArray& a2) {
	if (a1.GetSize() != a2.GetSize()) {
		return false;
	}
	else {
		return (memcmp(a1.GetData(), a2.GetData(), a1.GetSize()) == 0);
	}
}

void CopyByteArray(CByteArray& dest, const CByteArray& src) {
	dest.SetSize(src.GetSize());
	memcpy_s(dest.GetData(), dest.GetSize(), src.GetData(), src.GetSize());
}

using ByteVector = std::vector<BYTE>;

void fillByteArray(QByteArray& a, const ByteVector& data) {
	a.resize(data.size());
	memcpy_s(a.data(), a.size(), data.data(), data.size());
}

using MessageType = CSettings::MESSAGETYPE;

static int iStatusCaseIndex = 0;
struct StatusCase {
	MessageType statusHdr;
	MessageType statusMsg;
	ByteVector statusData;
	int iCaseIndex = iStatusCaseIndex++;
};

struct MakeStatusMsgCase {
	MakeStatusMsgCase(CSettings& a_s, QByteArray& a_arExpectedStatusMsg,
		const MessageType& a_statusHdr, const MessageType& a_statusMsg, const ByteVector& a_statusData)
		: s{ a_s }
		, arExpectedStatusMsg{ a_arExpectedStatusMsg }
		, statusHdr{ a_statusHdr }
		, statusMsg{ a_statusMsg }
	{
		fillByteArray(arStatusData, a_statusData);
	}

	MakeStatusMsgCase(CSettings& a_s, QByteArray& a_arExpectedStatusMsg, const StatusCase& statusCase)
		: s{ a_s }
		, arExpectedStatusMsg{ a_arExpectedStatusMsg }
		, statusHdr{ statusCase.statusHdr }
		, statusMsg{ statusCase.statusMsg }
	{
		fillByteArray(arStatusData, statusCase.statusData);
	}

	CSettings& s;
	QByteArray& arExpectedStatusMsg;
	MessageType statusHdr;
	MessageType statusMsg;
	QByteArray arStatusData;
};

static const WORD wStatusMsgMemoryOffset = 16;

void generateMakeStatusMsgCase(CSettings& a_s, QByteArray& a_arExpectedStatusMsg, const StatusCase& statusCase) {
	MakeStatusMsgCase caseData(a_s, a_arExpectedStatusMsg, statusCase);

	// Generate CSettings' status message
	caseData.s.m_arStatusData = caseData.arStatusData;
	caseData.s.m_StatusMsg = caseData.statusMsg;
	caseData.s.m_StatusHdr = caseData.statusHdr;
	caseData.s.MakeStatusMsg();

	// Generate expected status message
	caseData.statusMsg.m_wMaxLength = wStatusMsgMemoryOffset + caseData.arStatusData.size();
	caseData.statusHdr.m_wMaxLength = wStatusMsgMemoryOffset + caseData.statusMsg.m_wMaxLength;
	caseData.arExpectedStatusMsg.resize(caseData.statusHdr.m_wMaxLength);

	BYTE* pData = (BYTE*) caseData.arExpectedStatusMsg.data();
	::ZeroMemory(pData, caseData.statusHdr.m_wMaxLength);
	WORD* pHeader = (WORD*)pData;
	const auto& statusHdr = caseData.statusHdr;
	pHeader[0] = statusHdr.m_wMaxLength;
	pHeader[1] = statusHdr.m_wType;
	pHeader[2] = statusHdr.m_wDestination;
	pHeader[3] = statusHdr.m_wSource;
	const auto& statusMsg = caseData.statusMsg;
	pHeader[7] = statusMsg.m_wMaxLength;
	pHeader[8] = statusMsg.m_wType;
	pHeader[9] = statusMsg.m_wDestination;
	pHeader[10] = statusMsg.m_wSource;
	static const WORD wStatusDataOffset = 28;
	memcpy(pData + wStatusDataOffset, caseData.arStatusData.data(), caseData.arStatusData.size());
}

::testing::AssertionResult StatusMessagesGeneratedAreEqual(const StatusCase& statusCase) {
	CSettings s;
	QByteArray arExpectedStatusMessage;
	generateMakeStatusMsgCase(s, arExpectedStatusMessage, statusCase);

	if (s.m_arStatusMsg == arExpectedStatusMessage) {
		return ::testing::AssertionSuccess();
	}
	else {
		return ::testing::AssertionFailure() << "Status messages are not equal for case index "
			<< statusCase.iCaseIndex << "\n";
	}
}

std::vector<StatusCase> statusCases{
	{{}, {}, {}},
	{{ 1 }, {}, {}},
	{{ 1,2,3,4,5,6 }, {}, {}},
	{{ 1 }, { 1 }, {}},
	{{}, { 1,2,3,4,5,6 }, {}},
	{{ 1,2,3,4,5,6 }, { 10,20,30,40,50,60 }, {}},
	{{ }, { }, { 1 }},
	{{ }, { }, {1,2,3,4,5}},
	{{ }, { }, { 1,2,3,4,5,6,7,8,90, 150, 255 }},
	{{ 1 }, { 2 }, { 1,2,3,4,5,6,7,8,9 }},
	{{ 1,2 }, { 2,3 }, { 1,2,3,4,5,6,7,8,9 }},
	{{ 1,2,10 }, { 2,3 }, { 1,2,3,4,5,6,7,8,90, 150, 255 }},
	{{ 1,2 }, { 2,3 }, { 1,2,3,4,5,6,7,8,90, 150, 255 }},
	{{ 10,20,30,40 }, { 2,3,4,5 }, { 1,2,3,4,5,6,7,8,90, 150, 255 }},
};

TEST(CSettingsTest, Test_MakeStatusMsg) {
	// Add long data.
	// Undefining windows "max" macro to enable numeric_limits::max().
#undef max
	using win_word_t = unsigned __int16;
	ByteVector v(std::numeric_limits<win_word_t>::max() - wStatusMsgMemoryOffset * 2);
	std::iota(v.begin(), v.end(), 0);
	statusCases.push_back({ { 10,20,30,40 }, { 2,3,4,5 }, std::move(v) });

	for (const auto& statusCase : statusCases) {
		EXPECT_TRUE(StatusMessagesGeneratedAreEqual(statusCase));
	}
}

::testing::AssertionResult UpdatedStatusMessagesAreEqual(const StatusCase& statusCase, unsigned char ind, DWORD timestamp) {
	CSettings s;
	QByteArray arExpectedStatusMessage;
	generateMakeStatusMsgCase(s, arExpectedStatusMessage, statusCase);
	// Update in CSettings
	s.UpdateStatusMsg(ind, timestamp);

	// Update in expected array
	BYTE* pData = (BYTE*) arExpectedStatusMessage.data();
	UINT nLength = (UINT)arExpectedStatusMessage.size();
	*((DWORD*)(pData + 8)) = *((DWORD*)(pData + 22)) = timestamp;
	pData[12] = ind;
	pData[26] = ind;
	*((WORD*)(pData + nLength - sizeof(DWORD))) = CRC16(pData + 14, nLength - 14 - sizeof(DWORD), s.m_wCRC16Init);
	*((WORD*)(pData + nLength - sizeof(WORD))) = CRC16(pData, nLength - sizeof(WORD), s.m_wCRC16Init);

	if (s.m_arStatusMsg == arExpectedStatusMessage) {
		return ::testing::AssertionSuccess();
	}
	else {
		return ::testing::AssertionFailure() << "Updated status messages are not equal for case index "
			<< statusCase.iCaseIndex << "; ind = " << ind << "; timestamp = " << timestamp << "\n";
	}
}

TEST(CSettingsTest, Test_UpdateStatusMsg) {
	const std::vector<unsigned char> indCases = { 0, 1, 100, 200, 255 };
	for (const auto& statusCase : statusCases) {
		for (unsigned char ind : indCases) {
			EXPECT_TRUE(UpdatedStatusMessagesAreEqual(statusCase, ind, 0));
		}
	}

	const std::vector<DWORD> timestampCases = { 0, 1, 10000, std::numeric_limits<DWORD>::max() };
	for (const auto& statusCase : statusCases) {
		for (unsigned char timestamp : timestampCases) {
			EXPECT_TRUE(UpdatedStatusMessagesAreEqual(statusCase, 0, timestamp));
		}
	}
}

class Checker {
public:
	bool Load(QTextStream& stream) {
		return m_log.LoadFromStream(stream);
	}

	void SetCurrentSection(const QString& section) {
		m_currentSection = section;
	}

	::testing::AssertionResult Check(const QString& option, const QString& expectedValue) {
		auto value = m_log.Value(m_currentSection, option);
		if (value.isNull()) {
			return ::testing::AssertionFailure() << "No option " << option.toStdString()
				<< " in section " << m_currentSection.toStdString() << '\n';
		}

		if (value == expectedValue) {
			return ::testing::AssertionSuccess();
		}
		else {
			return ::testing::AssertionFailure() << "Wrong value for option " << option.toStdString() << ": " << value.toString().toStdString();
		}
	}

	::testing::AssertionResult Check(const QString& section, const QString& option, const QString& expectedValue) {
		SetCurrentSection(section);
		return Check(option, expectedValue);
	}

private:
	parsing::LogHelper m_log;
	QString m_currentSection;
};

TEST(CSettingsTest, Test_Save_For_Default_And_Completness) {
	
	QString filename = "TestData/default_save.log";
	QFile file{ filename };
	ASSERT_TRUE(file.open(QIODevice::Text | QIODevice::ReadWrite | QIODevice::Truncate));
	QTextStream saveStream (&file);
	CSettings s;
	ASSERT_TRUE(s.Save(saveStream));
	saveStream.seek(0);

	Checker checker;
	ASSERT_TRUE(checker.Load(saveStream));

	checker.SetCurrentSection("General");
	ASSERT_TRUE(checker.Check("PollingPeriod", "1000"));
	ASSERT_TRUE(checker.Check("ShowSIOMessages", "0"));
	ASSERT_TRUE(checker.Check("ShowMessageErrors", "0"));
	ASSERT_TRUE(checker.Check("SettingsReportPath", "ugs.rep"));
	ASSERT_TRUE(checker.Check("BufferSize", "0x90000"));

	checker.SetCurrentSection("UDP");
	ASSERT_TRUE(checker.Check("IncomingPort", "11112"));

	checker.SetCurrentSection("COM");
	ASSERT_TRUE(checker.Check("SetupParams", "COM1: baud=9600 data=8 parity=N stop=1"));
	ASSERT_TRUE(checker.Check("rttc", "0"));
	ASSERT_TRUE(checker.Check("wttc", "0"));
	ASSERT_TRUE(checker.Check("rit", "-1"));

	checker.SetCurrentSection("Message");
	ASSERT_TRUE(checker.Check("CPAddr", "0x0000"));
	ASSERT_TRUE(checker.Check("PUAddr", "0x0000"));
	ASSERT_TRUE(checker.Check("Prefix", ""));
	ASSERT_TRUE(checker.Check("CRC16Init", "0xFFFF"));
	ASSERT_TRUE(checker.Check("ComposedType", "0x0003"));
	ASSERT_TRUE(checker.Check("OutputComposedType", "0x0000"));
	ASSERT_TRUE(checker.Check("TypesToUnPack", ""));
	ASSERT_TRUE(checker.Check("MarkComposedMessageMask", "0x0000 0x0000"));
	ASSERT_TRUE(checker.Check("MarkMessageMask", "0x0000 0x0000"));
	ASSERT_TRUE(checker.Check("TypesToMark", ""));
	ASSERT_TRUE(checker.Check("Type0", "0x0001 0x1000 0x0000 0x0000 0x0000 0x0000"));
	ASSERT_TRUE(checker.Check("StatusPeriod", "0"));
	ASSERT_TRUE(checker.Check("SendStatusTO", "1000000"));
	ASSERT_TRUE(checker.Check("StatusMsg", "0x0000 0x0000 0x0000 0x0003 0x0000 0x0000"));
	ASSERT_TRUE(checker.Check("TUType", "0x0002"));
	ASSERT_TRUE(checker.Check("TUSrcMask", "0x0000"));
	ASSERT_TRUE(checker.Check("TUSrcComMsgIndex", "0"));
	ASSERT_TRUE(checker.Check("TUPrimToSecSrc", "1"));
	ASSERT_TRUE(checker.Check("TUSecToPrimSrc", "1"));
	ASSERT_TRUE(checker.Check("OutPrefix", ""));

	checker.SetCurrentSection("Log");
	ASSERT_TRUE(checker.Check("KeepLog", "0"));
	ASSERT_TRUE(checker.Check("KeepLog", "0"));
	ASSERT_TRUE(checker.Check("LogTypesToUnPack", ""));
	ASSERT_TRUE(checker.Check("LogComposedTypeToPack", "0x0000"));
	ASSERT_TRUE(checker.Check("LogTypesToPack", "*"));

	checker.SetCurrentSection("Status");
	ASSERT_TRUE(checker.Check("SourceIndex", "0x0020"));
	ASSERT_TRUE(checker.Check("StatusRequestMessageType", "0x0001"));
}

TEST(CSettingsTest, Test_Save_Msg_Type_To_Unpack) {
	QString str;
	QTextStream stream (&str);
	CSettings s;
	s.m_bUnpackAll = false;
	s.m_mapMsgTypesToUnpack[0x0000] = NULL;
	s.m_mapMsgTypesToUnpack[0x0010] = NULL;
	s.m_mapMsgTypesToUnpack[0x1200] = NULL;
	s.m_mapMsgTypesToUnpack[0x9999] = NULL;
	s.Save(stream);
	stream.seek(0);
	
	Checker checker;
	checker.Load(stream);
	ASSERT_TRUE(checker.Check("Message", "TypesToUnPack", "0x0010 0x0000 0x9999 0x1200"));
}

TEST(CSettingsTest, Test_Save_Types_To_Mark) {
	QString str;
	QTextStream stream(&str);
	CSettings s;
	s.m_bMarkAll = false;
	s.m_mapMsgTypesToMark[0x0000] = NULL;
	s.m_mapMsgTypesToMark[0x0010] = NULL;
	s.m_mapMsgTypesToMark[0x1200] = NULL;
	s.m_mapMsgTypesToMark[0x9999] = NULL;
	s.Save(stream);
	stream.seek(0);

	Checker checker;
	checker.Load(stream);
	ASSERT_TRUE(checker.Check("Message", "TypesToMark", "0x0010 0x0000 0x9999 0x1200"));
}

TEST(CSettingsTest, Test_Save_Message_Types) {
	QString str;
	QTextStream stream(&str);
	CSettings s;
	s.m_mapMsgTypes[0x0000] = {0, 1, 2, 3, 4, 5};
	s.m_mapMsgTypes[0x0010] = {0x0010, 0x2000, 0x3003, 0x1919, 0, 0};
	s.m_mapMsgTypes[0x1200] = {0x1200, 1, 2, 3, 4, 5 };
	s.m_mapMsgTypes[0x9999] = {0x9999, 1, 2, 3, 4, 5 };
	s.Save(stream);
	stream.seek(0);

	Checker checker;
	checker.Load(stream);
	ASSERT_TRUE(checker.Check("Message", "Type1", "0x0010 0x2000 0x3003 0x1919 0x0000 0x0000"));
	ASSERT_TRUE(checker.Check("Message", "Type2", "0x0000 0x0001 0x0002 0x0003 0x0004 0x0005"));
	ASSERT_TRUE(checker.Check("Message", "Type3", "0x9999 0x0001 0x0002 0x0003 0x0004 0x0005"));
	ASSERT_TRUE(checker.Check("Message", "Type4", "0x1200 0x0001 0x0002 0x0003 0x0004 0x0005"));
}

TEST(CSettingsTest, Test_Save_Status_Message) {
	QString str;
	QTextStream stream(&str);
	CSettings s;
	s.m_StatusHdr.m_wType = 1;
	s.m_StatusHdr.m_wDestination = 2;
	s.m_StatusHdr.m_wSource = 3;
	s.m_StatusMsg.m_wType = 4;
	s.m_StatusMsg.m_wDestination = 5;
	s.m_StatusMsg.m_wSource = 6;
	s.m_arStatusData = QByteArray::fromHex("1011121314aaff");
	s.Save(stream);
	stream.seek(0);

	Checker checker;
	checker.Load(stream);
	ASSERT_TRUE(checker.Check("Message", "StatusMsg",
		"0x0001 0x0002 0x0003 0x0004 0x0005 0x0006 1011121314AAFF"));
}

TEST(CSettingsTest, Test_Save_Out_Prefix) {
	QString str;
	QTextStream stream(&str);
	CSettings s;
	s.m_arOutPrefix = QByteArray::fromHex("1011121314aaff");
	s.Save(stream);
	stream.seek(0);

	Checker checker;
	checker.Load(stream);
	ASSERT_TRUE(checker.Check("Message", "OutPrefix", "1011121314AAFF"));
}

TEST(CSettingsTest, Test_Load) {
	CSettings s;
	std::string filename = "TestData\\load.log";
	ASSERT_TRUE(s.Load(filename.data()));

	ASSERT_EQ(s.m_dwPollingPeriod, 1001);
	ASSERT_TRUE(s.m_bTestLoopback);
	ASSERT_EQ(s.m_strSettingsReportPath, QString("ugs-test.rep"));
	ASSERT_EQ(s.m_nBufferSize, 0x90009);
	ASSERT_EQ(s.m_iCOMRttc, 100);
	ASSERT_EQ(s.m_wCPAddr, 0x0044);
	ASSERT_FALSE(s.m_arPrefix.isEmpty());
	ASSERT_EQ(s.m_arPrefix, QByteArray::fromHex("000102030405aaff"));
	ASSERT_EQ(s.m_MarkComposedMask.m_wDestMask, 0x0001);
	ASSERT_EQ(s.m_MarkComposedMask.m_wSrcMask, 0x0002);
}

TEST(CSettingsTest, Test_Skipping_For_Wrong_Values) {
	CSettings s;
	std::string filename = "TestData\\load_skipping.log";
	ASSERT_TRUE(s.Load(filename.c_str()));

	ASSERT_EQ(s.m_dwPollingPeriod, 1000);
	ASSERT_EQ(s.m_nBufferSize, 0x90000);
	ASSERT_TRUE(s.m_arPrefix.isEmpty());
}

TEST(CSettingsTest, Test_Load_Types_To_Unpack_Star) {
	QString str = {
		"[Message]\n"
		"TypesToUnPack = \"*\""
	};
	QTextStream stream(&str);
	CSettings s;
	s.m_mapMsgTypesToUnpack[1] = NULL;
	s.m_mapMsgTypesToUnpack[2] = NULL;
	s.m_mapMsgTypesToUnpack[0xffff] = NULL;
	s.Load(stream);

	ASSERT_EQ(s.m_bUnpackAll, TRUE);
	ASSERT_TRUE(s.m_mapMsgTypesToUnpack.size() == 1);
	ASSERT_EQ(s.m_mapMsgTypesToUnpack.begin()->first, 0);
}

TEST(CSettingsTest, Test_Load_Types_To_Unpack_Without_Star) {
	QString str = {
		"[Message]\n"
		"TypesToUnPack = \"0x0001 0x0002 0x0003 0xAAAA 0xFFFF\""
	};
	QTextStream stream(&str);
	CSettings s;
	s.m_mapMsgTypesToUnpack[4] = NULL;
	s.m_mapMsgTypesToUnpack[5] = NULL;
	s.m_mapMsgTypesToUnpack[6] = NULL;
	s.Load(stream);

	ASSERT_EQ(s.m_bUnpackAll, FALSE);
	ASSERT_TRUE(s.m_mapMsgTypesToUnpack.size() == 5);
	ASSERT_TRUE(s.m_mapMsgTypesToUnpack.find(0x0001) != s.m_mapMsgTypesToUnpack.cend());
	ASSERT_TRUE(s.m_mapMsgTypesToUnpack.find(0x0002) != s.m_mapMsgTypesToUnpack.cend());
	ASSERT_TRUE(s.m_mapMsgTypesToUnpack.find(0x0003) != s.m_mapMsgTypesToUnpack.cend());
	ASSERT_TRUE(s.m_mapMsgTypesToUnpack.find(0xAAAA) != s.m_mapMsgTypesToUnpack.cend());
	ASSERT_TRUE(s.m_mapMsgTypesToUnpack.find(0xFFFF) != s.m_mapMsgTypesToUnpack.cend());
}

TEST(CSettingsTest, Test_Load_Mark_Composed_Message_Mask) {
	QString str = {
		"[Message]\n"
		"MarkComposedMessageMask = \"0x0001 0x0002\""
	};
	QTextStream stream(&str);
	CSettings s;
	s.m_mapMsgTypesToUnpack[4] = NULL;
	s.m_mapMsgTypesToUnpack[5] = NULL;
	s.m_mapMsgTypesToUnpack[6] = NULL;
	s.Load(stream);

	ASSERT_TRUE(s.m_MarkComposedMask.m_wDestMask == 1);
	ASSERT_TRUE(s.m_MarkComposedMask.m_wSrcMask == 2);
}

TEST(CSettingsTest, Test_Load_Mark_Composed_Message_Mask_Star) {
	QString str = {
		"[Message]\n"
		"TypesToMark = \"*\""
	};
	QTextStream stream(&str);
	CSettings s;
	s.m_mapMsgTypesToMark[4] = NULL;
	s.m_mapMsgTypesToMark[5] = NULL;
	s.m_mapMsgTypesToMark[6] = NULL;
	s.Load(stream);

	ASSERT_EQ(s.m_bMarkAll, TRUE);
	ASSERT_TRUE(s.m_mapMsgTypesToMark.size() == 1);
	ASSERT_EQ(s.m_mapMsgTypesToMark.begin()->first, 0);
}

TEST(CSettingsTest, Test_Load_Mark_Composed_Message_Mask_Without_Star) {
	QString str = {
		"[Message]\n"
		"TypesToMark = \"0x0001 0x0002 0xAAAA 0xFFFF\""
	};
	QTextStream stream(&str);
	CSettings s;
	s.m_mapMsgTypesToMark[4] = NULL;
	s.m_mapMsgTypesToMark[5] = NULL;
	s.m_mapMsgTypesToMark[6] = NULL;
	s.Load(stream);

	ASSERT_EQ(s.m_bMarkAll, FALSE);
	ASSERT_TRUE(s.m_mapMsgTypesToMark.size() == 4);
	ASSERT_TRUE(s.m_mapMsgTypesToMark.find(0x0001) != s.m_mapMsgTypesToMark.cend());
	ASSERT_TRUE(s.m_mapMsgTypesToMark.find(0x0002) != s.m_mapMsgTypesToMark.cend());
	ASSERT_TRUE(s.m_mapMsgTypesToMark.find(0xAAAA) != s.m_mapMsgTypesToMark.cend());
	ASSERT_TRUE(s.m_mapMsgTypesToMark.find(0xFFFF) != s.m_mapMsgTypesToMark.cend());
}

TEST(CSettingsTest, Test_Load_Message_Types) {
	QString str = {
		"[Message]\n"
		"Type1 = \"0x0001 0x0002 0x0003 0x0004 0xAAAA 0xFFFF\"\n"
		"Type2 = \"0x0002 0x0002 0x0003 0x0004 0xAAAA 0xFFFF\"\n"
		"Type3 = \"0x0003 0x0002 0x0003 0x0004 0xAAAA 0xFFFF\"\n"
		"Type9 = \"0x0009 0x0002 0x0003 0x0004 0xAAAA 0xFFFF\"\n"
	};
	QTextStream stream(&str);
	CSettings s;
	s.m_mapMsgTypes[0xaa] = {};
	s.m_mapMsgTypes[0xbb] = {};
	s.m_mapMsgTypes[0xcc] = {};
	s.Load(stream);

	using MsgType = CSettings::MESSAGETYPE;
	ASSERT_EQ(s.m_mapMsgTypes.size(), 4);

	ASSERT_TRUE(s.m_mapMsgTypes.find(0x0001) != s.m_mapMsgTypes.cend());
	ASSERT_EQ(s.m_mapMsgTypes[0x0001], MsgType(0x0001, 0x1000));

	ASSERT_TRUE(s.m_mapMsgTypes.find(0x0002) != s.m_mapMsgTypes.cend());
	ASSERT_EQ(s.m_mapMsgTypes[0x0002], MsgType(0x0002, 2, 3, 4, 0xaaaa, 0xffff));

	ASSERT_TRUE(s.m_mapMsgTypes.find(0x0003) != s.m_mapMsgTypes.cend());
	ASSERT_EQ(s.m_mapMsgTypes[0x0003], MsgType(0x0003, 2, 3, 4, 0xaaaa, 0xffff));

	ASSERT_TRUE(s.m_mapMsgTypes.find(0x0009) != s.m_mapMsgTypes.cend());
	ASSERT_EQ(s.m_mapMsgTypes[0x0009], MsgType(0x0009, 2, 3, 4, 0xaaaa, 0xffff));
}

TEST(CSettingsTest, Test_Load_Message_Status) {
	QString str = {
		"[Message]\n"
		"StatusMsg = \"0x0001 0x0002 0x0003 0x0004 0x0005 0x0006 1122AAFF\"\n"
	};
	QTextStream stream(&str);
	CSettings s;
	s.Load(stream);

	ASSERT_EQ(s.m_StatusHdr.m_wType, 1);
	ASSERT_EQ(s.m_StatusHdr.m_wDestination, 2);
	ASSERT_EQ(s.m_StatusHdr.m_wSource, 3);
	ASSERT_EQ(s.m_StatusMsg.m_wType, 4);
	ASSERT_EQ(s.m_StatusMsg.m_wDestination, 5);
	ASSERT_EQ(s.m_StatusMsg.m_wSource, 6);
	ASSERT_EQ(s.m_arStatusData, QByteArray::fromHex("1122AAFF"));
}

TEST(CSettingsTest, Test_Load_Log_Types_To_Unpack) {
	QString str = {
		"[Log]\n"
		"LogTypesToUnPack = \"*\"\n"
	};
	QTextStream stream(&str);
	CSettings s;
	s.Load(stream);

	ASSERT_EQ(s.m_mapLogMsgTypesToUnpack.size(), 1);
	ASSERT_EQ(s.m_bLogUnpackAll, TRUE);
	ASSERT_EQ(s.m_mapLogMsgTypesToUnpack.begin()->first, 0);

	str = {
		"[Log]\n"
		"LogTypesToUnPack = \"0x0001 0xFFFF\"\n"
	};
	stream.setString(&str);
	s.Load(stream);

	ASSERT_EQ(s.m_mapLogMsgTypesToUnpack.size(), 2);
	ASSERT_EQ(s.m_bLogUnpackAll, FALSE);
	ASSERT_TRUE(s.m_mapLogMsgTypesToUnpack.find(1) != s.m_mapLogMsgTypesToUnpack.cend());
	ASSERT_TRUE(s.m_mapLogMsgTypesToUnpack.find(0xFFFF) != s.m_mapLogMsgTypesToUnpack.cend());
}

TEST(CSettingsTest, Test_Load_Log_Types_To_Pack) {
	QString str = {
		"[Log]\n"
		"LogTypesToPack = \"*\"\n"
	};
	QTextStream stream(&str);
	CSettings s;
	s.Load(stream);

	ASSERT_EQ(s.m_mapLogMsgTypesToPack.size(), 1);
	ASSERT_EQ(s.m_bLogPackAll, TRUE);
	ASSERT_EQ(s.m_mapLogMsgTypesToPack.begin()->first, 0);

	str = {
		"[Log]\n"
		"LogTypesToPack = \"0x0001 0xFFFF\"\n"
	};
	stream.setString(&str);
	s.Load(stream);

	ASSERT_EQ(s.m_mapLogMsgTypesToPack.size(), 2);
	ASSERT_EQ(s.m_bLogPackAll, FALSE);
	ASSERT_TRUE(s.m_mapLogMsgTypesToPack.find(1) != s.m_mapLogMsgTypesToPack.cend());
	ASSERT_TRUE(s.m_mapLogMsgTypesToPack.find(0xFFFF) != s.m_mapLogMsgTypesToPack.cend());
}