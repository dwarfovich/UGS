#include "stdafx.h"
#include "ParsingUtils.h"

#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QDebug>
#define DEB qDebug()

namespace parsing {
	bool LogHelper::LoadFromStream(QTextStream& stream) {
		stream >> m_config;

		return true;
	}

	bool LogHelper::LoadFromFile(const QString& filename)
	{
		QFile file { filename };
		bool success = file.open(QIODevice::Text | QIODevice::ReadOnly);
		if (!success) {
			return false;
		}

		QTextStream stream { &file };
		stream >> m_config;

		return true;
	}

	bool LogHelper::LoadFromString(const QString& str)
	{
		QString temp { str };
		QTextStream stream { &temp };
		stream >> m_config;

		return true;
	}

	void LogHelper::SetCurrentSection(const QString& section)
	{
		m_currentSection = section;
	}

	QStringView LogHelper::Value(const QString& section, const QString& key) const {
		auto sectionIter = m_config.find(section);
		if (sectionIter == m_config.cend()) {
			return {};
		}
		else {
			auto optionIter = sectionIter->second.find(key);
			if (optionIter == sectionIter->second.cend()) {
				return {};
			}
			else {
				return optionIter->second;
			}
		}
	}

	QTextStream& operator>>(QTextStream& stream, details::SectionsMap& config)
	{
		config.clear();
		QString currentSection;
		QString inputLine;
		while (!stream.atEnd()) {
			inputLine = stream.readLine();
			QStringView line = inputLine;
			line = line.trimmed();
			if (line.size() > 0 && line.front() == '[') {
				auto sectionCloseIndex = line.indexOf(']');
				if (sectionCloseIndex > 0) {
					currentSection = line.mid(1, sectionCloseIndex - 1).toString();
				}
			}
			else {
				QStringView view { line };
				auto commentPos = line.indexOf(';');
				if (commentPos >= 0) {
					view = view.left(commentPos);
				}
				
				int equalityIndex = view.indexOf('=');
				if (equalityIndex < 0) {
					continue;
				}

				QList<QStringView> tokens = { view.left(equalityIndex), view.mid(equalityIndex + 1, view.size())};
				auto keyStr = tokens[0].trimmed();
				auto valueStr = tokens[1].trimmed();
				if (valueStr.size() >= 2 && valueStr.front() == '\"') {
					valueStr = valueStr.mid(1, valueStr.size() - 2);
				}

				if (!keyStr.empty()) {
					config[currentSection][keyStr.toString()] = valueStr.toString();
				}
			}
		}

		return stream;
	}

} // namespace parsing