# ExistOS-For-HP39GII 变更记录

**维护说明**: 每次版本发布后更新本文件，并升级版本号。格式遵循 [Keep a Changelog](https://keepachangelog.com/) 风格。

---

## [build 129] - 2026-09-01 (开发中)

### 进行中
- 阶段 B/C/D 安全加固（CDC flash 最小防御、VM 沙箱、FTL 等）待实施
- KhiCAS 全量帮助中文化（约 800 条）待实施

---

## [build 128] - 2026-09-01 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-128))

### Bug 修复
- KhiCAS: 修复菜单栏法语残留（"Fich" → "File"）
- KhiCAS: 修复 Help 系统语言映射（中文误识别为法语）
- KhiCAS: 修复 init_locale() 强制覆盖语言设置
- KhiCAS: 修复 main.cc 启动时 lang=0 重置
- KhiCAS: 修复多处残留法语文本
- KhiCAS: **菜单中文空白** — kdisplay.cc 剩余 132 处 UTF-8 中文全部转为 GBK \x 转义（PrintXY/HZK16S 字库要求 GBK，UTF-8 无法渲染）；修复 \xCFA2 hex 转义贪婪吞并 bug（kdisplay.cc:1891）
- KhiCAS: **config 菜单顺序错位** — 插入"中文/English"菜单项后 items 数组后移，处理代码未同步：selection 4/5 语义错位（点"中文"变英文、点"English"无反应、Sqrt 开关失效），已按 items 实际位置修正（selection 4=Sqrt、5=中文、6=English）；移除不可达的重复 selection==3 分支
- KhiCAS: **quit 退出卡住** — kdisplay.cc kcas_main 末尾 `for(;;) GetKey()` 死循环删除（此前只修了未编译的 main.cc 版本，无效）；Console_GetLine 返回 NULL 时保存会话并正常退出
- KhiCAS: MB_ElementCount 支持 GBK 双字节字符计数（菜单填充/反白条宽度计算正确）
- KhiCAS: 删除 doMenu 与 config 菜单中的调试 printf 刷屏（每帧 16 行）
- KhiCAS: 语言持久化生效 — load_lang_setting() 在 kcas_main 中 restore_session 前调用（此前只加在未编译的 main.cc）
- KhiCAS: 交互字符串法语残留清理（确认按钮、数字位数、考试时长、堆/栈大小提示等 → GBK 中文）
- KhiCAS: 补审修复 — `\xa3\xac0` hex 转义贪婪吞并（中文逗号后接 ASCII 0 被解析为 0xAC0）
- KhiCAS: config 菜单 Quit（selection 15）由 break 改为 return——选 Quit 真正退出配置菜单（原会回到菜单循环）
- KhiCAS: 会话菜单 "Enregistrer" 法语残留 → "保存"；考试时长提示 "Mode examen." → lang 分支
- KhiCAS: khicas_stub.cpp MB_ElementCount 与 kdisplay.cc 对齐（GBK 双字节计数）
- KhiCAS: **菜单 quit 后 UI 残留** — khicasTask 收尾补 vGL_clearArea 清屏 + "Quitting.../Waiting session save..." 提示 + khicasRunning/keyStatus 清理 + 1s 等待，与 shift+ON 退出序列（stub GetKey case KEY_ON）一致，home 正常显示
- KhiCAS: **中文无法显示** — HP39 文本渲染全走 vGL_putString→vGL_putChar（仅 ASCII 字库），GBK 高字节查 ASCII 字库返回空白。修复：vGL_putString 识别 GBK 双字节（0xA1-0xF7+0xA1-0xFE）、x 步进 16px；新增 vGL_putChar16 按 HZK16 区位索引渲染 16x16 点阵（字库经 Script/sys_ld.script .rodata 段 fonts_hzk_start 链接，flash XIP 不占 RAM）；菜单选中箭头 \xe6\x9b（UTF-8 残片）→ GBK \xa1\xfa(→)
- KhiCAS: **Cmd/File 菜单及子菜单汉化补全** — kadd.cc 金融子菜单（贷款/储蓄/年利率/年付款次数/月供/当前应付款额等专业术语，原 Epargne/Mensualite/Somme due 等法语）、分形/多项式子菜单（根盆地/多项式/数字位数，原 bassins racines/Polynome 等法语）、Cmd 主菜单（原 Epargne/Table caracteres/Exemple 系列/Quitter 法语）、电子表格（Reeval/Quit/Config）、Flash Files 标题"Flash 信息"→"Flash 文件"、Mastermind 英文分支法语→英文；file.cc "No Data"→"无数据"
- Libs: file.cc `"\x000\x000"` hex 转义歧义修正为 `"\x00\x00"`

### 待完成
- catalog 命令目录法语帮助文本中文化（kdisplay.cc 约 150 条，量大）
- REQ-SEC-002: USB 操作限制（回滚后重设计，不影响 Updater 刷写）
- REQ-BUILD-005: CI 供应链安全加固
- 内存优化：中文字串增加固件体积（500K+ → 276K 目标）

---

## [build 127] - 2026-08-30 (已发布)

### 新增功能
- KhiCAS 中文化：删除法语，lang=0 英语，lang=1 中文（默认）
- 约 300 条界面字符串翻译为中文
- 新增 khi_i18n.h 翻译辅助头文件
- 更新 keymap.md（补充说明书 SHIFT/ALPHA 功能、各视图按键功能）

### Bug 修复
- 回滚 REQ-SEC-002 块限制（影响 ExistOS Updater 图形工具刷写）

---

## [build 126] - 2026-08-30 (已废弃,块限制导致写入失败)

### 安全修复
- REQ-SEC-001: LL_SWI_GET_CONTEXT 增加地址验证
- REQ-SEC-002: 限制 CDC ERASEB/PROGP 仅允许操作数据区（block>=160）

### Bug 修复（25项）
- OSLoader: CDC 缓冲区越界、VM_Unconscious strcat、msc_disk 边界
- OSLoader: 日志缓冲区溢出、mem_cr 累加重置、赋值嵌入条件判断
- OSLoader: DisplayPutStr 内存泄漏、llapi 上下文保存边界检查
- System: PrintTask 栈 400→1024、VROMLoader 缺 return、getentropy 种子
- System: SystemConfig snprintf、bytes_read 顺序、UICore 修复
- System: SysIRQ 中断 printf 移除、MMU case 补 break
- Libs: jpgViewer/mjpegPlayer sizeof(vrambuf)、memory.c 泄漏

### 移除
- 删除 EMU48 Saturn 模拟器（22个文件，约25000行代码）
- 删除按键收集工具 keytest App

---

## 历史版本 (build 125 及之前)

> 以下版本由上游维护：[ExistOS-Team/ExistOS-For-HP39GII](https://github.com/ExistOS-Team/ExistOS-For-HP39GII)

- **build 125**: 基线建立，全量代码审核（121个问题）
- **build 70**: Emu48、KhiCAS 脚本编辑器、JPEG/MicroPython、崩溃日志
- **build 1**: 初始版本（OSLoader、FreeRTOS、虚拟内存、KhiCAS、USB）