/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qt5iniimpl.h"
#include <QRect>
#include <QIODevice>
#include <QDataStream>
#include <QVector>

static const Qt::CaseSensitivity IniCaseSensitivity = Qt::CaseSensitive;
class QSettingsKey : public QString
{
public:
    inline QSettingsKey(const QString &key, Qt::CaseSensitivity cs, int /* position */ = -1)
        : QString(key) { Q_ASSERT(cs == Qt::CaseSensitive); Q_UNUSED(cs); }

    inline QString originalCaseKey() const { return *this; }
    inline int originalKeyPosition() const { return -1; }
};

typedef QMap<QSettingsKey, QByteArray> UnparsedSettingsMap;
typedef QMap<QSettingsKey, QVariant> ParsedSettingsMap;

class QSettingsGroup
{
public:
    inline QSettingsGroup()
        : num(-1), maxNum(-1) {}
    inline QSettingsGroup(const QString &s)
        : str(s), num(-1), maxNum(-1) {}
    inline QSettingsGroup(const QString &s, bool guessArraySize)
        : str(s), num(0), maxNum(guessArraySize ? 0 : -1) {}

    inline QString name() const { return str; }
    inline QString toString() const;
    inline bool isArray() const { return num != -1; }
    inline int arraySizeGuess() const { return maxNum; }
    inline void setArrayIndex(int i)
    { num = i + 1; if (maxNum != -1 && num > maxNum) maxNum = num; }

    QString str;
    int num;
    int maxNum;
};
Q_DECLARE_TYPEINFO(QSettingsGroup, Q_MOVABLE_TYPE);

inline QString QSettingsGroup::toString() const
{
    QString result;
    result = str;
    if (num > 0) {
        result += QLatin1Char('/');
        result += QString::number(num);
    }
    return result;
}


enum { Space = 0x1, Special = 0x2 };

static const char charTraits[256] =
    {
        // Space: '\t', '\n', '\r', ' '
        // Special: '\n', '\r', '"', ';', '=', '\\'

        0, 0, 0, 0, 0, 0, 0, 0, 0, Space, Space | Special, 0, 0, Space | Special, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        Space, 0, Special, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, Special, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

bool readIniLine(const QByteArray &data, int &dataPos,
                                           int &lineStart, int &lineLen, int &equalsPos)
{
    int dataLen = data.length();
    bool inQuotes = false;

    equalsPos = -1;

    lineStart = dataPos;
    while (lineStart < dataLen && (charTraits[uint(uchar(data.at(lineStart)))] & Space))
        ++lineStart;

    int i = lineStart;
    while (i < dataLen) {
        while (!(charTraits[uint(uchar(data.at(i)))] & Special)) {
            if (++i == dataLen)
                goto break_out_of_outer_loop;
        }

        char ch = data.at(i++);
        if (ch == '=') {
            if (!inQuotes && equalsPos == -1)
                equalsPos = i - 1;
        } else if (ch == '\n' || ch == '\r') {
            if (i == lineStart + 1) {
                ++lineStart;
            } else if (!inQuotes) {
                --i;
                goto break_out_of_outer_loop;
            }
        } else if (ch == '\\') {
            if (i < dataLen) {
                char ch = data.at(i++);
                if (i < dataLen) {
                    char ch2 = data.at(i);
                    // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
                    if ((ch == '\n' && ch2 == '\r') || (ch == '\r' && ch2 == '\n'))
                        ++i;
                }
            }
        } else if (ch == '"') {
            inQuotes = !inQuotes;
        } else {
            Q_ASSERT(ch == ';');

            if (i == lineStart + 1) {
                char ch;
                while (i < dataLen && (((ch = data.at(i)) != '\n') && ch != '\r'))
                    ++i;
                lineStart = i;
            } else if (!inQuotes) {
                --i;
                goto break_out_of_outer_loop;
            }
        }
    }

break_out_of_outer_loop:
    dataPos = i;
    lineLen = i - lineStart;
    return lineLen > 0;
}
QString variantToString(const QVariant &v)
{
    QString result;

    switch (v.type()) {
    case QVariant::Invalid:
        result = QLatin1String("@Invalid()");
        break;

    case QVariant::ByteArray: {
        QByteArray a = v.toByteArray();
        result = QLatin1String("@ByteArray(");
        result += QString::fromLatin1(a.constData(), a.size());
        result += QLatin1Char(')');
        break;
    }

    case QVariant::String:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::Bool:
    case QVariant::Double:
    case QVariant::KeySequence: {
        result = v.toString();
        if (result.contains(QChar::Null))
            result = QLatin1String("@String(") + result + QLatin1Char(')');
        else if (result.startsWith(QLatin1Char('@')))
            result.prepend(QLatin1Char('@'));
        break;
    }
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Rect: {
        QRect r = qvariant_cast<QRect>(v);
        result += QLatin1String("@Rect(");
        result += QString::number(r.x());
        result += QLatin1Char(' ');
        result += QString::number(r.y());
        result += QLatin1Char(' ');
        result += QString::number(r.width());
        result += QLatin1Char(' ');
        result += QString::number(r.height());
        result += QLatin1Char(')');
        break;
    }
    case QVariant::Size: {
        QSize s = qvariant_cast<QSize>(v);
        result += QLatin1String("@Size(");
        result += QString::number(s.width());
        result += QLatin1Char(' ');
        result += QString::number(s.height());
        result += QLatin1Char(')');
        break;
    }
    case QVariant::Point: {
        QPoint p = qvariant_cast<QPoint>(v);
        result += QLatin1String("@Point(");
        result += QString::number(p.x());
        result += QLatin1Char(' ');
        result += QString::number(p.y());
        result += QLatin1Char(')');
        break;
    }
#endif // !QT_NO_GEOM_VARIANT

    default: {
#ifndef QT_NO_DATASTREAM
        QDataStream::Version version;
        const char *typeSpec;
        if (v.type() == QVariant::DateTime) {
            version = QDataStream::Qt_5_6;
            typeSpec = "@DateTime(";
        } else {
            version = QDataStream::Qt_4_0;
            typeSpec = "@Variant(";
        }
        QByteArray a;
        {
            QDataStream s(&a, QIODevice::WriteOnly);
            s.setVersion(version);
            s << v;
        }

        result = QLatin1String(typeSpec);
        result += QString::fromLatin1(a.constData(), a.size());
        result += QLatin1Char(')');
#else
        Q_ASSERT(!"QSettings: Cannot save custom types without QDataStream support");
#endif
        break;
    }
    }

    return result;
}
QStringList variantListToStringList(const QVariantList &l)
{
    QStringList result;
    result.reserve(l.count());
    QVariantList::const_iterator it = l.constBegin();
    for (; it != l.constEnd(); ++it)
        result.append(variantToString(*it));
    return result;
}



QStringList splitArgs(const QString &s, int idx)
{
    int l = s.length();
    Q_ASSERT(l > 0);
    Q_ASSERT(s.at(idx) == QLatin1Char('('));
    Q_ASSERT(s.at(l - 1) == QLatin1Char(')'));

    QStringList result;
    QString item;

    for (++idx; idx < l; ++idx) {
        QChar c = s.at(idx);
        if (c == QLatin1Char(')')) {
            Q_ASSERT(idx == l - 1);
            result.append(item);
        } else if (c == QLatin1Char(' ')) {
            result.append(item);
            item.clear();
        } else {
            item.append(c);
        }
    }

    return result;
}


QVariant stringToVariant(const QString &s)
{
    if (s.startsWith(QLatin1Char('@'))) {
        if (s.endsWith(QLatin1Char(')'))) {
            if (s.startsWith(QLatin1String("@ByteArray("))) {
                // return QVariant(s.midRef(11, s.size() - 12).toLatin1());
                return QVariant(s.mid(11, s.size() - 12).toLatin1());
            } else if (s.startsWith(QLatin1String("@String("))) {
                // return QVariant(s.midRef(8, s.size() - 9).toString());
                return QVariant(s.mid(8, s.size() - 9));
            } else if (s.startsWith(QLatin1String("@Variant("))
                       || s.startsWith(QLatin1String("@DateTime("))) {
#ifndef QT_NO_DATASTREAM
                QDataStream::Version version;
                int offset;
                if (s.at(1) == QLatin1Char('D')) {
                    version = QDataStream::Qt_5_6;
                    offset = 10;
                } else {
                    version = QDataStream::Qt_4_0;
                    offset = 9;
                }
                QByteArray a = /*s.midRef(offset).toLatin1();*/s.mid(offset).toLatin1();
                QDataStream stream(&a, QIODevice::ReadOnly);
                stream.setVersion(version);
                QVariant result;
                stream >> result;
                return result;
#else
                Q_ASSERT(!"QSettings: Cannot load custom types without QDataStream support");
#endif
#ifndef QT_NO_GEOM_VARIANT
            } else if (s.startsWith(QLatin1String("@Rect("))) {
                QStringList args = splitArgs(s, 5);
                if (args.size() == 4)
                    return QVariant(QRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt()));
            } else if (s.startsWith(QLatin1String("@Size("))) {
                QStringList args = splitArgs(s, 5);
                if (args.size() == 2)
                    return QVariant(QSize(args[0].toInt(), args[1].toInt()));
            } else if (s.startsWith(QLatin1String("@Point("))) {
                QStringList args = splitArgs(s, 6);
                if (args.size() == 2)
                    return QVariant(QPoint(args[0].toInt(), args[1].toInt()));
#endif
            } else if (s == QLatin1String("@Invalid()")) {
                return QVariant();
            }

        }
        if (s.startsWith(QLatin1String("@@")))
            return QVariant(s.mid(1));
    }

    return QVariant(s);
}
QVariant stringListToVariantList(const QStringList &l)
{
    QStringList outStringList = l;
    for (int i = 0; i < outStringList.count(); ++i) {
        const QString &str = outStringList.at(i);

        if (str.startsWith(QLatin1Char('@'))) {
            if (str.length() >= 2 && str.at(1) == QLatin1Char('@')) {
                outStringList[i].remove(0, 1);
            } else {
                QVariantList variantList;
                const int stringCount = l.count();
                variantList.reserve(stringCount);
                for (int j = 0; j < stringCount; ++j)
                    variantList.append(stringToVariant(l.at(j)));
                return variantList;
            }
        }
    }
    return outStringList;
}

static const char hexDigits[] = "0123456789ABCDEF";
void iniEscapedKey(const QString &key, QByteArray &result)
{
    result.reserve(result.length() + key.length() * 3 / 2);
    for (int i = 0; i < key.size(); ++i) {
        uint ch = key.at(i).unicode();

        if (ch == '/') {
            result += '\\';
        } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')
                   || ch == '_' || ch == '-' || ch == '.') {
            result += (char)ch;
        } else if (ch <= 0xFF) {
            result += '%';
            result += hexDigits[ch / 16];
            result += hexDigits[ch % 16];
        } else {
            result += "%U";
            QByteArray hexCode;
            for (int i = 0; i < 4; ++i) {
                hexCode.prepend(hexDigits[ch % 16]);
                ch >>= 4;
            }
            result += hexCode;
        }
    }
}
bool iniUnescapedKey(const QByteArray &key, int from, int to, QString &result)
{
    bool lowercaseOnly = true;
    int i = from;
    result.reserve(result.length() + (to - from));
    while (i < to) {
        int ch = (uchar)key.at(i);

        if (ch == '\\') {
            result += QLatin1Char('/');
            ++i;
            continue;
        }

        if (ch != '%' || i == to - 1) {
            if (uint(ch - 'A') <= 'Z' - 'A') // only for ASCII
                lowercaseOnly = false;
            result += QLatin1Char(ch);
            ++i;
            continue;
        }

        int numDigits = 2;
        int firstDigitPos = i + 1;

        ch = key.at(i + 1);
        if (ch == 'U') {
            ++firstDigitPos;
            numDigits = 4;
        }

        if (firstDigitPos + numDigits > to) {
            result += QLatin1Char('%');
            // ### missing U
            ++i;
            continue;
        }

        bool ok;
        ch = key.mid(firstDigitPos, numDigits).toInt(&ok, 16);
        if (!ok) {
            result += QLatin1Char('%');
            // ### missing U
            ++i;
            continue;
        }

        QChar qch(ch);
        if (qch.isUpper())
            lowercaseOnly = false;
        result += qch;
        i = firstDigitPos + numDigits;
    }
    return lowercaseOnly;
}

void iniEscapedString(const QString &str, QByteArray &result/*, QTextCodec *codec*/)
{
    bool needsQuotes = false;
    bool escapeNextIfDigit = false;
    // bool useCodec = codec && !str.startsWith(QLatin1String("@ByteArray("))
    //                 && !str.startsWith(QLatin1String("@Variant("));

    int i;
    int startPos = result.size();

    result.reserve(startPos + str.size() * 3 / 2);
    const QChar *unicode = str.unicode();
    for (i = 0; i < str.size(); ++i) {
        uint ch = unicode[i].unicode();
        if (ch == ';' || ch == ',' || ch == '=')
            needsQuotes = true;

        if (escapeNextIfDigit
            && ((ch >= '0' && ch <= '9')
                || (ch >= 'a' && ch <= 'f')
                || (ch >= 'A' && ch <= 'F'))) {
            result += "\\x";
            result += QByteArray::number(ch, 16);
            continue;
        }

        escapeNextIfDigit = false;

        switch (ch) {
        case '\0':
            result += "\\0";
            escapeNextIfDigit = true;
            break;
        case '\a':
            result += "\\a";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\v':
            result += "\\v";
            break;
        case '"':
        case '\\':
            result += '\\';
            result += (char)ch;
            break;
        default:
            if (ch <= 0x1F || (ch >= 0x7F /*&& !useCodec*/)) {
                result += "\\x";
                result += QByteArray::number(ch, 16);
                escapeNextIfDigit = true;
// #ifndef QT_NO_TEXTCODEC
//             } else if (useCodec) {
//                 // slow
//                 result += codec->fromUnicode(&unicode[i], 1);
// #endif
            } else {
                result += (char)ch;
            }
        }
    }

    if (needsQuotes
        || (startPos < result.size() && (result.at(startPos) == ' '
                                         || result.at(result.size() - 1) == ' '))) {
        result.insert(startPos, '"');
        result += '"';
    }
}

inline static void iniChopTrailingSpaces(QString &str, int limit)
{
    int n = str.size() - 1;
    QChar ch;
    while (n >= limit && ((ch = str.at(n)) == QLatin1Char(' ') || ch == QLatin1Char('\t')))
        str.truncate(n--);
}

void iniEscapedStringList(const QStringList &strs, QByteArray &result)
{
    if (strs.isEmpty()) {
        /*
            We need to distinguish between empty lists and one-item
            lists that contain an empty string. Ideally, we'd have a
            @EmptyList() symbol but that would break compatibility
            with Qt 4.0. @Invalid() stands for QVariant(), and
            QVariant().toStringList() returns an empty QStringList,
            so we're in good shape.
        */
        result += "@Invalid()";
    } else {
        for (int i = 0; i < strs.size(); ++i) {
            if (i != 0)
                result += ", ";
            iniEscapedString(strs.at(i), result);
        }
    }
}

bool iniUnescapedStringList(const QByteArray &str, int from, int to,
                                              QString &stringResult, QStringList &stringListResult)
{
    static const char escapeCodes[][2] =
        {
            { 'a', '\a' },
            { 'b', '\b' },
            { 'f', '\f' },
            { 'n', '\n' },
            { 'r', '\r' },
            { 't', '\t' },
            { 'v', '\v' },
            { '"', '"' },
            { '?', '?' },
            { '\'', '\'' },
            { '\\', '\\' }
        };
    static const int numEscapeCodes = sizeof(escapeCodes) / sizeof(escapeCodes[0]);

    bool isStringList = false;
    bool inQuotedString = false;
    bool currentValueIsQuoted = false;
    int escapeVal = 0;
    int i = from;
    char ch;

StSkipSpaces:
    while (i < to && ((ch = str.at(i)) == ' ' || ch == '\t'))
        ++i;
    // fallthrough

StNormal:
    int chopLimit = stringResult.length();
    while (i < to) {
        switch (str.at(i)) {
        case '\\':
            ++i;
            if (i >= to)
                goto end;

            ch = str.at(i++);
            for (int j = 0; j < numEscapeCodes; ++j) {
                if (ch == escapeCodes[j][0]) {
                    stringResult += QLatin1Char(escapeCodes[j][1]);
                    goto StNormal;
                }
            }

            if (ch == 'x') {
                escapeVal = 0;

                if (i >= to)
                    goto end;

                ch = str.at(i);
                if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
                    goto StHexEscape;
            } else if (ch >= '0' && ch <= '7') {
                escapeVal = ch - '0';
                goto StOctEscape;
            } else if (ch == '\n' || ch == '\r') {
                if (i < to) {
                    char ch2 = str.at(i);
                    // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
                    if ((ch2 == '\n' || ch2 == '\r') && ch2 != ch)
                        ++i;
                }
            } else {
                // the character is skipped
            }
            chopLimit = stringResult.length();
            break;
        case '"':
            ++i;
            currentValueIsQuoted = true;
            inQuotedString = !inQuotedString;
            if (!inQuotedString)
                goto StSkipSpaces;
            break;
        case ',':
            if (!inQuotedString) {
                if (!currentValueIsQuoted)
                    iniChopTrailingSpaces(stringResult, chopLimit);
                if (!isStringList) {
                    isStringList = true;
                    stringListResult.clear();
                    stringResult.squeeze();
                }
                stringListResult.append(stringResult);
                stringResult.clear();
                currentValueIsQuoted = false;
                ++i;
                goto StSkipSpaces;
            }
            // fallthrough
        default: {
            int j = i + 1;
            while (j < to) {
                ch = str.at(j);
                if (ch == '\\' || ch == '"' || ch == ',')
                    break;
                ++j;
            }

            {
                int n = stringResult.size();
                stringResult.resize(n + (j - i));
                QChar *resultData = stringResult.data() + n;
                for (int k = i; k < j; ++k)
                    *resultData++ = QLatin1Char(str.at(k));
            }
            i = j;
        }
        }
    }
    if (!currentValueIsQuoted)
        iniChopTrailingSpaces(stringResult, chopLimit);
    goto end;

StHexEscape:
    if (i >= to) {
        stringResult += QChar(escapeVal);
        goto end;
    }

    ch = str.at(i);
    if (ch >= 'a')
        ch -= 'a' - 'A';
    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) {
        escapeVal <<= 4;
        escapeVal += strchr(hexDigits, ch) - hexDigits;
        ++i;
        goto StHexEscape;
    } else {
        stringResult += QChar(escapeVal);
        goto StNormal;
    }

StOctEscape:
    if (i >= to) {
        stringResult += QChar(escapeVal);
        goto end;
    }

    ch = str.at(i);
    if (ch >= '0' && ch <= '7') {
        escapeVal <<= 3;
        escapeVal += ch - '0';
        ++i;
        goto StOctEscape;
    } else {
        stringResult += QChar(escapeVal);
        goto StNormal;
    }

end:
    if (isStringList)
        stringListResult.append(stringResult);
    return isStringList;
}

bool readIniSection(const QSettingsKey &section, const QByteArray &data,
               ParsedSettingsMap *settingsMap)
{
    QStringList strListValue;
    bool sectionIsLowercase = (section == section.originalCaseKey());
    int equalsPos;

    bool ok = true;
    int dataPos = 0;
    int lineStart;
    int lineLen;
    int position = section.originalKeyPosition();

    while (readIniLine(data, dataPos, lineStart, lineLen, equalsPos)) {
        char ch = data.at(lineStart);
        Q_ASSERT(ch != '[');

        if (equalsPos == -1) {
            if (ch != ';')
                ok = false;
            continue;
        }

        int keyEnd = equalsPos;
        while (keyEnd > lineStart && ((ch = data.at(keyEnd - 1)) == ' ' || ch == '\t'))
            --keyEnd;
        int valueStart = equalsPos + 1;

        QString key = section.originalCaseKey();
        bool keyIsLowercase = (iniUnescapedKey(data, lineStart, keyEnd, key) && sectionIsLowercase);

        QString strValue;
        strValue.reserve(lineLen - (valueStart - lineStart));
        bool isStringList = iniUnescapedStringList(data, valueStart, lineStart + lineLen,
                                                   strValue, strListValue/*, codec*/);
        QVariant variant;
        if (isStringList) {
            variant = stringListToVariantList(strListValue);
        } else {
            variant = stringToVariant(strValue);
        }

        /*
            We try to avoid the expensive toLower() call in
            QSettingsKey by passing Qt::CaseSensitive when the
            key is already in lowercase.
        */
        settingsMap->insert(QSettingsKey(key, keyIsLowercase ? Qt::CaseSensitive
                                                             : IniCaseSensitivity,
                                         position),
                            variant);
        ++position;
    }

    return ok;
}
bool readIniFile(const QByteArray &data,
            UnparsedSettingsMap *unparsedIniSections)
{
#define FLUSH_CURRENT_SECTION() \
    { \
            QByteArray &sectionData = (*unparsedIniSections)[QSettingsKey(currentSection, \
                                                  IniCaseSensitivity, \
                                                  sectionPosition)]; \
            if (!sectionData.isEmpty()) \
            sectionData.append('\n'); \
            sectionData += data.mid(currentSectionStart, lineStart - currentSectionStart); \
            sectionPosition = ++position; \
    }

    QString currentSection;
    int currentSectionStart = 0;
    int dataPos = 0;
    int lineStart;
    int lineLen;
    int equalsPos;
    int position = 0;
    int sectionPosition = 0;
    bool ok = true;

    while (readIniLine(data, dataPos, lineStart, lineLen, equalsPos)) {
        char ch = data.at(lineStart);
        if (ch == '[') {
            FLUSH_CURRENT_SECTION();

            // this is a section
            QByteArray iniSection;
            int idx = data.indexOf(']', lineStart);
            if (idx == -1 || idx >= lineStart + lineLen) {
                ok = false;
                iniSection = data.mid(lineStart + 1, lineLen - 1);
            } else {
                iniSection = data.mid(lineStart + 1, idx - lineStart - 1);
            }

            iniSection = iniSection.trimmed();

            if (qstricmp(iniSection.constData(), "general") == 0) {
                currentSection.clear();
            } else {
                if (qstricmp(iniSection.constData(), "%general") == 0) {
                    currentSection = QLatin1String(iniSection.constData() + 1);
                } else {
                    currentSection.clear();
                    iniUnescapedKey(iniSection, 0, iniSection.size(), currentSection);
                }
                currentSection += QLatin1Char('/');
            }
            currentSectionStart = dataPos;
        }
        ++position;
    }

    Q_ASSERT(lineStart == data.length());
    FLUSH_CURRENT_SECTION();

    return ok;

#undef FLUSH_CURRENT_SECTION
}

class QSettingsIniKey : public QString
{
public:
    inline QSettingsIniKey() : position(-1) {}
    inline QSettingsIniKey(const QString &str, int pos = -1) : QString(str), position(pos) {}

    int position;
};
Q_DECLARE_TYPEINFO(QSettingsIniKey, Q_MOVABLE_TYPE);

static bool operator<(const QSettingsIniKey &k1, const QSettingsIniKey &k2)
{
    if (k1.position != k2.position)
        return k1.position < k2.position;
    return static_cast<const QString &>(k1) < static_cast<const QString &>(k2);
}

typedef QMap<QSettingsIniKey, QVariant> IniKeyMap;

struct QSettingsIniSection
{
    int position;
    IniKeyMap keyMap;

    inline QSettingsIniSection() : position(-1) {}
};

typedef QMap<QString, QSettingsIniSection> IniMap;

/*
    This would be more straightforward if we didn't try to remember the original
    key order in the .ini file, but we do.
*/
bool writeIniFile(QIODevice &device, const ParsedSettingsMap &map)
{
    IniMap iniMap;
    IniMap::const_iterator i;

#ifdef Q_OS_WIN
    const char * const eol = "\r\n";
#else
    const char eol = '\n';
#endif

    for (ParsedSettingsMap::const_iterator j = map.constBegin(); j != map.constEnd(); ++j) {
        QString section;
        QSettingsIniKey key(j.key().originalCaseKey(), j.key().originalKeyPosition());
        int slashPos;

        if ((slashPos = key.indexOf(QLatin1Char('/'))) != -1) {
            section = key.left(slashPos);
            key.remove(0, slashPos + 1);
        }

        QSettingsIniSection &iniSection = iniMap[section];

        // -1 means infinity
        if (uint(key.position) < uint(iniSection.position))
            iniSection.position = key.position;
        iniSection.keyMap[key] = j.value();
    }

    const int sectionCount = iniMap.size();
    QVector<QSettingsIniKey> sections;
    sections.reserve(sectionCount);
    for (i = iniMap.constBegin(); i != iniMap.constEnd(); ++i)
        sections.append(QSettingsIniKey(i.key(), i.value().position));
    std::sort(sections.begin(), sections.end());

    bool writeError = false;
    for (int j = 0; !writeError && j < sectionCount; ++j) {
        i = iniMap.constFind(sections.at(j));
        Q_ASSERT(i != iniMap.constEnd());

        QByteArray realSection;

        iniEscapedKey(i.key(), realSection);

        if (realSection.isEmpty()) {
            realSection = "[General]";
        } else if (qstricmp(realSection.constData(), "general") == 0) {
            realSection = "[%General]";
        } else {
            realSection.prepend('[');
            realSection.append(']');
        }

        if (j != 0)
            realSection.prepend(eol);
        realSection += eol;

        device.write(realSection);

        const IniKeyMap &ents = i.value().keyMap;
        for (IniKeyMap::const_iterator j = ents.constBegin(); j != ents.constEnd(); ++j) {
            QByteArray block;
            iniEscapedKey(j.key(), block);
            block += '=';

            const QVariant &value = j.value();

            /*
                The size() != 1 trick is necessary because
                QVariant(QString("foo")).toList() returns an empty
                list, not a list containing "foo".
            */
            if (value.type() == QVariant::StringList
                || (value.type() == QVariant::List && value.toList().size() != 1)) {
                iniEscapedStringList(variantListToStringList(value.toList()), block/*, iniCodec*/);
            } else {
                iniEscapedString(variantToString(value), block/*, iniCodec*/);
            }
            block += eol;
            if (device.write(block) == -1) {
                writeError = true;
                break;
            }
        }
    }
    return !writeError;
}

bool ensureAllSectionsParsed(UnparsedSettingsMap & unparsedIniSections, ParsedSettingsMap & originalKeys)
{
    UnparsedSettingsMap::const_iterator i = unparsedIniSections.constBegin();
    const UnparsedSettingsMap::const_iterator end = unparsedIniSections.constEnd();

    for (; i != end; ++i) {
        if (!readIniSection(i.key(), i.value(), &originalKeys)){
            return false;
        }
    }
    unparsedIniSections.clear();
    return true;
}

bool Qt5IniImpl::ReadFunc(QIODevice &device, QSettings::SettingsMap &map)
{
    UnparsedSettingsMap tmpIniSections;
    if(!readIniFile(device.readAll(), &tmpIniSections)){
        return false;
    }
    ParsedSettingsMap outmap;
    if(!ensureAllSectionsParsed(tmpIniSections, outmap)){
        return false;
    }
    ParsedSettingsMap::const_iterator i = outmap.constBegin();
    while (i != outmap.constEnd()) {
        map.insert(i.key(), i.value());
        ++i;
    }
    return true;
}

bool Qt5IniImpl::WriteFunc(QIODevice &device, const QSettings::SettingsMap &map)
{
    ParsedSettingsMap tmpMap;
    QSettings::SettingsMap::const_iterator it = map.constBegin();
    while(it != map.constEnd()){
        tmpMap.insert(QSettingsKey(it.key(), IniCaseSensitivity), it.value());
        ++it;
    }
    return writeIniFile(device, tmpMap);
}
