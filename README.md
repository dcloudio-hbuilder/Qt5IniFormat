# Qt5IniFormat

A small Qt-format INI reader/writer library intended to be used as a
custom QSettings format. The implementation is based on QtCore
functionality and includes parts derived from Qt sources.

Repository layout (key files)
- `qt5iniformat.h` / `qt5iniformat.cpp` — public interface exported by the library.
- `qt5iniimpl.h` / `qt5iniimpl.cpp` — the actual INI read/write implementation (partially
  derived from QtCore).
- `Qt5IniFormat.pro` — qmake project file to build the library.
- `LICENSE` — licensing information for the repository (contains notes about Qt-derived
  files and the Unlicense text for other files).

Overview
--------
The library exposes two functions that can be registered with
`QSettings::registerFormat` to provide a custom INI format handler:

- `Qt5IniFormatReadFunc(QIODevice & device, QSettings::SettingsMap & map)`
- `Qt5IniFormatWriteFunc(QIODevice & device, const QSettings::SettingsMap & map)`

These functions read from / write to a `QIODevice` and translate between
INI text and `QSettings::SettingsMap`.

Build
-----
Prerequisites: Qt (qmake) and a compatible compiler toolchain (mingw, MSVC,
clang, etc.).

Example build steps (Unix / MinGW/MSYS):

```ps1
qmake Qt5IniFormat.pro
make
```

Example build steps (MSVC / Visual Studio developer prompt):

```ps1
qmake Qt5IniFormat.pro -spec win32-msvc
nmake
```

You can also open `Qt5IniFormat.pro` in Qt Creator and build from there.

Usage example
-------------
Register the format and use it with `QSettings`:

```cpp
#include <QSettings>
#include "qt5iniformat.h"

int main()
{
    // Register a custom extension/format (returns a QSettings::Format value)
    QSettings::Format fmt = QSettings::registerFormat("ini5", Qt5IniFormatReadFunc, Qt5IniFormatWriteFunc);

    // Use the registered format to open a settings file
    QSettings settings("config.ini", fmt);
    settings.setValue("window/width", 800);
    settings.setValue("window/height", 600);
    settings.sync();

    return 0;
}
```

API summary
-----------
- `bool Qt5IniFormatReadFunc(QIODevice & device, QSettings::SettingsMap & map);`
  - Read INI data from `device` and populate `map`.
- `bool Qt5IniFormatWriteFunc(QIODevice & device, const QSettings::SettingsMap &map);`
  - Write `map` contents to `device` in INI format.

License and copyright
---------------------
Important: some files in this repository include copyright and license headers
from Qt. In particular, `qt5iniimpl.h` and `qt5iniimpl.cpp` contain a Qt
copyright notice and indicate licensing under The Qt Company terms (LGPL v2.1 or
v3) with the Qt Company LGPL Exception 1.1.

Other files in the repository are released under the Unlicense (public domain).
Refer to the `LICENSE` file at the repository root for the exact, combined text
and details.

If you need the repository under a single unified license, you must either:
1. Replace Qt-derived code with your own implementation, or
2. Obtain the necessary rights from the original copyright holder.

Contributing
------------
- Issues and pull requests are welcome. When proposing changes that affect files
  derived from Qt, please make the licensing consequences clear in the PR
  description.
- If you plan to replace Qt-derived portions to change licensing, make sure
  that the new code is authored by you or contributors who agree to the
  intended license.

Contact
-------
Create an issue in the repository or consult the repository host (e.g. GitHub)
for author/maintainer contact details.

Appendix
--------
If you embed this library as a submodule or copy it into another project,
consider the following:

1. Review the license headers inside `qt5iniimpl.*` before distribution.
2. If you have licensing questions for commercial use, consult legal counsel or
   contact The Qt Company.

---

This README was generated based on the repository's source files.
