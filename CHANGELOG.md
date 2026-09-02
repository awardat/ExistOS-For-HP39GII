# ExistOS-For-HP39GII 变更记录

**维护说明**: 每次版本发布后更新本文件，并升级版本号。格式遵循 [Keep a Changelog](https://keepachangelog.com/) 风格。

---

## [build 131] - 2026-09-02 (开发中)

### 规划
- RPN39 阶段 2：科学计算（MATH 菜单 + DEG/RAD）
- RPN39 简单函数绘图（阶段 3）
- KhiCAS 全量帮助中文化（约 800 条）
- D4 FTL_Sync 真机掉电测试

---

## [build 130] - 2026-09-02 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-130))

### 新增
- **RPN39 RPN 计算器**（42S 基准，见 docs/RPN39-design.md）：
  - 4 层栈 + 基础四则（ENTER 压栈/42S 自动栈提升/x<>y/R↓/DROP/CLx）
  - **寄存器管理**：26 个 A-Z 寄存器（STO `(` / RCL `)` 等字母键；Vars 列表选择；Shift+backspace 清空全部）
  - **掉电持久化**（/rpn39_sto.dat：寄存器 + 栈——HP 关机保留语义）
  - 字体：Fira Code 24px（0 斜杠）+ ttf2c.py 字体转换工具（OFL 许可）
- **KhiCAS**：
  - ON/C 短按 = 返回/取消（原厂语义；AC 移至 Shift+backspace）
  - 菜单内 Shift+View 打开当前菜单项帮助（FKEYS 类菜单等同 F6/Help）
  - 模态输入（inputline）返回后清按键状态（修复保存 session 后 quit 卡住）
- **E 组整改**：msc WRITE10 边界、getSuffix 三缺陷、路径拼接有界、MB_ElementCount 与渲染对齐、CrashLog SP 校验、链接器 ASSERT、主机工具修复、UI_Task 栈 800→2048（栈溢出检查重新启用）
- **构建卫生**：-Wall/-Wextra + 栈保护 0 警告；刷写支持 OSLoader/ExistOS 单独更新
- 审核报告：build130 增量审核 + 用户决策 + 修复引入回归记录（docs/）

### 发布惯例
- **每次发布 OSLoader.sb 与 ExistOS.sys 一并附上**（无论单版本是否改动）

---

## [build 129] - 2026-09-02 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-129))

### ⚠️ 风险警告
- **加速模式（480MHz）超出 STMP3770 文档上限（380MHz）**：可能导致**耗电剧增、发热、设备损坏**。请谨慎使用，长时间使用建议标准（240MHz）或省电（160MHz）模式
- 320MHz（FRAC 分频）真机两次尝试失败（频率异常/挂死），已回退搁置，详见 docs/boost-320-frac-issue.md

### 新增
- **电源三档变频**：标准 120~240 / 省电 80~160 / 加速 240~480（空闲自动降频，空闲 WFI 省电）
- **安全加固**：
  - CDC flash 命令参数校验（ERASEB/PROGP/MKNCB：解析失败拒绝、块/页上界检查，不再有危险默认值）
  - VM 沙箱收紧：低 512K 仅读（防 VM 写内核区）；FLASH_PAGE_READ 权限方向修正；SET_CONTEXT/MEM_PHY_INFO 指针验证
  - 日志缓冲竞态修复（数据先写、索引后更 + 消费侧快照）
- **FTL_Sync 入队**：文件系统同步与 FTL 读写队列串行（消除映射并发竞态，掉电保护更可靠）
- **ZRAM OOM 降级**：压缩池满不再 NULL 崩溃（与 MINILZO 分支对齐返回错误）
- **GBK 字形查找统一**：gbk16_glyph() 统一范围/边界检查（修复 UICore.h 下溢与 vGL 越界），两套渲染后端共用
- **构建卫生**：-Wall -Wextra 告警清零、-fstack-protector-strong 栈保护、清理 .bak 残留

### 修复
- **ON/C 键承担取消/后退**（HP 原版语义）：KhiCAS ON 短按=取消（AC，菜单里=返回、编辑器=清空）；UI ON 短按=返回主页面；Shift+ON/C 关机不变
- View 键释放（系统/新计算器无定义）；KhiCAS 保留切换公式输入
- KhiCAS: 清除历史/清除变量确认框法语汉化（"Effacer l'historique?"→清除历史?、"OK: oui, Back: conserver"→OK: 是, Back: 保留、"Effacer variables?"→清除变量?）
- 键位表 ON 统一为 ON/C（原版 ROM 开机后为取消键；口语仍称 ON）

### 待完成
- KhiCAS 全量帮助中文化（约 800 条）
- RPN39 计算器（42S 基准：4 层栈 + 四则 → 科学 → 绘图）
- D4 FTL_Sync 真机掉电测试

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