# AGENTS.md

给 AI agent / 新开发者看的项目指南。针对当前工作分支 `aaron`（Win32 摸鱼阅读器，fork 自 binbyu/Reader）。

## 项目简介

纯 C++ + Win32 API 的单文件绿色小说阅读器（Reader.exe），无 MFC/.NET/Qt 等界面框架。GDI/GDI+ 自绘文本，支持 txt / epub / mobi，带章节目录、书签、全文查找、透明度、无边框、高 DPI、编辑模式、自动翻页等"摸鱼"特性。作者 binbyu，fork 维护者 Aaron。

**本分支与上游的差异**（相对 upstream/master）：
- 已同步上游全部本地阅读功能（v2.0.0.x：显示设置/取色器/段距/压缩空行/章前分页、mobi、高DPI、中文菜单、按键自定义、编辑模式、书签、托盘等）。
- **移除了在线小说（书源）与 Web 阅读（legado 局域网同步）功能**：相关源文件、菜单、对话框、`book_source_t`/`.ol`/`.web` 逻辑均已删除；依赖库 libhttps/openssl/wolfssl 已从工程和仓库移除。
- **完全离线版**：vcxproj 已不定义 `ENABLE_NETWORK`，版本更新检查不再编译；Upgrade.cpp/Upgrade.h 与 Reader.cpp 中相关代码用 `#ifdef ENABLE_NETWORK` 包裹保留（源码级），未来想恢复版本检查只需在 vcxproj 加回该宏。
- **已移除代理设置**（在线功能删除后无用）：Proxy 对话框/菜单/`proxy_t`/序列化字段均不存在。
- 关于对话框只保留原作者（binbyu）、aaron 分支维护者与两个版本号信息（打赏/公众号二维码已删，barcode.h 已移除）。
- 本地功能新增：Ctrl+H 隐藏边框（Reader.cpp WM_KEYDOWN 硬编码，与按键系统并存）、鼠标侧键4/5翻页（WM_XBUTTONDOWN）、书签列表 Del 键/右键删除（删除后立即落盘）。

## 分支与远程布局

| 分支/远程 | 内容 |
|---|---|
| `aaron`（当前） | 上游最新 + 上述定制 |
| `master`（本地） | 同步自上游的 v2.0.0.0（2022-05），落后 upstream/master 若干提交，未做删减 |
| `upstream`（远程） | https://github.com/binbyu/Reader ，含在线小说/Web阅读等 |

常用对比命令：
```
git fetch upstream
git log --oneline aaron..upstream/master    # 上游新提交
git diff --stat aaron upstream/master       # 总体差异（会包含我们删掉的在线功能）
```

后续同步上游注意：上游改动若涉及 OnlineBook/OnlineDlg/BooksourceDlg/WebBook/WebDlg/HtmlParser/Jsondata 的书源部分、`ENABLE_NETWORK` 区域内的在线逻辑、Reader.rc/Reader_zh-cn.rc 的 IDD_ONLINE/IDD_BOOKSOURCE/IDD_WEB 对话框，需要按本分支方式继续排除；涉及 Jsondata/Cache/Keyset 等共享文件的改动正常合入。

## 构建

- IDE：用 Visual Studio 2022 打开根目录 `Reader.sln`，直接编 `Release|x86` 或 `Release|x64`。
- 工具集：工程为 `PlatformToolset=v143`（VS2022）+ `WindowsTargetPlatformVersion=10.0`（取最新安装的 SDK）。如需其他工具集可在 IDE"重定解决方案目标"或命令行覆盖。
- 命令行示例：
  ```
  msbuild Reader.sln /t:Rebuild /p:Configuration=Release /p:Platform=x86 ^
    /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0
  ```
- 输出：解决方案根目录 `Release\Reader.exe`（x86）、`x64\Release\Reader.exe`（x64）。
- 依赖全部入库，无需额外安装：
  - `opensrc/cjson`（源码随主工程编译）
  - `opensrc/libxml2/lib`（预编译静态库 2.11.9，win32/Makefile.msvc 重编，/MT；头文件在 `inc/`）
  - `opensrc/miniz`（源码，epub 解压 + 为 libmobi 提供 crc32/uncompress 桥接）
  - `opensrc/libmobi/src`（libmobi 0.12 源码随主工程编译，开关 `USE_MINIZ`+`USE_LIBXML2`+`USE_XMLWRITER`，不编其自带 miniz.c/xmlwriter.c）
  - 系统库：gdiplus、winhttp（代码内 `#pragma comment` 自动链接）
- 关键宏：`ENABLE_NETWORK`（**当前未定义** = 完全离线版；Upgrade/版本检查代码不编译，`#ifndef ENABLE_NETWORK` 分支生效）；`ZLIB_WINAPI`、`LIBXML_STATIC`、`CJSON_HIDE_SYMBOLS`。
- 预编译头：本项目不用 PCH，各 .cpp 自行 include（framework.h 为公共头）。
- 源码为 UTF-8 无 BOM 且含中文注释，工程已设 `/source-charset:utf-8`，勿改。
- `tool/Upgrader` 是独立的 MFC 对话框小程序（下载新版本用），不在 Reader.sln 内；`tool/Upgrader.exe` 是预编译好的。
- 上游分支（master / upstream）构建方式：`rebuild.bat`（原作者机器专用）、`build-local.bat`（vswhere 找 MSBuild，v143，出 x86+x64 并打包）、`.github/workflows/build-package.yml`（CI）。本分支同样可用 build-local.bat。

## 架构（本分支）

```
Reader.cpp (WinMain + WndProc + 消息处理，程序主干，~2800行)
 ├─ Globals.cpp        全部共享全局变量的唯一定义（Reader.h 中 extern）
 ├─ Dialogs.cpp        About / Setting(基本设置) / JumpProgress(进度跳转) 对话框
 ├─ Gfx.cpp            背景图、封面位图、loading.gif、资源图像
 ├─ WindowUtil.cpp     标题栏、菜单工具、任务栏/托盘、DPI 树视图字体
 ├─ AutoPage.cpp       自动翻页定时器控制
 ├─ AaronMod.cpp/.h    本分支定制集中地：Ctrl+H、鼠标侧键4/5、书签删除/列表渲染
 ├─ Book (基类, Book.cpp)        打开/关闭书、后台线程解析、章节表、书签
 │   ├─ TextBook (TextBook.cpp)  txt：编码探测 + 章节标题识别
 │   ├─ EpubBook (EpubBook.cpp)  epub：miniz 解压 + libxml2 解析 OPF/NCX/文本、封面
 │   ├─ MobiBook (MobiBook.cpp)  mobi：libmobi 解析
 │   └─ (Book 继承 Page)
 ├─ Page (Page.cpp)              v2.0 重写的分页排版引擎：行/页信息、翻页滚动、进度、封面
 ├─ Cache (Cache.cpp)            进度与设置持久化：JSON 明文(Jsondata.cpp) + 简单加密
 │                               存 .cache.dat（隐藏文件），header_t + item_t[]
 ├─ Jsondata (Jsondata.cpp)      header_t/item_t <-> cJSON 序列化（书源字段已删）
 ├─ Upgrade (Upgrade.cpp)        版本检查（已停用：ENABLE_NETWORK 未定义时不编译；WinHTTP 实现）
 ├─ Keyset (Keyset.cpp)          快捷键/热键自定义（g_Keysets 表 + 设置对话框）
 ├─ DisplaySet/Advset            显示设置 / 高级设置(目录解析规则) 对话框
 ├─ Editctrl (Editctrl.cpp)      编辑模式（当页文本编辑保存）
 ├─ DPIAwareness                 高DPI适配
 ├─ Utils (Utils.cpp)            编码转换、拖拽、注册表默认打开方式等
 └─ dump (dump.cpp)              崩溃 mini dump + TEST_MODEL 日志
```

- 消息驱动：键盘/鼠标 → WndProc → KS_KeyDownProc(按键表) / AM_KeyDown 等钩子 → OnPageUp/OnHideBorder/OnFullScreen 等；翻页即移动 Page 当前位置，触发 `WM_PAINT` 重绘。
- 打开书流程：`OnOpenBook` → 按扩展名 new TextBook/EpubBook/MobiBook → 后台线程解析 → `WM_OPEN_BOOK` 回主线程 → Cache 恢复进度（含每本书独立显示设置的应用）→ loading.gif 动画直到完成。
- 每本书独立显示设置：item_t 的 use_display/font/font_color/bg_color；`_update_data` 在每次落盘前把当前字体/颜色路由到 item 覆盖或全局快照，Exit 时恢复全局再写缓存。
- 自定义消息：`WM_NEW_VERSION`/`WM_UPDATE_CHAPTERS`/`WM_OPEN_BOOK`/`WM_BOOK_EVENT`/`WM_SAVE_CACHE` 等（types.h）。
- 菜单章节列表动态创建，命令 ID 范围见 types.h（`IDM_OPEN_BEGIN..END` 等）；目录 UI 是 TreeView 弹窗。

## 关键文件速查

| 文件 | 作用 |
|---|---|
| `Reader/Reader.cpp/.h` | 主程序：窗口、消息、菜单、绘制入口、自动翻页、透明度、无边框/全屏、Ctrl+H、鼠标侧键 |
| `Reader/Book.cpp/.h` | 书籍抽象基类 + 后台打开线程 |
| `Reader/TextBook.cpp` | txt 解析 |
| `Reader/EpubBook.cpp` | epub 解析 |
| `Reader/MobiBook.cpp` | mobi 解析 |
| `Reader/Page.cpp` | 分页与排版核心算法（v2.0 重写） |
| `Reader/Cache.cpp` + `Jsondata.cpp` | 设置/进度持久化（.cache.dat，JSON+混淆） |
| `Reader/Upgrade.cpp` | 版本检查（当前停用，ENABLE_NETWORK 未定义时编译为空） |
| `Reader/Keyset.cpp` | 按键自定义系统 |
| `Reader/types.h` | 公共类型、宏、自定义消息、cache 结构 |
| `Reader/Reader_zh-cn.rc` | 编译进exe的中文资源（Reader.rc 为英文遗留，未编译） |
| `opensrc/` | 第三方：cjson、miniz、libmobi（源码）/ libxml2（静态库） |
| `tool/Upgrader/` | MFC 升级器子项目（上游已删，仅历史遗留说明） |

## 编码约定与注意事项

- 代码风格：Win32 匈牙利命名（`hWnd`、`m_XXX`、`On*` 消息处理函数、`_XXX` 全局变量），Tab 缩进，基本无注释——改动请跟随现有风格。
- Unicode 构建（`CharacterSet=Unicode`），字符串注意 `TCHAR/WCHAR`。
- 不使用异常/Rtti；多线程用 `_beginthreadex` + `ForceKill`/超时强杀等待退出。
- 绘制全走 GDI/GDI+（背景图、loading gif、文本、取色器），无皮肤框架。
- 改 header_t/item_t 结构时必须同步改 `Jsondata.cpp` 的序列化字段（cache 是 JSON，字段缺省容忍，但结构字段要与 json 类对齐）。
- x64 配置 `ConformanceMode=true`：新代码注意 `goto` 跳过变量初始化会报 C2362。

## 运行时行为

- 首次运行生成隐藏文件 `.cache.dat`（exe 同目录），删除即恢复默认/清空文件列表。
- 快捷键/翻页模式/热键等用户文档见根目录 `README.md` 与 `readme.txt`。
