# AGENTS.md

给 AI agent / 新开发者看的项目指南。针对当前工作分支 `aaron`（Win32 摸鱼阅读器，fork 自 binbyu/Reader 的 v1.6.1.0）。

## 项目简介

纯 C++ + Win32 API 的单文件绿色小说阅读器（Reader.exe），无 MFC/.NET/Qt 等界面框架。GDI/GDI+ 自绘文本，支持 txt / epub，带章节目录、全文查找、透明度、无边框、自动翻页等"摸鱼"特性。作者 binbyu，fork 维护者 Aaron。

## 分支与远程布局

| 分支/远程 | 内容 |
|---|---|
| `aaron`（当前） | 上游 v1.6.1.0（2020-03 fork）+ 本地 3 个提交：Ctrl+H 隐藏边框、鼠标侧键4/5翻页、工程改为 v140 工具集 |
| `master`（本地） | 同步自上游的 v2.0.0.0（2022-05），落后 upstream/master 约 16 个提交 |
| `upstream`（远程） | https://github.com/binbyu/Reader ，最新含 mobi、Web 阅读（legado 局域网同步）等 |

常用对比命令：
```
git fetch upstream
git log --oneline aaron..upstream/master    # 上游新提交
git diff --stat aaron upstream/master       # 总体差异
```

## 构建（aaron 分支）

- IDE：用 Visual Studio 2019+ 打开根目录 `Reader.sln`，直接编 `Release|Win32`（解决方案里只有 Win32 平台）。
- 工具集：工程写死 `PlatformToolset=v140`（VS2015 工具集）+ `WindowsTargetPlatformVersion=10.0.14393.0`。本机没有 v140 时，可在 IDE"重定解决方案目标"，或命令行覆盖（见下），无需改文件。
- 命令行（示例，VS2022 + 任意 Win10 SDK）：
  ```
  msbuild Reader.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32 ^
    /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0
  ```
- 输出：解决方案根目录 `Release\Reader.exe`（Debug 同理）。
- 依赖全部入库，无需额外安装：
  - `opensrc/cjson`（源码随主工程编译）
  - `opensrc/zlib/lib`、`opensrc/libxml2/lib`（预编译静态库，epub 用；头文件在对应 `inc/`）
  - 系统库：gdiplus、ws2_32、winhttp（代码内 `#pragma comment` 自动链接）
- 关键宏：`ENABLE_NETWORK=1`（types.h，关闭则去掉在线升级；上游 v1.10 起用 `tool\repstr.exe` 从 vcxproj 里删该宏做"无网络版"）、`ZLIB_WINAPI`、`LIBXML_STATIC`。
- 预编译头：所有 .cpp 必须 `#include "stdafx.h"` 在最前（stdafx.cpp 创建 PCH）。
- `tool/Upgrader` 是独立的 MFC 对话框小程序（下载新版本用），不在 Reader.sln 内，需单独加工程编译；`tool/Upgrader.exe` 是预编译好的。
- 上游分支（master / upstream）构建方式不同：`rebuild.bat`（原作者机器专用）、`build-local.bat`（vswhere 自动找 MSBuild，默认 v143，出 x86+x64 并 7z 打包到 publish/）、`.github/workflows/build-package.yml`（CI，msbuild v143 x64）。

## 架构（aaron 分支，约 5700 行 C++）

```
Reader.cpp (WinMain + WndProc + 全部 On* 消息处理，~2400行，程序主干)
 ├─ Book (基类, Book.cpp)        打开/关闭书、后台线程解析、MD5、章节表
 │   ├─ TextBook (TextBook.cpp)  txt：编码探测 + 章节标题识别
 │   ├─ EpubBook (EpubBook.cpp)  epub：zlib 解压 + libxml2 解析 OPF/NCX/文本、封面
 │   └─ (Book 继承 PageCache)
 ├─ PageCache (PageCache.cpp)    分页排版引擎：行信息缓存、PageUp/Down/LineUp/Down、
 │                               进度、封面绘制；窗口尺寸/字体变化时 Reset
 ├─ Cache (Cache.cpp)            二进制持久化文件 .cache.dat（隐藏）：
 │                               header_t(全局设置) + item_t[](每本书进度, MD5 关联)
 ├─ Upgrade (Upgrade.cpp)        WinHTTP 访问 version.json 检查新版本(支持代理)
 ├─ Utils (Utils.cpp)            杂项：编码转换、拖拽、注册表默认打开方式等
 └─ dump (dump.cpp)              崩溃 mini dump
```

- 消息驱动：键盘/鼠标 → WndProc → OnPageUp/OnPageDown/OnHideBorder/OnFullScreen 等；翻页即移动 PageCache 当前行，触发 `WM_PAINT` 重绘。
- 打开书流程：`OnOpenBook` → 起 `_beginthreadex` 线程 → `Book::CalcMd5` 读文件算 MD5 → `ParserBook` 解析文本和章节 → `WM_OPEN_BOOK` 回主线程 → Cache 里恢复进度 → 显示 loading.gif 动画直到完成。
- 菜单章节列表：动态创建，命令 ID 范围定义在 types.h（`IDM_CHAPTER_BEGIN..END`、`IDM_OPEN_BEGIN..END`）；目录 UI 是 TreeView 弹窗。
- 自定义消息：`WM_NEW_VERSION`/`WM_UPDATE_CHAPTERS`/`WM_OPEN_BOOK`（WM_USER+100..102，见 types.h）。
- 设置持久化：header_t 结构（types.h）直接二进制读写，含字体/颜色/透明度/热键/翻页模式/背景图/代理等，尾部有 reserved[64] 供扩展。

## 关键文件速查

| 文件 | 作用 |
|---|---|
| `Reader/Reader.cpp/.h` | 主程序：窗口、消息、菜单、快捷键、绘制入口、自动翻页、透明度、无边框/全屏 |
| `Reader/Book.cpp/.h` | 书籍抽象基类 + 后台打开线程 |
| `Reader/TextBook.cpp` | txt 解析（编码探测、章节识别 `m_ValidChapter`） |
| `Reader/EpubBook.cpp` | epub 解析（zip/libxml2） |
| `Reader/PageCache.cpp` | 分页与排版核心算法 |
| `Reader/Cache.cpp` | 设置/进度持久化（.cache.dat） |
| `Reader/Upgrade.cpp` | 版本检查（WinHTTP + version.json + cJSON） |
| `Reader/types.h` | 公共类型、宏、自定义消息、cache 文件格式 |
| `opensrc/` | 第三方：cjson(源码) / zlib、libxml2(静态库) |
| `tool/Upgrader/` | MFC 升级器子项目 |
| `version.json` | 升级检查服务端数据（github 中转 → 网盘地址） |

## 编码约定与注意事项

- 代码风格：Win32 匈牙利命名（`hWnd`、`m_XXX`、`On*` 消息处理函数、`_XXX` 全局变量），Tab 缩进，基本无注释——改动请跟随现有风格。
- Unicode 构建（`CharacterSet=Unicode`），字符串注意 `TCHAR/WCHAR`。
- 不使用异常/Rtti；多线程用 `_beginthreadex` + 自定义 `ForceKill` 等待退出。
- 绘制全走 GDI/GDI+（背景图、loading gif、文本），无皮肤框架。
- 改 header_t/item_t 结构时必须同步考虑 `Cache` 版本兼容（`reserved` 字段 + `GetCacheVersion`）。
- 用户的本地改动只有两处功能点（Reader.cpp 的 Ctrl+H 与 WM_XBUTTONDOWN 分支，见 commit 2d2489b），后续同步上游时注意保留。

## 运行时行为

- 首次运行生成隐藏文件 `.cache.dat`（exe 同目录），删除即恢复默认/清空文件列表。
- 快捷键/翻页模式/热键等用户文档见根目录 `README.md` 与 `readme.txt`。
