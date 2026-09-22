# KhiCAS Python 示例脚本

本目录是 HP39GII（ExistOS/KhiCAS）的 Python 兼容模式示例脚本，使用 `from math/linalg/matplotl/cas import *` 等 giac Python 兼容层模块。

## 运行方式

1. 把 `.py` 文件复制到设备 **`/python/`** 目录（Python app 首页第 4 个图标）
2. **运行**：KhiCAS → 文件(Fich) 菜单 → **运行脚本** → 选择文件
3. **编辑**：文件菜单 → **打开脚本**（编辑器内 **F5 = 保存并运行**）
4. **查看输出**：运行结果输出到主界面 Console；需要导出时用 **文件(Fich) 菜单 → 保存日志**（Console 历史输出到串口，配串口工具捕获）

## 命名约束（重要）

KhiCAS 的 Python 兼容层由 giac 解析器实现，脚本里的标识符会与 **giac 内置函数/常量**发生冲突。以下为已知约束（写脚本时请规避）：

| 名称 | 冲突原因 | 规避方式 |
|------|----------|----------|
| `i` | giac 中 `i` 是**虚数单位**（内置符号常量） | 循环/临时变量改用 `k`、`j` 等 |
| `e` | giac 中 `e` 是自然常数 | 变量改用 `ex`、`val` 等 |
| `sign` | giac **内置函数**，不能重定义 | 自定义函数改名（如 `my_sign`） |
| `sum`/`max`/`min`/`abs`/`floor`/`round` 等 | 均为 giac 内置函数 | 不要用这些名字定义自己的函数 |
| `range`/`len`/`list` 等 | Python 兼容层提供的函数，可正常使用 | — |

## 其他已知限制

- **多行 `def` 函数体**：解析器对多行函数体支持有限，`def f(x):` + 多行缩进体可能报语法错误（建议用单表达式式函数，或把逻辑写在同一行；自定义函数名避开内置名）
- **`# -*- coding: gbk -*-` 等注释行**：可以保留（run_script 会跳过首部空行/注释行），但注释行会被忽略
- **中文文本**：脚本文件用 **GBK 编码**（设备字库为 HZK16S，UTF-8 中文无法渲染）
- **print 输出**：走 Console 日志通道，运行后直接显示在主界面；导出用 **文件菜单 → 保存日志**（串口输出；主界面 F5 落盘不可用）
- **括号跨行**：逐行执行按物理行切分，表达式括号跨行（如 `f(` 换行 `x)`）不支持，请保持单行完整

## 脚本列表

| 文件 | 内容 |
|------|------|
| `01_basics.py` | 变量、算术、for 循环、自定义函数 |
| `02_math.py` | math 模块、素数试除法、`caseval()` 调用符号引擎 |
| `03_lists.py` | 列表操作、索引、求和 |
| `04_matrix.py` | linalg 矩阵（行列式/求逆/乘法/转置） |
| `05_plot.py` | matplotl 函数曲线与散点 |
| `06_montecarlo.py` | random 蒙特卡洛估算 π |

### 脚本计时（time 扩展用法）

`time(1,2,3)` 返回**毫秒时间戳**（KhiCAS 移植扩展，内部 `rtc_get_tick_ms()`），可在任意脚本中自行计时：

```python
caseval("bench_t0:=time(1,2,3)")
# ... 被测代码 ...
caseval("bench_t1:=time(1,2,3)")
caseval("bench_ms:=bench_t1-bench_t0")
print(caseval("bench_ms"))
```

- 必须用 **3 个参数**调用（`time(1,2,3)`）；`time(a,b)` 两参数用于设置 RTC 时间，不要使用
- 另可用 `time(表达式)` 对单个表达式计时（返回秒，如 `time(integrate(sin(x)/x,x))`）

## Python app 样本（独立 MicroPython，2026-09-20）

**Python app**（首页第 4 个图标）是真正的 MicroPython 解释器，样本以 **`pyNN_` 前缀**与 KhiCAS 样本区分：

| 文件 | 内容 |
|------|------|
| `py01_basics.py` | 语言基础：算术/字符串/列表/循环/条件/函数/字典 + 计时 + 本地导入 |
| `py02_module.py` | 供 py01 测试 `import` 的小模块（放同目录） |
| `py07_nqueens.py` | N-Queens 8x8 基准（92 解 / 2057 节点 + 计时） |

**运行方式**：把文件复制到设备 `/python/` → Python app 内按 **F4（运行）** 选择文件，或 **F6 文件 → 打开并运行**。

**完整说明**：[docs/Python-app-manual.md](../docs/Python-app-manual.md)（按键表/菜单/语法差异/性能基准/FAQ）。

与 KhiCAS 样本的差异：MicroPython 无 giac 兼容层限制（`i`/`sign` 等可自由使用）；支持 `open()` 文件读写与 `import` 本地模块（均按 `/python/` 解析）。

## 备注

以上命名约束已收录进《KhiCAS 用户手册》：[docs/KhiCAS-manual.md](../docs/KhiCAS-manual.md)。
