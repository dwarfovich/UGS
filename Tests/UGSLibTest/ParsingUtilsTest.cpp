#include "pch.h"
#include "gtest/gtest.h"
#include "../../UGSLib/ParsingUtils.h"

using namespace parsing;

TEST(ParsingUtilsTest, TestConvertToChar)
{
	char testing = 0;
	// Base 10
	ASSERT_TRUE(convert(QString("1"), testing));
	ASSERT_EQ(testing, 1);
	ASSERT_TRUE(convert(QString("127"), testing));
	ASSERT_EQ(testing, 127);
	ASSERT_TRUE(convert(QString("-128"), testing));
	ASSERT_EQ(testing, -128);
	// Base 16
	ASSERT_TRUE(convert<16>(QString("0x1"), testing));
	ASSERT_EQ(testing, 1);
	ASSERT_TRUE(convert<16>(QString("0x7F"), testing));
	ASSERT_EQ(testing, 127);
	ASSERT_TRUE(convert<16>(QString("-0x80"), testing));
	ASSERT_EQ(testing, -128);
	// Additional whitespaces
	ASSERT_TRUE(convert(QString("1  "), testing));
	ASSERT_EQ(testing, 1);
	ASSERT_TRUE(convert(QString("   127"), testing));
	ASSERT_EQ(testing, 127);
	ASSERT_TRUE(convert(QString("   -128    "), testing));
	ASSERT_EQ(testing, -128);
}

TEST(ParsingUtilsTest, Test_Convert_Not_Modifying_If_Failed)
{
	char testing = 100;
	// Hex based string without hex parameter for convert()
	ASSERT_FALSE(convert(QString("0x1"), testing));
	ASSERT_EQ(testing, 100);
	// Out of ranges
	ASSERT_FALSE(convert(QString("999"), testing));
	ASSERT_EQ(testing, 100);
	ASSERT_FALSE(convert(QString("-999"), testing));
	ASSERT_EQ(testing, 100);
}


TEST(ParsingUtilsTest, Test_Convert_Unsigned)
{
	unsigned testing = 0;
	// Base 10
	ASSERT_TRUE(convert(QString("1"), testing));
	ASSERT_EQ(testing, 1);
	ASSERT_TRUE(convert(QString("10000"), testing));
	ASSERT_EQ(testing, 10000);
	// Base 16
	ASSERT_TRUE(convert<16>(QString("0x1"), testing));
	ASSERT_EQ(testing, 1);
	ASSERT_TRUE(convert<16>(QString("0xFFFF"), testing));
	ASSERT_EQ(testing, 0xFFFF);
	// Additional whitespaces
	ASSERT_TRUE(convert(QString("1  "), testing));
	ASSERT_EQ(testing, 1);
	ASSERT_TRUE(convert(QString("   999"), testing));
	ASSERT_EQ(testing, 999);
	ASSERT_TRUE(convert(QString("   999    "), testing));
	ASSERT_EQ(testing, 999);
}

TEST(ParsingUtilsTest, Test_Convert_Failed_Unsigned)
{
	unsigned testing = 3;

	ASSERT_FALSE(convert(QString("-1"), testing));
	ASSERT_EQ(testing, 3);
	ASSERT_FALSE(convert(QString("999999999999999"), testing));
	ASSERT_EQ(testing, 3);
}

TEST(ParsingUtilsTest, Test_LogHelper_Load_Sanity)
{
	QString content{
		"[General]\n"
		"Key1 = Val1\n"
		"Key2 = Val2\n"
		"[Section 2]\n"
		"Key1 = Val1\n"
		"Key2 = Val2 ; comment\n"
		"; comment\n"
	};
	LogHelper log;
	ASSERT_TRUE(log.LoadFromString(content));
	ASSERT_EQ(log.Value("General", "Key1"), QString("Val1"));
	ASSERT_EQ(log.Value("General", "Key2"), QString("Val2"));
	ASSERT_EQ(log.Value("Section 2", "Key1"), QString("Val1"));
	ASSERT_EQ(log.Value("Section 2", "Key2"), QString("Val2"));
}

TEST(ParsingUtilsTest, Test_LogHelper_Load_Spaces)
{
	QString content{
		"  \n"
		"  [General]  \n"
		"  Key1 = Val1\n"
		"Key2 = Val2  \n"
		"[Section 2]\n"
		"  Key1 = Val1  \n"
		"  Key 2 = Val 2 ; comment\n"
		"   ; comment\n"
		"\n"
	};
	LogHelper log;
	ASSERT_TRUE(log.LoadFromString(content));
	ASSERT_EQ(log.Value("General", "Key1"), QString("Val1"));
	ASSERT_EQ(log.Value("General", "Key2"), QString("Val2"));
	ASSERT_EQ(log.Value("Section 2", "Key1"), QString("Val1"));
	ASSERT_EQ(log.Value("Section 2", "Key 2"), QString("Val 2"));
}

TEST(ParsingUtilsTest, Test_LogHelper_Load_Skipping_Comments)
{
	QString content{
		" ;   comment  \n"
		"[General];comment\n"
		"Key1 = Val1;comment\n"
		"Key2 = Val2    ; comment\n"
		";[Section 2]\n"
		"    ;  Key1 = Val1\n"
		"   ; comment\n"
	};
	LogHelper log;
	ASSERT_TRUE(log.LoadFromString(content));
	ASSERT_EQ(log.Value("General", "Key1"), QString("Val1"));
	ASSERT_EQ(log.Value("General", "Key2"), QString("Val2"));
	ASSERT_EQ(log.Value("Section 2", "Key1"), QStringView{});
	ASSERT_EQ(log.Value("Section 2", "Key2"), QStringView{});
}

TEST(ParsingUtilsTest, Test_LogHelper_Load_Double_Quotes)
{
	QString content{
		"[General]\n"
		"Key1 = \"Val1\";comment\n"
		"Key2 = \"Val2\"    ; comment\n"
		"[Section 2]\n"
		"Key1 =     \"Val 1\"\n"
		"Key2 = \"1 2 3 456\"  \n"
	};
	LogHelper log;
	ASSERT_TRUE(log.LoadFromString(content));
	ASSERT_EQ(log.Value("General", "Key1"), QString("Val1"));
	ASSERT_EQ(log.Value("General", "Key2"), QString("Val2"));
	ASSERT_EQ(log.Value("Section 2", "Key1"), QString("Val 1"));
	ASSERT_EQ(log.Value("Section 2", "Key2"), QString("1 2 3 456"));
}

TEST(ParsingUtilsTest, Test_LogHelper_Load_Skipping_Wrong_Lines)
{
	QString content{
		"[General]\n"
		"  = Val1\n"
		"Key2 =   \n"
		"General\n"
		"General2]\n"
		"Key3 Val2\n"
		"[Section2\n"
		"KeyTest = test\n"
		"Section3]\n"
		"KeyTest2 = test\n"
		"\"KeyTest3 = test\"\n"
	};
	LogHelper log;
	ASSERT_TRUE(log.LoadFromString(content));
	ASSERT_EQ(log.Value("General", "Key1"), QStringView{});
	ASSERT_EQ(log.Value("General", "Key2"), QStringView{});
	ASSERT_EQ(log.Value("General", "KeyTest"), QString("test"));
	ASSERT_EQ(log.Value("General", "KeyTest2"), QString("test"));
	ASSERT_EQ(log.Value("General", "KeyTest3"), QStringView{});
}

TEST(ParsingUtilsTest, Test_LogHelper_Load_From_File)
{
	LogHelper log;
	ASSERT_TRUE(log.LoadFromFile("TestData/log_example.log"));
	ASSERT_EQ(log.Value("General", "Key1"), QString("Val1"));
	ASSERT_EQ(log.Value("General", "Key2"), QString("Val2"));
	ASSERT_EQ(log.Value("Section 2", "Key1"), QString("Val1"));
	ASSERT_EQ(log.Value("Section 2", "Key2"), QString("Val2"));
}