# ExistOS-For-HP39GII 变更记录

**维护说明**: 每次版本发布后更新本文件，并升级版本号。格式遵循 [Keep a Changelog](https://keepachangelog.com/) 风格。

---

## [build 144] - 开发中

### 已完成
- （待补充）

## [build 143] - 2026-09-22 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-143))

### 已完成
- **原厂固件定向逆向**（`docs/hp-firmware-re-analysis.md`）：解包 firmware.sb（主镜像明文、引导段加密）；原厂 NAND 结构源码级答案（NCB/LDLB/DBBT 格式 + 块布局 0/4/8/12/16/19）；CP15 直方图揭示原版 cache lockdown + TTB/FCSE，解释 80 MHz 仍流畅；原版 PPL 为编译型、CAS 语法编译进固件但未开放
- **坏块 RAM 状态缓存**（`dhara_nand_is_bad`，每块 2 bit；原厂 master BB 表思路）——真机验证通过：开机体感变快、KhiCAS 退出保存变快
- **Python app**：符号面板改用局部刷新（方向键不再全屏重绘、消除 console 闪现）并显示自己的底栏（选择/取消/上翻/下翻）；底栏 F4「运行」取消留空，「运行脚本」移入 **F6 文件**菜单；文件根目录从 `/xcas/` 迁到 **`/python/`**（首次启动自动创建并迁移旧文件）
- **B2 实验与结论**：`/python/pyheap.cfg` 可配置 GC 堆大小（KB），每次进入应用重读、变化时重建堆+解释器；实机实验（32K–512K、onchip/swap）对 N-Queens 无影响（600–624ms）→ 换页非 Python 瓶颈；据此**默认堆改为从大到小尝试（512K 起）**，大程序有空间、未用部分不占内存不换页
- **配置持久化修复**：JSON 解析器取值遇换行不停止，导致最后一个键 `enable_mem_swap` 解析为 `"true\n"` 而恒为 false（保存正常、重启丢设置）——修复后 MEM SWAP 保持生效
- 发布前清理全部临时诊断输出（串口 `[PY]`/`[CFG]` 与启动堆信息行）

## [build 142] - 2026-09-21 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-142))

### 已完成
- **Python app 执行模型改为「单元格」**：ENT 只换行（行尾 `:` 自动缩进 4 空格、空行 ENT 回行首结束块），**F5=run** 执行自上次运行以来输入的全部内容；逐条语句执行（单行用 MicroPython single 模式**回显结果值**，带缩进块整体 exec）；编辑（←→ 行内光标/退格/Shift+退格清空/ON 清屏）
- **Python app 数学键直通 + 符号面板扩容**：SIN/COS/TAN/LN/LOG/x²/xʸ 直出 `sin(/cos(/tan(/log(/log10(/**2/**`（Shift 层给 `asin(/acos(/atan(/exp(/log2(/sqrt(/pow(`，`(` 的 Shift 为 `abs(`）；F1 面板增至 6 页（符号/运算符/括号与赋值/结构/函数/常量）；应用内帮助 5 页
- **移除媒体解码代码**：删除 lvgl `extra/libs` 下 ffmpeg/gif/png/sjpg/bmp/rlottie（含 `LV_USE_GIF` 1→0 与构建清单），保留 freetype/fsdrv/qrcode
- **MEM SWAP 扩容尝试（6MB）已回滚**：OSLoader 的 `VM_RAM_SIZE` 3MB→6MB 多占 12KB 页表（每 1MB VM 空间 4KB L2 表）→ 堆 34KB→22KB → 启动期 OOM（`!!!!OOM!!!` + 复位循环）白屏。恢复 3MB；OSLoader RAM 512KB/堆 34KB 是硬约束
- **README 新增「存储空间分布（128MB NAND）」与「为什么 MEM SWAP 固定为 3MB」**（中英双版）；进展章节只保留最新版本；删除「KhiCAS 基本使用」章节，目录改指 KhiCAS 用户手册与内置函数参考
- **DFU 恢复流程修复**：恢复时同时写入 OSLoader 与 System（原先只写 System，恢复后下次冷启动仍白屏）

## [build 141] - 2026-09-20 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-141))

### 已完成
- **应用页改为 4×1 单行**（4 个图标等距，名称 12px 居中；FormCalc 标签改 `FCalc`，为第 4 个内置应用 Python 留位）
- **文本查看器自适应**：纯 ASCII 文件用 7 行 12px 小字体；含中文（非 ASCII）文件自动切 16px 中英混排 6 行（中文可读，不再显示 `?`）；二进制文件（控制字符 >10%）显示「（二进制文件）」+ 大小提示
- **Python app 中文提示补全**：脚本打开失败/内存不足/未找到 .py 脚本 均显示中文
- **Python app 开启任意精度整数**（MicroPython MPZ，固件 +11KB）：`2**100`、`math.factorial(50)` 等大整数运算可用；`SSIZE_MAX` 工具链缺失由端口定义补齐
- **FormCalc 子网掩码计算器**（电子工程第 10 项 SUBNET）：IP 四段 + 前缀长度实时计算，显示子网掩码/网络地址/广播地址/可用主机范围/主机数（/31、/32 特例按标准处理）；↑↓ 调整、←→/ENT 切字段、F1 恢复默认；持久化 `/formcalc/subnet.dat`
- **README**：新增「内存与 MEM SWAP（重要）」说明（刷机后默认关闭、开启方法与代价）；进展章节改为按版本倒序且只保留大功能发布

## [build 140] - 2026-09-20 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-140))

### 已完成
- **独立 Python app（MicroPython v1.29.0）**：首页应用页第 4 个图标（原创图标）；终端式 REPL（6 行 × 31 字符、16px 中英混排、96 行回看、行级局部刷新）；完整键位（数字/运算符/Shift 层/ALPHA 26 字母表、Shift+ALPHA 小写锁定、←→ 行内编辑、UP/DOWN 滚动、Shift+UP/DOWN 翻页、ON 清屏、Shift+ON 退出）；底部 F 键菜单（符号面板 4 页 / 清屏 / 取消 / 运行 / 帮助 4 页 / 文件菜单 6 项）；中文报错映射（25 条常见异常）；MicroPython 文件对象（`open()` 走 FatFs）与本地 `import`（按 `/xcas/` 解析）；脚本浏览与运行（F4 或文件菜单，`/xcas/pyNN_*.py`）；会话变量保留 + 复位解释器；「保存会话」导出终端 96 行到 `/xcas/session.txt`
- **应用页 4x1 布局**（4 图标单行、名称 12px 居中）与**文本查看器**（文件管理器内打开任意文件：7 行 12px 小字体、UP/DOWN 滚动、Shift+UP/DOWN 翻页、只读）
- **文档与样本**：Python app 用户手册（`docs/Python-app-manual.md`）；样本 `py01_basics.py` / `py02_module.py` / `py07_nqueens.py`（N-Queens 8×8：标准 1221ms / 加速 703ms）
- **审核整改**：Python app 退出/重入互斥（防双任务共用解释器堆）、查看器退出清全屏、查看器缓冲按需分配（原 16KB 常驻）、GBK 占位按字符、路径截断与负偏移防护、诊断输出清理

## [build 139] - 2026-09-19 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-139))

### 已完成
- **KhiCAS `save_script` 写入 0 字节修复**（"另存为 0 字节"根因）：`save_script` 调用 `write_file` 时未传长度（默认 0），而 `Bfile_WriteFile_OS` 对 len=0 直接返回 → 文件被创建但内容为空；现传入实际长度，并在 `write_file` 加 len=0 兜底 + 写入后**按长度截断**（覆盖更短内容不再残留尾部旧数据）；新增平台 API `Bfile_TruncateFile_OS`（FatFs/littlefs 双实现）
- **FormCalc 审核 P3 四项 + 死代码清理**：AMORT 支持 **B/E**（F6，与 TVM 共用全局期初/期末；BGN 首期 INT=0）与**用户 PMT**（优先取 TVM 表单的 PMT 寄存器）；DEPREC **DB200 增加 SL 交叉**（某年直线法超过余额递减即切换，残值地板保留）；DATE 的 30/360 增加**31 日与 2 月末调整**（US/NASD）；`bondPrice` 移除死参；删除 `config_set_charge_mode` 死代码（无调用方）
- **KhiCAS 用户手册配图**：14 张实拍屏幕照片（压缩后插入 docs/images/khicas-manual/，含命令目录两张、绘图数据视图；编辑器 F1 菜单与 Console 同布局不再补拍）
- **KhiCAS 用户手册**（`docs/KhiCAS-manual.md`）：启动退出 / 界面布局 / 输入求值 / 按键总览 / 菜单（Console 功能菜单 + 文件菜单 18 项）/ 变量 / 自动补全（Shift+0）/ 配置菜单 / 脚本编辑与运行 / Python 兼容模式与命名约束 / 绘图 / 持久化 / FAQ；README 中英双版与 samples/README 已加链接

### 规划
- 独立 Python app（micropython 最新稳定版、独立入口；N-Queens 验证）

## [build 138] - 2026-09-19 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-138))

### 已完成
- **Console F1 快速插入菜单**：Console 的 F1 段由 algb 代数菜单替换为编辑器 F1 的 test 段（`if/else/</>/==/!=/and/or`；Python 模式为 `&&/||`），便于输入比较/逻辑运算符；原 algb 条目（simplify/factor/…）可通过命令目录输入
- **语法切换恢复 + 持久化**：`Shift+Symb` 直通配置菜单（原 09-03 键位清理后配置菜单只能从 F6 文件菜单第 12 项进入）；语法选择（Xcas / 兼容 Python）持久化到 `/khi_lang.dat` 第 3 个 int（旧文件向后兼容），重启保持——此前每次启动回到编译默认 Python 兼容模式
- **KhiCAS 输入自动补全**（Shift+0）：前缀匹配 355 条命令目录（completeCaten，显示中文名）+ 1552 条内置函数名（static_lexer_numworks，补齐 sqrt/sin/ln/evalf 等目录未收录命令）；输入完整命令名优先精确匹配；唯一匹配直接补全为 `命令(`（程序结构 while/if/for 插入原文本）；多匹配弹出选择菜单；无匹配不动作；Python 模式过滤 XCAS_ONLY 条目、跳过大小写别名
- **giac 2.0.0 有限移植三档（solve.cc）**：方程/不等式求解整体替换（9893→11712 行）+ `solve.h`（`gbasis_param_t` 新增 5 字段、`gbasis`/`vecteur2vector_polynome` 签名演进）；适配符号 `RUR_PARAM_MAX_DEG`/`has_undef`/`realset_glue`/`sqrfree`（内容递归）/`mrref` 排列重载/`apply_permutation` 模板/`proot` 兼容重载；编译 0 错误 0 undefined，固件 +34KB；测试用例见本地 docs/giac-solve-test.md（待真机验证）

### 说明
- README 新增「KhiCAS 运算速度说明」：部分符号运算需数秒到数十秒属算法固有开销（一次符号求值约 85ms；实测 solve(exp(x)=x^2,x) 17.4s / 加速 8.3s 等），建议耗时计算使用加速模式

## [build 137] - 2026-09-19 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-137))

### 已完成
- **KhiCAS 文本界面键位重排**：Shift+( = 剪切行到剪贴板（原 ON/C 行为迁移）、Shift+) = 粘贴剪贴板到当前行（换行转空格）、ON/C = 退出（有选中/搜索时先取消）；覆盖结果查看页/脚本编辑器，帮助页保持 ON=返回
- **KhiCAS 计算沙漏指示**：求值（`run`）前后驱动系统图标行 `INDICATE_BUSY` 位（顶部 x=82 竖条）——耗时 >50ms 的计算可见，短计算由显示任务 50ms 轮询自然过滤不闪；系统图标链路与 shift/alpha/电池一致
- **KhiCAS 小写锁定**：Shift+ALPHA 锁定小写输入（复用现有 0x80 锁定机制，按键后状态保留），再按 ALPHA 或 Shift+ALPHA 解除；原三态循环（大写→小写→退出）不变
- **giac 2.0.0 有限移植试点 A**：`sym2poly.cc` 整体替换（4545→6771 行）——新增代数数算法簇（algnum_* 正规化/rref/gcd/evalf）、RUR 表示（rur_* 多项式系统求解）、多项式约化（mreduce）、项排序增强（sort1/sort2/cleanup_pow）；编译 0 警告、链接 0 undefined（ExistOS.sys +3.4KB）；回归测试用例 37 条见 docs/giac-sym2poly-test.md（本地）

- **giac 2.0.0 有限移植一档（risch）**：`risch.cc` 整体替换（1047→1107 行）——里施微分方程塔选择优化、系数修复、对数系数处理；40 条积分测试用例通过（无回归，含奇异/不可积保护）
- **giac 2.0.0 有限移植二档（csturm）**：`csturm.cc` 整体替换（2111→4989 行）——实根隔离重写（vas/aberth/mps_solve 等）；`csturm.h` 补 aberth/mps_solve 声明；global 补常量 `ABERTH_NMAX`/`ABERTH_NBITSMAX`；适配快照 proot 三参签名；17 项测试全部正确（实根隔离含重根/数值求根/求解联动）
- **π 渲染修复**：数学排版视图 π 宽匹配（"pi"/GBK π/UTF-8 π）；Console 输出把 UTF-8 π 转 GBK π（HZK16S `A6D0` 字形）
- **中文告警修复**：`giac2aspen` 语言映射修正（中文模式此前误用法语列），aspen 中文表转 GBK
- **`(-)` 键修复**：输入 `-`（原为 `_`）
- **File 菜单"保存日志"**：Console 历史导出到串口（带 begin/end 标记）
- **Python 兼容层循环限制调研**：5 轮自检定位多分支循环体解析卡死；限制归档 docs/khicas-python-limits.md（本地）；N-Queens 样本归档 ref/nqueens/
- **KhiCAS 键位批次调整**：Symb→程序命令菜单、Plot→绘图菜单、Num→代数菜单、Apps→打开脚本列表；Shift+4=矩阵菜单、Shift+7=列表菜单；Shift+空格=∑；删除 Shift+a b/c 双引号、Shift+0 与 F10-F14 模拟映射；字符表扩为两页（ASCII / 希腊字母+数学符号，Shift+VARS 打开、上下翻页）；"剪切行"改为"复制行"（Shift+( 复制、Shift+) 粘贴）
- **KhiCAS 图形界面键位**：ON=退出、View=三态视图循环（图形→数据→表达式）、Plot=曲线分析、Home=回 Console、Num=无操作；删除闪烁状态栏提示；物理 Num/Plot 与软键 F1/F3 用专用键码解耦（F1 帮助、F3 轨迹切换恢复可用）
- **KhiCAS 脚本执行链路**：run_script 改为逐行+缩进块聚合执行（绕过多行 Python 块解析限制）；跳过首部注释/空白行（修复带 `# -*- coding` 声明脚本完全不执行）；自动 python_compat 模式；运行沙漏；编辑器 F5=保存并运行；主界面 F5=日志查看；samples/ 新增 6 个示例脚本与 README（命名约束/已知限制）
- **文件浏览器修复**：菜单 ENTER(76) 选中修复（所有菜单生效，解决 build 119 起"文件浏览器无法加载脚本"问题）；目录识别改用 FAT 属性（非空目录不再误判为文件）；枚举模式 `*.*`→`*`（无扩展名目录可见）；`screen_1bpp` 从固定地址改为堆分配（修复偶发全屏花点/全黑）
- **Console 结果双视图**：历史结果行左键=数学排版、右键=文本表达式；新增"保存日志"（File 菜单，Console 历史导出到串口）
- **build 137 delta 审核整改**：File 菜单项数 17→18（修正数组越界写、恢复"退出"项）；图形界面软键遮蔽修复；Console 日志缓冲单点收集去重；清理 5 处诊断 printf；删除空 PrintTask；file.cc 目录图标改用目录属性


## [build 136] - 2026-09-16 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-136))

### 已完成
- **KhiCAS HOME 键**：任意位置按 HOME 回到初始界面（Console 根）——EXIT 注入穿透方案（GetKey 持续返回 EXIT 利用"返回上级"契约逐级退出，Console 根消费标志停止，80 次上限安全网）
- **micropython 清理**：移除未使用的 MicroPython 1.18 源码树（36MB）与 libmpy 预编译库（CMake 从未链接、无调用点）；独立 Python app（最新稳定版）列入待办
- **FreeRTOS 调研**：上游 V11.3.1 vs 当前 V10.4.3 LTS Patch 2——官方 ARM7/9 port 已移除、V11 安全修复与本项目实际用法零命中 → 维持 V10.4.3
- **Round 4 C 组三项整改**（充电复测后处理）：① CDC 断开补 `VMResume()` + 恢复省电档——edb 非 REBOOT 退出/仅 PING 后不再永久挂起 System；② EN_RCSCALE 两处覆盖统一为手册推荐值 1（原优化空转）；③ 1.4V+2h 窗口加迟滞（<1380mV 才复位，平台 ±10mV 抖动不再重置窗口）
- **KhiCAS 命令目录中文名**：新增 `completeCatZhName[]` 显示层数组（260 条显示条目 "name 中文" 形式，XCAS_ONLY 保留原名）——name/insert/排序/搜索仍用英文原名，规避 insert==NULL 的 159 条输入污染与 strcmp 排序问题；目录列表与帮助首行均显示中文名
- **build 136 delta 审核整改**：① P1——中文模式目录/帮助计数错配（`CAT_COMPLETE_COUNT_FR`=758 对 355 条数组的越界读取）修复为 `ZH`；② 卫生四项——`System/KLib/mpy`（11MB 死头文件）清理、CDC 断开档位恢复加 PING 门槛（未 PING 不误降档）、`KEY_CTRL_HOME` 死宏删除、RPN39 手册版本号更新

---

## [build 135] - 2026-09-11 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-135))

### 充电系统完整修复（真机日志驱动）
- **三重根因**（`/charge_log.csv` 每秒诊断日志逐一定位）：
  1. 断充测量窗口只关 `ENABLE_DCDC` 未关 `PWD_BATTCHRG`——充电未停、静置读数虚高 150mV+（实测 1512 vs 真实 1352）导致提前停充
  2. "重插自动恢复"块把测量期 `PWD=1` 误判为待恢复态——2 秒打断测量并重置判据变量，导致整晚不停（24h 兜底除外）
  3. 充满停充会被上述恢复块自动重开
- **修复**：测量真断充（PWD=1/恢复 PWD=0）；恢复块加 `!g_measState && !g_chargeSessionDone`；5V 丢失只停硬件保留充电意愿（重插自动恢复）
- **验收**（11.4h 真机日志）：测量静置 61s、真实电压 1296→1440 稳步爬升、1.4V+2h 窗口精确触发（08:47 达 1.4V → 10:49 停）、停充后保持停充；充满终压 1.416V
- 诊断代码清理（UI 充电行/USB 电压/串口 CSV/设备日志文件恢复常规），README 硬件实测更新

### 规划
- FormCalc 实测反馈迭代
- KhiCAS：物理按键映射加强；示例功能补充；giac 2.0.0 有限移植
- D4 FTL_Sync 真机掉电测试

---

## [build 134] - 2026-09-06 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-134))

### 新增（FormCalc 完整实现 + 充电拔电修复 + Round 5 审核整改）

- **FormCalc 应用（Apps 页第三个程序，Home 键直达）**：
  - 首页功能列表：**单位换算 / 金融计算器 / 电子工程**（循环选择，F1-F3 直达）
  - **金融计算器 8 项**（HP-12C 全集）：TVM（N/i%/PV/PMT/FV 互解 + 期初/期末，FIX 2-9 位）、现金流（r% 常驻行 + CF0-12 滚动列表，NPV/IRR）、摊销（12C 语义区间 INT/PRIN/BAL）、债券（DD.MMYYYY 日期、30/360 价格、YLD 反解、付息频 1/2/4/12）、折旧（SL/DB200/SYD）、日期（儒略日、360/ACT 双日基）、利率换算（NOM↔EFF）、利润（12C 售价基）
  - **电子工程 9 项**：欧姆定律（V/I/R/P 四向）、分压器、串并联电阻、RC 时间常数、谐振频率、频率周期、正弦幅值、dBm、变压器（全量求解，View 公式视图）
  - **单位换算 10 类**：长度/面积/体积/质量/温度（℃℉K 公式对）/速度/压力/能量/功率/数据——输入值 + ←→ 选源单位，一页实时显示全部单位结果（超长 ↑↓ 滚动）
  - 交互：字段名中文 + 缩写（本金 PV 式）、输入 ENT 下框 / ↑↓ 切换 / 直接数字输入、行级局部刷新（输入只闪数值列——UICore flushRect 子宽支持）、循环菜单、Shift+BKSP 两遍确认清空全部、ON 返回层级（带位置记忆）
  - 持久化 /formcalc/formcalc.dat（FC03，17 表单）
- **session 文件目录化**：/rpn39/（sto/matx/cplx/stat）+ /formcalc/formcalc.dat；旧根目录文件自动读取迁移（降级兼容）
- **充电状态回归修复**：充电管理缺"外接电源消失"检测——拔 USB 后软件位保持导致误显"充电中:是"；新增 VDD5V<3500mV 立即停充 + 会话状态重置
- **FormCalc 图标**：表单纸意象（字段行 + F+ 断口），无衬底框线
- **Round 5 审核整改（A+B+C）**：
  - A：RPN39 持久化四件套空分支修复（P0——f_read 移入新路径分支，旧文件兼容迁移）
  - B：AMORT/DEPREC 期数上限 1e6 拒绝；IRR 不再要求 r%；BOND 整付息期校验；fcMsg snprintf 钳制超长转 e；solveI/IRR/BOND 二分 NaN 早退；UICore flushRect 参数防御
  - C：L0 返回高亮映射修正；ON 退出提交编辑值；死代码清理三处；**标题右侧 GBK 状态 16px 混排（12px ascii 渲染 GBK 雪花根治）**；单位页灰条/值列残影；电阻"并联"→"串并联"
- RPN39 手册更新至 build 134

---

## [build 133] - 2026-09-05 (已发布，[GitHub Release](https://github.com/awardat/ExistOS-For-HP39GII/releases/tag/build-133))

### 新增（2026-09-04 充电/功耗/显示修正 + 2026-09-05 KhiCAS 汉化/审核整改）
- **充电固定镍氢**（锂电直插实测失败——AAA 外围电路短路拉低电压；UI 类型选择隐藏、调用点固定镍氢、STOP_ILIMIT=0 + 12h 定时停充；ALKALINE_CHARGE 语义修正为 boost 电感效率位）
- **充电状态显示修正**：GET_CHARGE_STATUS 由软件开关改为硬件实际充电路径（电池满后 DCDC 关，不再误显"充电中"；三重停充实测——4 并镍氢充满自停）
- **UI 空闲 CPU 28%→1%**：页面信息刷新 900→3000ms、按键轮询 20→30ms（时钟/状态不常变）、控制台光标独立 1s 节奏；测试保持 28% 恒定 10 分钟
- **ON+{+,-} 对比度调节修复**：key_task_capt 状态 0 增加 ON 物理按住检测（矩阵扫描顺序导致组合键先入队时失效）
- **vTask1（Status Print）忙循环修复**：50ms delay + printTaskList 30s 降频（原空转占 CPU 且串口洪水）
- **CDC 会话任务恢复**：PING 挂起的监控任务（vBatteryMon 等）在 DTR 断开时自动恢复（免重启）
- **KhiCAS 帮助中文化**：completeCatzh 全量 355 条（260 条 desc 中文 GBK；XCAS_ONLY 保留原文）；lang=1 切换中文目录；**乱码根治**——textArea print() 宽度推进按字节×7（中文 14px）与 16px 渲染漂移 2px/字导致 ASCII 叠压中文（p 盖式/p= 盖键）→ 改 hp39_text_width（GBK=16px）推进；帮助/只读视图 ON=返回（View 不承担返回，编辑态语义保留）；charmap/脚本保存类型/trace 加 ON 等效退出；命令目录一级分类补译 5 项（算术加密/复数/选项/矩阵/三维）
- **Round 4 审核整改（A+B 组）**：SMP 样本标准差负值保护（均匀大数据集 NaN 根治）；统计计算提取 statCompute 共用（Welford 单遍在线方差——POP/SMP sd 数值稳定）；STAT 会话重置（RPN39 启动清空——主界面 F4 收集不再续上次数据；旧数据经 Shift+7 页加载查看）；STAT ENT 补 42S 压栈；矩阵光标钳制（matxClamp 覆盖 INV/CLR/R→A/B 全部维度变化路径）；复数除法/倒数溢出保护（±Inf → 0）；MATX F6 编辑槽按页直达（页 1→R / 页 2→A / 页 3→B——原循环 +1 与标签不符）；矩阵分类标签错字（地阵→矩阵）；UICore 档位标签按实际运行档（充电中强制标准档时不再误显 Boost，英文 Off→Standard）
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