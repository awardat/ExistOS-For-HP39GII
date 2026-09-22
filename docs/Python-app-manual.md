# Python 应用用户手册（HP39GII / ExistOS）

> 独立 MicroPython 应用（v1.29.0）。适用于 build 140 及以后版本。
> 多行输入后按 **F5（run）** 执行；应用内帮助在 **F6 文件 → 关于**（本手册为完整版）。

---

## 1. 简介

HP39GII 首页应用页第 4 个图标 **Python** 是一个**真正的 MicroPython 解释器**（非 KhiCAS 的语法兼容层）：

- 完整 Python 语言：类、生成器、异常、推导式、闭包、模块导入（内置模块）
- 适合：Python 学习、脚本、算法练习（如 N-Queens）
- 无网络/蓝牙/线程等无硬件支撑的功能（以硬件为准）
- 会话变量在退出后**保留**（再次进入继续用；要清空用 文件菜单 → 复位解释器）

## 2. 启动与退出

| 操作 | 说明 |
|------|------|
| 首页 → 应用页 → 第 4 格 **Python** → ENT | 进入（显示 `MicroPython v1.29.0` 横幅与 `>>>` 提示符） |
| **Shift+ON** | 退出（单独 ON 是清屏） |
| 再次进入 | 之前的变量仍在（会话保留） |

## 3. 界面

- **标题行**：`Python 3.4 / MicroPython 1.29 [96K]`（96K = 解释器堆大小）
- **输出区**：6 行 × 31 字符（16px 字号，中英文混排），保留最近 **96 行**回看
- **报错信息已汉化**：`SyntaxError/NameError/TypeError` 等常见错误显示为中文
- **底栏**：F1 符号 ｜ F2 清屏 ｜ F3 取消 ｜ F4（空） ｜ **F5 run（执行输入）** ｜ F6 文件
- **运行脚本**：F6 文件 → **运行脚本**（列出 `/python/*.py`）

## 4. 按键表

### 4.1 直接输入

| 按键 | 输入 | 按键 | 输入 |
|------|------|------|------|
| 0–9 | `0`–`9` | `(` `)` | `(` `)` |
| `.` | `.` | `,` | `,` |
| `+` `-` `×` `÷` | `+` `-` `*` `/` | `(-)` | `-` |
| ENT | **换行**（只输入，不执行） | 退格 | 删除（行内任意位置） |

### 4.2 Shift 层（Shift + 键）

| 按键 | 输入 | 按键 | 输入 |
|------|------|------|------|
| 5 / 6 | `[` `]` | 8 / 9 | `{` `}` |
| 3 | `#`（注释） | 0 | `"` |
| `.` | `=`（赋值/比较） | `×` | `!` |
| `(-)` | `_`（下划线） | | |

### 4.3 ALPHA 字母层

按 **ALPHA**：第 1 次 = 大写（一次有效），第 2 次 = 小写（一次有效），第 3 次 = 关闭。
**Shift+ALPHA = 锁定小写**（指示灯亮，再按 ALPHA 或 Shift+ALPHA 解除）。

| 键 | 字母 | 键 | 字母 | 键 | 字母 |
|----|------|----|------|----|------|
| VARS | a | MATH | b | a b/c | c |
| ×10ⁿ | d | SIN | e | COS | f |
| TAN | g | LN | h | LOG | i |
| x² | j | xʸ | k | `(` | l |
| `)` | m | ÷ | n | `,` | o |
| 7 | p | 8 | q | 9 | r |
| × | s | 4 | t | 5 | u |
| 6 | v | − | w | 1 | x |
| 2 | y | 3 | z | | |

字母层的符号：ALPHA + `+` = 空格，ALPHA + `.` = `:`，ALPHA + 0 = `"`，ALPHA + `(-)` = `;`

### 4.4 导航与编辑

| 按键 | 功能 |
|------|------|
| ← / → | 行内光标左右移动（回退改错） |
| UP / DOWN | 输出回看逐行滚动 |
| Shift+UP / Shift+DOWN | 输出回看整页翻页 |
| 退格 | 删除光标前字符（行首忽略） |
| **Shift+退格** | **清空当前输入**（并清屏） |
| ON | 清屏 |
| Shift+ON | 退出 |

### 4.5 功能键

| 键 | 功能 |
|----|------|
| F1 | **符号面板**（4 页，见下） |
| F2 | 清屏 |
| F3 | 取消当前输入（同 Shift+退格） |
| F4 | （空） |
| F5 | **run：执行本次输入**（单行表达式会显示结果值） |
| F6 | **文件菜单**（见下） |

### 4.1 数学键直通（参照 KhiCAS）

| 键 | 普通层 | Shift 层 |
|----|--------|----------|
| `SIN` | `sin(` | `asin(` |
| `COS` | `cos(` | `acos(` |
| `TAN` | `tan(` | `atan(` |
| `LN` | `log(`（自然对数） | `exp(` |
| `LOG` | `log10(` | `log2(` |
| `x²` | `**2` | `sqrt(` |
| `xʸ` | `**` | `pow(` |
| `(` | `(` | `abs(` |

（ALPHA 字母输入优先；以上键在 ALPHA 模式下仍输入字母）

## 5. 菜单

### 5.1 符号面板（F1）

| 页 | 内容 |
|----|------|
| 符号 | `: , . = < > _ # " '` |
| 运算符 | `== != <= >= + - * / % **` |
| 括号与赋值 | `( ) [ ] { } // += -= =` |
| 结构 | `if  elif  else:  for  while  def  return  print(  import  in` |

操作：**↑↓←→ 选择**、**ENT 或 F1 插入**、**F2 或 ON 取消**、**F3 上翻页 / F4 下翻页**（共 6 页：符号 / 运算符 / 括号与赋值 / 结构 / **函数** / **常量与模块**）。

### 5.2 文件菜单（F6）

| 项 | 功能 |
|----|------|
| 运行脚本 | 列出 `/python/*.py`，ENT 运行 |
| 保存会话 | 把终端最近 96 行写入 `/python/session.txt` |
| 清屏 | 同 F2 |
| 复位解释器 | 清空所有变量、重开会话（相当于重启解释器） |
| 关于 | 打开帮助的"关于"页 |
| 退出 | 同 Shift+ON |

## 6. Python 支持的功能

> 基于 **MicroPython v1.29.0**，编译档位为 EXTRA FEATURES（详见下文"不支持"清单）。

### 6.1 语言特性

| 支持 | 说明 |
|------|------|
| 变量 / 运算 / 字符串 / f-string | 含 `%` 与 `str.format`；整数为**任意精度** |
| 列表 / 元组 / 字典 / 集合 / frozenset | 推导式（list/dict/set）、切片（**步长必须为 1**） |
| 条件 / 循环 | `if/elif/else`、`for`（含 `enumerate/zip/range`）、`while`、`break/continue` |
| 函数 | 默认参数、关键字参数、`*args`、`**kwargs`、lambda、闭包、装饰器 |
| 类 | 单继承、`super()`、属性（property）、`staticmethod`/`classmethod`、特殊方法（`__init__/__str__/__len__/__getitem__` 等） |
| 生成器 | `yield`、生成器表达式 |
| 异常 | `try/except/else/finally`、`raise`、自定义异常类 |
| 上下文管理 | `with`（`__enter__`/`__exit__`） |
| 模块 | `import`（内置模块 + `/python/` 下的本地 `.py`） |

### 6.2 内置函数与类型（常用）

`print` `len` `range` `enumerate` `zip` `map` `filter` `sorted` `reversed` `sum` `min` `max` `abs` `round` `pow` `divmod` `int` `float` `complex` `bool` `str` `bytes` `bytearray` `list` `tuple` `dict` `set` `frozenset` `type` `isinstance` `id` `hash` `repr` `dir` `getattr` `setattr` `hasattr` `delattr` `callable` `iter` `next` `chr` `ord` `hex` `bin` `oct` `any` `all` `format` `open` `exec` `eval` `compile` `help` `memoryview` `super` `property` `staticmethod` `classmethod` `NotImplemented`

### 6.3 内置模块

> **使用前必须先 `import`**（如 `import math` 后 `math.factorial(30)`）；这与标准 Python 一致。`2**100`、字符串、列表等内建功能不需要 import。

| 模块 | 可用内容 |
|------|----------|
| `math` | 常数 `pi/e/tau/inf/nan`；`sqrt/exp/log/log2/log10/pow`；三角与反三角、双曲；`floor/ceil/trunc/fabs/fmod/frexp/ldexp/modf/copysign`；`isnan/isinf/isclose`；`factorial/gamma/lgamma/erf/erfc` |
| `cmath` | 复数版数学函数（配合 `complex`） |
| `random` | `random/randint/randrange/choice/shuffle/uniform/gauss/seed/getrandbits` |
| `struct` | `pack/unpack/pack_into/unpack_from/calcsize` |
| `array` | 类型化数组（`'b','B','h','H','i','I','l','L','f','d'` 等） |
| `collections` | `namedtuple/deque/OrderedDict/defaultdict` |
| `heapq` | 堆操作（`heappush/heappop/heapify/nlargest/nsmallest`） |
| `io` | `BytesIO/StringIO`（配合 `open()`） |
| `time` | `ticks_ms/ticks_us/ticks_cpu/ticks_add/ticks_diff`、`sleep/sleep_ms/sleep_us`（**日历函数未实现**） |
| `sys` | `print_exception`、`exit`、`maxsize`、`implementation`、`modules`、`path` |
| `gc` | `collect/mem_free/mem_alloc/enable/disable` |
| `micropython` | `mem_info/qstr_info/opt_level`、`ringio` |
| `errno` | 常用 errno 常量 |

### 6.4 文件与导入

- `open(path, mode)`：读写 `/python/` 下文件（相对路径按 `/python/` 解析）；支持 `read/readline/readlines/write/seek/tell/flush/close` 与 `with`
- `import 模块名`：查找 `/python/模块名.py`（子目录按 `/python/子目录/模块名.py` 解析）
- 示例见 §7

### 6.5 不支持 / 有限制

| 项目 | 说明 |
|------|------|
| `async/await`、`_thread`、`asyncio` | 未编译（无多线程硬件支撑） |
| `socket` / `network` / `ssl` / `bluetooth` / `machine`(GPIO/I2C/SPI) | 未编译（无硬件） |
| `json` / `re` / `hashlib` / `binascii` / `deflate` / `uctypes` / `framebuf` | 未编译（可后续按需开启，见下） |
| `input()` | 未接入 stdin 通道（读取会报错）；`select` 虽已编译但无可用对象 |
| 负步长切片 | `s[::-1]` 报 `NotImplementedError`（用循环或 `reversed`） |
| 多继承 | 不支持（单继承） |
| `match` 语句 | 不支持 |
| 大整数 | **支持任意精度整数**（MPZ）：`2**100`、`math.factorial(50)` 等均可；大数运算比机器整数慢 |
| 浮点 | 双精度但为**软件实现**（无 FPU），复杂浮点计算较慢 |
| 递归深度 | 受任务栈限制（建议 < 100 层） |
| `os` 模块 | 已编译但文件相关函数依赖 VFS（未启用），请用 `open()` |
| 中文输出 | 终端为 ASCII 字体（界面菜单已汉化）；中文以占位符显示 |

> 需要 `json`/`re`/`hashlib` 等功能时可以开启（重新编译固件）：在 `Libs/src/micropython/ports/eoslib/mpconfigport.h` 把对应 `MICROPY_PY_*` 置 1 即可。

## 7. 文件与脚本

### 7.0 输入与运行（单元格模型）

- **ENT 只换行**，不执行——可以连续输入多行代码
- **自动缩进**：以 `:` 结尾的行按 ENT 后下一行自动缩进 4 空格（沿用本行缩进）；在只有缩进的空行上按 ENT 回到行首（结束块）
- 输入完成后按 **F5（run）** 执行**自上次运行以来输入的全部内容**（结果输出显示在其下方，并出现新的 `>>>` 提示符）
- **逐条语句执行**：单行语句用 `single` 模式——表达式会**显示结果值**（如 `2+3` → `5`）；带缩进的块（`for`/`def`/`if` 等）整体用 `exec` 模式执行（如需输出请用 `print()`）
- `F1` 符号面板插入的符号/结构直接进入输入单元格
- **Shift+退格**清空当前输入；**ON** 清屏


- 脚本目录：**`/python/`**（独立于 KhiCAS 的 `/xcas/`；首次启动自动创建并迁移旧的 `/xcas/py*.py`、`session.txt`、`pyheap.cfg`）
- 读写示例：
  ```python
  f = open("/python/t.txt", "w")
  f.write("hello")
  f.close()
  print(open("/python/t.txt").read())
  ```
- 运行脚本：把 `.py` 复制到 `/python/` → **F6 文件 → 运行脚本** 选择文件运行
- 随仓库样本（`samples/`）：

| 文件 | 内容 |
|------|------|
| `py01_basics.py` | 语言基础：算术/字符串/列表/循环/条件/函数/字典 + 计时 + 本地导入 |
| `py02_module.py` | 供 py01 测试 `import` 的小模块 |
| `py07_nqueens.py` | N-Queens 8×8 基准（92 解 / 2057 节点 + 计时） |

## 8. 性能基准（N-Queens 8×8）

| 平台 | 耗时 |
|------|------|
| **HP39GII Python app（加速 480MHz）** | **703 ms** |
| **HP39GII Python app（标准 240MHz）** | **1221 ms** |
| HP-42S（参考基准） | ≈ 12 分钟 |
| HP-41CX（参考基准） | ≈ 18 分钟 |
| PB-2000C（参考基准） | 8:52 |

> 参考基准来自 "Calculator Benchmark"（ref/nqueens/）。本应用速度显著高于原厂系统同类任务。

## 9. 限制与已知问题

- 语言/模块限制见 **§6.5**（无网络/线程/GPIO、无大整数、无负步长切片等）
- 无中文输出（解释器输出为 ASCII；界面菜单已汉化）
- 浮点运算为软件实现，复杂浮点计算较慢
- 内存：GC 堆 96KB（不足时自动回退 64/32KB）；大脚本/大数据请开启 **MEM SWAP**（见 README）

## 10. FAQ

| 问题 | 解决 |
|------|------|
| 输入后没有反应 | **ENT 只换行**：多行输入完成后按 **F5（run）** 执行 |
| 缩进怎么输入 | 行尾 `:` 后 ENT **自动缩进 4 空格**；空行 ENT 回到行首；也可用 `+` 键（普通层）输入空格 |
| 表达式没显示结果 | 多行输入按**逐条语句**执行：单行表达式会回显值；块语句请用 `print()` |
| `名称'xxx' isn't defined` | 漏了 `import`（如 `import math`）或拼写错误；也可能是变量未赋值 |
| 提示 `NotImplementedError`（切片） | MicroPython 不支持负步长切片，改写法 |
| 内存不足（not enough memory） | 设置里开启 **MEM SWAP** 后重启 |
| 想清空所有变量 | F6 → 复位解释器 |
| 输入的中文/特殊字符 | 键盘无中文输入；用符号面板插入常用符号 |
