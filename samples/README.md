# KhiCAS Python 示例脚本（samples）

适用于 HP39GII ExistOS 的 KhiCAS「Python 兼容」环境（giac 的 Python 语法层，
非 CPython/MicroPython——不支持任意第三方库）。脚本均为 GBK 编码中文注释，
可在设备上正常显示。

## 拷贝到设备

把 `*.py` 放到设备存储的**根目录**（或任意目录，文件浏览器可进入子目录），
用你惯用的方式（F2 复制 / edb / 文件管理器）。

## 运行方式

- KhiCAS 主界面：**APPS 键** → 打开脚本列表 → 选择文件 → ENTER → 编辑器打开
- 或 文件(Fich) 菜单 → 打开脚本
- 编辑器内 **OK（F6）**：语法检查/运行（"OK: test syntax"）

## 脚本列表

| 文件 | 内容 | 演示要点 |
|------|------|---------|
| 01_basics.py | 基础 | 变量/for/if/def 函数/print |
| 02_math.py | 数学 | math 模块、素数、CAS 调用 caseval（符号因式分解/解方程） |
| 03_lists.py | 列表 | 列表、append、索引、手写求和/最大值 |
| 04_matrix.py | 矩阵 | linalg：det/inv/add/mul/transpose |
| 05_plot.py | 绘图 | matplotl：曲线与散点（clf/plot/scatter/show） |
| 06_montecarlo.py | 随机 | random、蒙特卡洛估算 π |

## 注意

- KhiCAS 的 Python 是 **giac 的 Python 语法兼容层**：`from math import *`、
  `from linalg import *`、`from cas import *` 等模块均为 giac 提供
- `caseval("...")` 可调用 Xcas 的完整符号引擎（如 `caseval("factor(x^4-1)")`）
- 语法以设备实测为准；如某脚本报错，记录提示信息反馈
