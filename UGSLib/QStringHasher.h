#pragma once

#include <QtCore/QHash>

namespace std {
    template<>
    struct hash<QString> {
        std::size_t operator() (const QString& s) const noexcept {
            return (size_t)qHash(s);
        }
    };
}