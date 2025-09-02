#ifndef QT5INIFORMAT_H
#define QT5INIFORMAT_H

#include "Qt5IniFormat_global.h"
#include <QSettings>
#include <QIODevice>

QT5INIFORMAT_EXPORT bool Qt5IniFormatReadFunc(QIODevice & device, QSettings::SettingsMap & map);
QT5INIFORMAT_EXPORT bool Qt5IniFormatWriteFunc(QIODevice & device, const QSettings::SettingsMap &map);
#endif // QT5INIFORMAT_H
