# Qt5IniFormat

一个小型的 Qt 插件/库，用于以 Qt 风格解析和写入 INI 文件（基于 QtCore 的实现）。

仓库结构（关键文件）
- `qt5iniformat.h` / `qt5iniformat.cpp` — 导出库的公共接口。
- `qt5iniimpl.h` / `qt5iniimpl.cpp` — 实际的 INI 读写实现（部分源自 QtCore）。
- `Qt5IniFormat.pro` — qmake 项目文件，用于构建库。
- `LICENSE` — 仓库许可说明（包含对 Qt 源自文件的许可说明及 Unlicense 文本）。

快速说明
---------
该库提供两个导出的函数：

- `Qt5IniFormatReadFunc(QIODevice & device, QSettings::SettingsMap & map)`
- `Qt5IniFormatWriteFunc(QIODevice & device, const QSettings::SettingsMap & map)`

它们可与 `QSettings::registerFormat` 配合，注册为自定义 INI 格式解析器/写入器。

构建
----
先决条件：已安装 Qt（包含 qmake 和对应的编译工具链，如 mingw/msvc/clang）。

在常见环境下的构建示例：

使用 qmake + make (Unix / mingw/msys)：

```ps1
qmake Qt5IniFormat.pro
make
```

使用 MSVC + nmake / Visual Studio Developer 命令提示符：

```ps1
qmake Qt5IniFormat.pro -spec win32-msvc
nmake
```

如果你使用 Qt Creator，打开 `Qt5IniFormat.pro` 并直接构建即可。

使用示例
--------
下面是如何将该格式注册到 `QSettings` 并使用的示例：

```cpp
#include <QSettings>
#include "qt5iniformat.h"

int main()
{
		// 注册自定义扩展名（返回值为 QSettings::Format）
		QSettings::Format fmt = QSettings::registerFormat("ini5", Qt5IniFormatReadFunc, Qt5IniFormatWriteFunc);

		// 使用注册的格式打开设置
		QSettings settings("config.ini", fmt);
		settings.setValue("window/width", 800);
		settings.setValue("window/height", 600);
		settings.sync();

		return 0;
}
```

API 摘要
--------
- `bool Qt5IniFormatReadFunc(QIODevice & device, QSettings::SettingsMap & map);`
	- 从 `device` 读取 INI 数据并填充 `map`。
- `bool Qt5IniFormatWriteFunc(QIODevice & device, const QSettings::SettingsMap &map);`
	- 将 `map` 的内容以 INI 格式写入 `device`。

许可与版权
-----------
请注意：仓库中部分文件直接保留了来自 Qt 的版权与许可头（例如 `qt5iniimpl.h` 与 `qt5iniimpl.cpp`），这些文件按照其文件头所述采用 The Qt Company 的许可（可在 LGPL v2.1 或 v3 下使用，并包含 Qt Company LGPL Exception 1.1）。

仓库的其它部分由仓库作者以 Unlicense（公有领域）形式发布。具体请参阅项目根目录的 `LICENSE` 文件以获得详细说明。

贡献
----
- 欢迎提交 issue 或 pull request。对于修改或新增文件，请尽量遵循现有的代码风格并在 PR 描述中说明是否改变了许可来源。
- 如果你要替换或重写来自 Qt 的实现以便统一许可，请在 PR 中明确说明并保证代码为你拥有或有权以目标许可发布。

联系方式
--------
如需联系仓库维护者，请在项目中创建 issue。你也可以在代码仓库的 host（例如 GitHub）上查找作者/组织信息。

附录
----
如果你想把本库作为子模块或直接集成到你的工程，建议：

1. 在使用前阅读 `qt5iniimpl.*` 文件头中的许可声明。
2. 如需在商业产品中使用且对许可有疑问，请咨询法律顾问或联系 Qt 授权团队。

---

（自动生成并补全于本仓库代码基础上）
