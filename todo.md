# TODO

基于 2026-10 代码现状的改进清单。状态：`[ ]` 未开始 / `[x]` 完成 / `~~删除线~~` 放弃。

## 快速改进（高价值低成本）

- [x] 1. **CI 自动构建**：push `aaron` 构建 x86+x64 + artifacts；打 `v*` tag 自动 release（已验证跑通，`fab63d2`）
- [x] 2. **修 MobiBook.cpp C4244 警告**（`b67ce46`）
- [x] 3. **删除英文遗留资源 Reader.rc**（`8910dd6`）
- [x] 4. ~~**WinHTTP 恢复证书校验**~~ → 已选方案 b：**版本更新检查整体移除**（vcxproj 去掉 `ENABLE_NETWORK`，成为完全离线版；Upgrade 代码以 `#ifdef` 保留源码级，version.json/IDD_UPGRADE 资源已删）。恢复严格校验的方案作废
- [x] 5. **书签增强**：书签列表显示"章节标题 | 当前行摘要 (进度%)"，按需从文本推导，无存储结构变更（`61261a7`）

## 中期改进（中成本）

- [x] 6. **缓存版本迁移**：确认 JSON 解析本就字段级容忍（设置/进度跨版本保留）；补上"解析失败先备份 .cache.dat.bak 再重写"防丢数据（`16a2e7c`）
- [x] 7. ~~**打开书加速**~~ → 不适用：上游 v2.0 已移除 MD5，改按文件名查缓存，无冗余哈希开销（过时注释已清理）
- [x] 8. **依赖库升级重编**（三个子提交）：
  - `cc9786c` 移除 zlib 1.2.8（epub 改走工程内 miniz 路径，CVE 面直接清零）
  - `ee761a0` libxml2 2.9.10 → 2.11.9（官方源码 nmake 重编 x86/x64 静态 /MT）
  - `2c70d3e` libmobi 0.12 源码化编译（去预编译 /MD 库与 MSVCRT ignore hack；关键开关 `USE_MINIZ`+`USE_LIBXML2`+`USE_XMLWRITER`，缺最后一个会导致 mobi 打不开）
- [x] 9. **拆分 Reader.cpp**（`953a97d`）：3680→~2860 行，新增 Globals/Dialogs/Gfx/WindowUtil/AutoPage 五个模块，全局变量 extern 化
- [x] 10. **定制点集中化**（`b0af12c`）：AaronMod.cpp/.h 收纳 Ctrl+H、鼠标侧键、书签删除/列表渲染，Reader.cpp 只留一行钩子
- [x] 11. **统一 x86/x64 工程配置**（`25449a2`）：Win32 开 ConformanceMode，全部 size_t 收窄警告清零（vendored libmobi 用按文件禁用）
- [x] 12. **每本书独立显示设置**（`0206af1`）：item_t 存字体/前景/背景覆盖，文件列表右键切换"使用独立显示设置"，_update_data 在每次落盘时路由归属，退出恢复全局

## 长期方向（高成本）

- [ ] 13. **渲染升级 DirectWrite/D2D**：GDI 逐行绘制无亚像素抗锯齿、无硬件加速；换来清晰度与滚动性能，绘制代码需全重写
- [ ] 14. **TTS 朗读**：Windows SAPI 实现，摸鱼听书场景
- [ ] 15. **epub 富内容**：目前纯文本 + 封面一张图；图片/样式需新渲染层（依赖 13）
- [ ] 16. **测试体系**：分页（Page）/章节解析是纯逻辑，补单测进 CI；PageCache 有 `TEST_MODEL` 遗迹可参考

## 清理项（顺手）

- [x] 17. 移除入库的 IDE 杂物：`Reader.vcxproj.user`、`Reader/x64/Release/` 中间产物（`bd07797`）
- [x] 18. 简化构建脚本：删作者机器专用的 `rebuild.bat`（networkless/repstr 流程已无意义），`build-local.bat` 头注释刷新（`bd07797`）
- [x] 19. 清理 `ENABLE_TAG=0` 死代码：tagset.cpp/h、全部 #if ENABLE_TAG 块（保留活跃 else 分支）、资源与 ID（`fce74b8`）
- [x] 20. ~~`Upgrade` 线程优雅退出~~ → 不适用：完全离线版 Upgrade 不参与编译（`#ifdef ENABLE_NETWORK` 保留源码级；若未来恢复版本检查再一并处理 TerminateThread）

## 已完成

- [x] 完全离线：移除版本更新检查（todo#4 方案 b，去 `ENABLE_NETWORK` 宏 + 删 version.json/升级对话框资源）
- [x] 同步上游 v2.0.0.5 并移除在线小说/Web 阅读（`2c4bbc6`）
- [x] 关于对话框精简、移除代理设置、书签 Del/右键删除（`d6b44e8`）
- [x] 菜单分隔线残留修复（`e88cdf8`）
- [x] Release v2.0.0.5-aaron 发布（tag `v2.0.0.5-aaron`）
