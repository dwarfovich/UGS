#pragma once

#include <QtCore/QString>
#include <QtCore/QTextStream>

#include <vector>
#include <unordered_map>

namespace parsing {

#undef max
#undef min

    namespace details {
        template <typename Type>
        using Limits = std::numeric_limits<Type>;

        template<typename LongestType, typename Type>
        bool hasProperRange(const LongestType& value) {
            return (value <= LongestType(Limits<Type>::max()) && value >= LongestType(Limits<Type>::min()));
        }
    }

    template<typename Type>
    using EnableIfUnsigned = std::enable_if_t<std::is_unsigned<Type>::value, bool>;

    template<int Base = 10, typename Type>
    EnableIfUnsigned<Type> convert(QStringView str, Type& value) {
        static_assert(Base == 10 || Base == 16, "Supported bases are 10 and 16 only");

        bool ok = true;
        auto temp = str.toULong(&ok, Base);

        if (!ok || !details::hasProperRange<decltype(temp), Type>(temp)) {
            return false;
        }
        else {
            value = static_cast<Type>(temp);
            return true;
        }
    }

    template<typename Type>
    using EnableIfSigned = std::enable_if_t<std::is_signed<Type>::value, bool>;

    template<int Base = 10, typename Type>
    EnableIfSigned<Type> convert(QStringView str, Type& value) {
        static_assert(Base == 10 || Base == 16, "Supported bases are 10 and 16 only");

        bool ok = true;
        auto temp = str.toLong(&ok, Base);
        if (!ok || !details::hasProperRange<decltype(temp), Type>(temp)) {
            return false;
        }
        else {
            value = static_cast<Type>(temp);
            return true;
        }
    }

    template<int Base = 10>
    bool convert(QStringView str, QString& value) {
        value = str.toString();

        return true;
    }

    template<int Base = 10, typename T>
    bool convertMultiple(QStringView str, std::vector<T>& container) {
        static_assert(Base == 10 || Base == 16, "Supported bases are 10 and 16 only");

        const auto& tokens = str.split(' ');
        std::vector<T> result;
        for (const auto& token : tokens) {
            T temp;
            bool success = convert<Base>(token, temp);
            if (!success) {
                return false;
            }
            result.push_back(temp);
        }

        container = std::move(result);
        return true;
    }

    template<int Base = 10>
    bool convert(QStringView str, QByteArray& container) {
        if (str.size() & 1) {
            return false;
        }

        container = QByteArray::fromHex(str.toLocal8Bit());
        return true;
    }

    namespace details {
        using OptionsMap = std::unordered_map<QString, QString>;
        using SectionsMap = std::unordered_map<QString, OptionsMap>;
    } // namespace details

    QTextStream& operator>> (QTextStream& stream, details::SectionsMap& sections);

    class LogHelper {
    public:
        bool LoadFromStream(QTextStream& stream);
        bool LoadFromFile(const QString& filename);
        bool LoadFromString(const QString& str);
        void SetCurrentSection(const QString& section);
        QStringView Value(const QString& section, const QString& key) const;
        template <int Base = 10, typename Type>
        void Parse(const QString& option, Type& target) {
            auto valueString = Value(m_currentSection, option);
            if (valueString.isNull()) {
                //std::cerr << "Cannot find option " << option.toStdString() << " in section "
                //    << m_currentSection.toStdString() << '\n';
            }
            else {
                bool success = convert<Base>(valueString, target);
                if (!success) {
                    //std::cerr << "Cannot parse value for option " << option.toStdString() << '\n';
                }
            }
        }

    private:
        details::SectionsMap m_config;
        QString m_currentSection;
    };

} // namespace parsing