# ExistOS-For-HP39GII 变更记录

**维护说明**: 每次版本发布后更新本文件，并升级版本号。格式遵循 [Keep a Changelog](https://keepachangelog.com/) 风格。

---

## [build 133] - 2026-09-04 (开发中)

### 新增（2026-09-04 充电/功耗/显示修正）
- **充电固定镍氢**（锂电直插实测失败——AAA 外围电路短路拉低电压；UI 类型选择隐藏、调用点固定镍氢、STOP_ILIMIT=0 + 12h 定时停充；ALKALINE_CHARGE 语义修正为 boost 电感效率位）
- **充电状态显示修正**：GET_CHARGE_STATUS 由软件开关改为硬件实际充电路径（电池满后 DCDC 关，不再误显"充电中"；充满自停已实测——4 并镍氢 1.4V 触发 1420mV 阈值）
- **UI 空闲 CPU 28%→1%**：页面信息刷新 900→3000ms、按键轮询 20→30ms（时钟/状态不常变）、控制台光标独立 1s 节奏；测试保持 28% 恒定 10 分钟
- **ON+{+,-} 对比度调节修复**：key_task_capt 状态 0 增加 ON 物理按住检测（矩阵扫描顺序导致组合键先入队时失效）
- **vTask1（Status Print）忙循环修复**：50ms delay + printTaskList 30s 降频（原空转占 CPU 且串口洪水）
- **CDC 会话任务恢复**：PING 挂起的监控任务（vBatteryMon 等）在 DTR 断开时自动恢复（免重启）
- **KhiCAS 帮助中文化**：completeCatzh 全量 355 条（260 条 desc 中文 GBK；XCAS_ONLY 保留原文）；lang=1 切换中文目录；**乱码根治**——textArea print() 宽度推进按字节×7（中文 14px）与 16px 渲染漂移 2px/字导致 ASCII 叠压中文（p 盖式/p= 盖键）→ 改 hp39_text_width（GBK=16px）推进；帮助/只读视图 ON=返回（View 不承担返回，编辑态语义保留）；charmap/脚本保存类型/trace 加 ON 等效退出；命令目录一级分类补译 5 项（算术加密/复数/选项/矩阵/三维）
- RPN39 手册/README 硬件实测记录（锂电失败、整机电流基线、AAA 电池建议）

### 规划
- ~~RPN39 阶段 3 测试~~（✅ 2026-09-05 用户确认 docs/RPN39-phase3-test.md 用例已过）
- KhiCAS：物理按键映射加强；示例功能补充；giac 2.0.0 有限移植
- FormCalc（表单计算：财务 12C 全集 + 工程）规划
- D4 FTL_Sync 真机掉电测试
- C 组待办其余项

---

## [build 132] - 2026-09-04 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-132))

### 新增（2026-09-04 电源/充电/性能 + RPN39 阶段 3）
- **电源两档制**：实测省电档与标准档待机电流差小（40-50 vs 50-60mA），去除省电档——标准 240/120 ↔ 加速 480/240；旧 'S'/'L' 配置按标准档
- **HCLK 240→120MHz**（手册明确 HCLK≤200MHz；CPU 走 PLL 独立分频不受影响）+ **空闲 PFM/DC_HALFCLK 轻载省电** + LOOPCTRL 手册推荐位（TOGGLE_DIF/EN_CM_HYST/EN_RCSCALE=1）
- **充电**：启用锂电 STOP_ILIMIT 停充（锂电靠限流停；镍氢 12h 定时已有）；配置页充电类型选择（锂电/镍氢，KEY_3 切换、持久化、启动应用、SWI +86）；开关与类型同行布局（EN 文案缩短）；**实测电流基线**：待机 省电 40-50/标准 50-60/加速 90mA（1.6V）
- **KhiCAS 待机电流优化**：vGL_getkey 空闲轮询加 5ms 延时（让出 CPU→idle 降频；KhiCAS 待机 80-90mA → 预计 ~50mA，按键响应无感知损失）
- **手册核对结论**：STMP3770 无 CPU 频率上限（480MHz 仅 USB 480Mbps 与"PLL 恒 480"，CPU div1 自然 480；"380MHz 限制"不成立）；唯一明确限制 HCLK≤200MHz
- **RPN39 阶段 3**（测试清单另记）：
  - 百分比 % / Δ% / %T（MATH 页 1 下三槽：12C 语义只改 X 保 Y——可直接 + 得价税合计；R→P 辐角 atan2(im,re) 修正）
  - STO 运算（42S：STO 态 Shift+四则 = STO+−×÷，R op= X 存回；Shift 区分不破坏 n/s/w 字母寄存器）
  - 复数 CPLX（Shift+,）：：方案 A 复寄存器 Z1/Z0（re+im）；3 子页——载入（主栈 Y,X）/四则、共轭/模/倒数/辐角/输出主栈/RECT↔POLAR 显示、R→P/P→R 实数坐标转换/交换/清除；持久化
  - 矩阵 MATX（Shift+4）：：A/B/R 槽 4×4 方阵格编辑（方向+数字+小数+负号+退格）；加减乘/转置/求逆（高斯-约当）/行列式（→主栈 X）/SIZE/R→A/B 链式；持久化
  - 统计 STAT（Shift+7 + 主界面 F4=Σ+/F5=Σ−）：8 统计量（n/均值/总体·样本标准差/最值/和/平方和），↑↓+ENT 取值→主栈；n≤256 持久化
- 结构：rpn39 拆出扩展模块（rpn39_int.h + rpn39_ext.cpp，aux_source_directory 需重 configure）
- **RPN39 用户手册**（docs/RPN39-manual.md：功能/用法/示例 15 章，随发布）

### 规划
- **RPN39 阶段 3 详规**（2026-09-03 更新）：统计（MATH 页 + Shift+7）、矩阵（MATH 页 + Shift+8）；财务/求解器已移出（见下方新程序）
- **新程序：表单计算（FormCalc）**（规划中，2026-09-03 更新）：财务（12C 经典功能全集：TVM/现金流/摊销/债券/折旧/日期/利率转换/利润）+ 工程（单位换算等），表单 UI；**求解器已移出**（去向待定：RPN39 阶段 3 或独立）；依据与参考：free42/WP43 等 ref 仓库（功能清单汇总）
- **KhiCAS**：~~App 菜单验证修复~~ → **该功能已整体移除**（2026-09-03：内含不完整示例；F5 留空、原 APPS 键已置空）
- **KhiCAS**：帮助文本中文化（约 800 条）；加强物理按键映射；视图按键完整方案（HOME/VIEWS 后续定义）；giac 桌面版（2.0.0）有限移植
- **KhiCAS 示例功能**（新待办）：App 菜单移除后，计划补充完整示例功能（Syracuse/Mastermind/分形等完善后重新加入——先前版本不完整）
- **金融/工程计算去向**（待定）：RPN39 阶段 3 或独立新程序（未来单开），暂不入 KhiCAS
- D4 FTL_Sync 真机掉电测试
- C 组待办：OSLoader 侧栈溢出检查开启（=2，需刷 OSLoader 验证）；字体母本唯一权威源标注；RAND 种子策略

---

## [build 131] - 2026-09-03 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-131))

### 新增（RPN39 阶段 2：科学计算）
- **角度模式**：DEG/RAD/GRAD（标题右上显示；持久化——/rpn39_sto.dat 第 31 值，旧文件兼容）
- **直接键位函数**（原厂键位对齐）：SIN/COS/TAN（按当前角度模式）、LN/LOG、X²、x^y（Y^X）、a b/c 小数↔分数显示切换（X 行连分数，1e-12 阈值，转不出保持 12 位小数）
- **Shift 层**：ASIN/ACOS/ATAN、e^x、10^x、√X、Y 次根（x^(1/y)）、1/X、X!（tgamma）、π（Shift+3）、**ABS（Shift+`(-)`）**、CLx（Shift+backspace）、退出（Shift+ON）
- **MATH 菜单**（5 页）：角度 / 常量基础（e/sign/round/floor/ceil）/ 双曲（sinh…atanh）/ 概率（nCr/nPr/RAND）/ 幂（x³/10^x/2^x）；**循环翻页**（第 1 页左跳第 5 页）+ **数字键 1-5 跳页**；标题**中英文对照**（GBK 混排）
- **EEX 科学计数输入**（X,T,θ,N 正常层；指数区负号 CHS 翻转）
- 三角近零噪声舍去（90° COS → 0）；运算 NaN/Inf 防护（→0）
- backspace 非输入态从当前 X 开始编辑（a/b/c 往返后仍可退格）
- 构建环境修正：**主系统须从仓库根构建**（顶层 CMakeLists；AGENTS.md 已更新）

### 规划
- **RPN39**：阶段 3 计算功能扩展（候选：复数/矩阵/统计/金融/工程常量——已取消绘图）
- **KhiCAS**：帮助文本中文化（约 800 条）；加强物理按键映射；**去除 Home→file-quit**（已完成，HOME 释放）+ **重新定义 6 个视图按键**（规划中）；giac 桌面版（2.0.0）有限移植
- D4 FTL_Sync 真机掉电测试
- **C 组待办**（build 132）：~~OSLoader 侧栈溢出检查开启（=2）~~（已移除：2026-09-03 实测导致刷写失败已永久回滚为 0）；字体母本唯一权威源标注（tools/ vs System/graphics/）；RAND 种子策略（固定池或文档化）
- **队列（暂不做）**：视图按键完整方案（HOME 已释放、SYMB/PLOT/NUM/APPS 已置空、F5=App——剩余 HOME/VIEWS 等的后续定义）；App 菜单内应用功能验证修复（见 kadd.cc）

---

## [build 130] - 2026-09-02 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-130))

### 新增
- **RPN39 RPN 计算器**（42S 基准）：
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
- 审核报告：build130 增量审核 + 用户决策 + 修复引入回归记录

### 发布惯例
- **每次发布 OSLoader.sb 与 ExistOS.sys 一并附上**（无论单版本是否改动）

---

## [build 129] - 2026-09-02 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-129))

### ⚠️ 风险警告
- **加速模式（480MHz）超出 STMP3770 文档上限（380MHz）**：可能导致**耗电剧增、发热、设备损坏**。请谨慎使用，长时间使用建议标准（240MHz）或省电（160MHz）模式
- 320MHz（FRAC 分频）真机两次尝试失败（频率异常/挂死），已回退搁置（FRAC 分频不再使用）

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