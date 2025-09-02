#include "qt5iniformat.h"
#include "qt5iniimpl.h"

bool Qt5IniFormatReadFunc(QIODevice & device, QSettings::SettingsMap & map){
    return Qt5IniImpl::ReadFunc(device, map);
}
bool Qt5IniFormatWriteFunc(QIODevice & device, const QSettings::SettingsMap &map){
    return Qt5IniImpl::WriteFunc(device, map);
}
