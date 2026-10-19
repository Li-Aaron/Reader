# TODO

基于 2026-10 代码现状的改进清单。状态：`[ ]` 未开始 / `[x]` 完成 / `~~删除线~~` 放弃。

## 快速改进（高价值低成本）

- [ ] 1. **CI 自动构建**：现有 workflow 只在 codex 分支触发；改为 push `aaron` 自动构建 x86+x64 并上传 artifact，打 tag 自动发 release
- [ ] 2. **修 MobiBook.cpp C4244 警告**：float→int 隐式截断，加显式转换
- [ ] 3. **删除英文遗留资源 Reader.rc**：未参与编译却需同步维护，删文件 + filters 清理
- [x] 4. ~~**WinHTTP 恢复证书校验**~~ → 已选方案 b：**版本更新检查整体移除**（vcxproj 去掉 `ENABLE_NETWORK`，成为完全离线版；Upgrade 代码以 `#ifdef` 保留源码级，version.json/IDD_UPGRADE 资源已删）。恢复严格校验的方案作废
- [ ] 5. **书签增强**：书签目前只有位置 int 数组，列表只显示位置；改为带标题（章节名 + 上下文摘要），涉及 `item_t`/`Jsondata.cpp`/书签 TreeView

## 中期改进（中成本）

- [ ] 6. **缓存版本迁移**：header_t 结构一变就整体重置，用户丢全部进度；利用 JSON 字段容忍性做增量迁移
- [ ] 7. **打开书加速**：`Book::CalcMd5` 每次全量读文件算 MD5，大书卡顿；改为"文件大小+mtime 先查缓存，未命中再算"
- [ ] 8. **旧依赖库升级重编**：zlib/libxml2/libmobi 静态库为 2019-2022 产物，有已知 CVE；升级源码重编 x86+x64
- [ ] 9. **拆分 Reader.cpp**：约 3700 行，对话框/菜单/翻页逻辑拆独立 cpp
- [ ] 10. **定制点集中化**：私有改动（Ctrl+H、鼠标侧键、书签删除）分散在 Reader.cpp；抽成独立文件 + hook，降低未来 merge upstream 的冲突面
- [ ] 11. **统一 x86/x64 工程配置**：Win32 `ConformanceMode=false` 而 x64 为 true；统一开启并清警告
- [ ] 12. **每本书独立显示设置**：目前字体/配色全局共享；改全局+例外或 item 级

## 长期方向（高成本）

- [ ] 13. **渲染升级 DirectWrite/D2D**：GDI 逐行绘制无亚像素抗锯齿、无硬件加速；换来清晰度与滚动性能，绘制代码需全重写
- [ ] 14. **TTS 朗读**：Windows SAPI 实现，摸鱼听书场景
- [ ] 15. **epub 富内容**：目前纯文本 + 封面一张图；图片/样式需新渲染层（依赖 13）
- [ ] 16. **测试体系**：分页（Page）/章节解析是纯逻辑，补单测进 CI；PageCache 有 `TEST_MODEL` 遗迹可参考

## 清理项（顺手）

- [ ] 17. 移除入库的 IDE 杂物：`Reader.aps`、`.vcxproj.user`
- [ ] 18. 简化 `build-local.bat`：networkless 构建步骤（repstr 删宏）已无意义
- [ ] 19. 清理 `ENABLE_TAG=0` 死代码（tagset.cpp 等编译但永不生效）
- [ ] 20. `Upgrade` 线程退出用 `TerminateThread` 强杀（上游遗留）；改事件通知优雅退出

## 已完成

- [x] 完全离线：移除版本更新检查（todo#4 方案 b，去 `ENABLE_NETWORK` 宏 + 删 version.json/升级对话框资源）
- [x] 同步上游 v2.0.0.5 并移除在线小说/Web 阅读（`2c4bbc6`）
- [x] 关于对话框精简、移除代理设置、书签 Del/右键删除（`d6b44e8`）
- [x] 菜单分隔线残留修复（`e88cdf8`）
- [x] Release v2.0.0.5-aaron 发布（tag `v2.0.0.5-aaron`）
