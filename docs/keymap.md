# ExistOS-For-HP39GII 当前键位定义（代码实际）

> **说明**：本文件记录**当前代码实现**的键位（编码 + KhiCAS 映射 + ExistOS UI 行为）。
> 原厂功能定义见 `keymap_org.md`（参考文件，不动）。编码与 org 表一致（51 键）。

## 一、HP39GII 物理键盘矩阵

编码格式：`KEY_xxx = (row << 3) + col`（`System/drivers/keyboard_gii39.h`）。

```
        col0    col1    col2    col3    col4
       ┌───────┬───────┬───────┬───────┬───────┐
row 0  │  F1   │  F3   │  F4   │  F5   │  UP   │
       ├───────┼───────┼───────┼───────┼───────┤
row 1  │ SYMB  │  F2   │  NUM  │  F6   │ RIGHT │
       ├───────┼───────┼───────┼───────┼───────┤
row 2  │ HOME  │ PLOT  │ VIEWS │X,T,θ,N│ LEFT  │
       ├───────┼───────┼───────┼───────┼───────┤
row 3  │ VARS  │ APPS  │ A B/C │  ←BS  │ DOWN  │
       ├───────┼───────┼───────┼───────┼───────┤
row 4  │  SIN  │ MATH  │  TAN  │  LN   │  LOG  │
       ├───────┼───────┼───────┼───────┼───────┤
row 5  │  X²   │  COS  │   (   │   )   │   ÷   │
       ├───────┼───────┼───────┼───────┼───────┤
row 6  │   ,   │  X^Y  │   8   │   9   │   ×   │
       ├───────┼───────┼───────┼───────┼───────┤
row 7  │ ALPHA │   7   │   5   │   6   │   -   │
       ├───────┼───────┼───────┼───────┼───────┤
row 8  │ SHIFT │   4   │   2   │   3   │   +   │
       ├───────┼───────┼───────┼───────┼───────┤
row 9  │   0   │   1   │   .   │  (-)  │ ENTER │
       ├───────┼───────┼───────┼───────┼───────┤
row 10 │ ON/C  │       │       │       │       │
       └───────┴───────┴───────┴───────┴───────┘
```

## 二、按键编码表（keyboard_gii39.h）

| 编码 | 键 | 编码 | 键 | 编码 | 键 |
|------|----|------|----|------|----|
| 0 | F1 | 25 | APPS | 52 | × |
| 1 | F3 | 26 | A B/C | 56 | ALPHA |
| 2 | F4 | 27 | ←BS | 57 | 7 |
| 3 | F5 | 28 | DOWN | 58 | 5 |
| 4 | UP | 32 | SIN | 59 | 6 |
| 8 | SYMB | 33 | MATH | 60 | - |
| 9 | F2 | 34 | TAN | 64 | SHIFT |
| 10 | NUM | 35 | LN | 65 | 4 |
| 11 | F6 | 36 | LOG | 66 | 2 |
| 12 | RIGHT | 40 | X² | 67 | 3 |
| 16 | HOME | 41 | COS | 68 | + |
| 17 | PLOT | 42 | ( | 72 | 0 |
| 18 | VIEWS | 43 | ) | 73 | 1 |
| 19 | X,T,θ,N | 44 | ÷ | 74 | . |
| 20 | LEFT | 48 | , | 75 | (-) |
| 24 | VARS | 49 | X^Y | 76 | ENTER |
| | | 50 | 8 | 80 | ON/C |
| | | 51 | 9 | | |

## 三、KhiCAS 键位映射（khicas_stub.cpp INPUT_TRANSLATE）

格式：物理键 → `普通 / SHIFT / ALPHA-小 / ALPHA-大`。未列出修饰键的表示各档相同。

### 功能键

| 物理键 | 普通 | SHIFT |
|--------|------|-------|
| F1-F6 | KEY_CTRL_F1-F6 | 同普通（KhiCAS 软件菜单键） |

### 导航/模式键

| 物理键 | 普通 | SHIFT |
|--------|------|-------|
| UP / DOWN | 光标上/下 | 上/下一页（PAGEUP/PAGEDOWN） |
| LEFT / RIGHT | 光标左/右 | 选择左/右（SHIFT_LEFT/RIGHT） |
| VIEWS | **KEY_CTRL_EXIT**（输入行→打开脚本编辑器；历史区→跳回输入行） | **KEY_CTRL_QUIT**（命令目录/帮助） |
| HOME | KEY_CTRL_INS | 同普通 |
| NUM | KEY_CTRL_OPTN | KEY_SHIFT_OPTN |
| SYMB | KEY_CTRL_SETUP | 同普通 |
| VARS | KEY_CTRL_VARS | KEY_CTRL_INS |
| MATH | KEY_CTRL_MENU（会话菜单） | 命令目录（CATALOG，org:Cmds） |
| A B/C | KEY_CTRL_FRACCNVRT | " 引号（DQUATE，org:'"） |
| X,T,θ,N | KEY_CTRL_XTT | KEY_CHAR_EXPN10 |
| ←BS | KEY_CTRL_DEL | 清空输入（AC，org:Clear） |
| **PLOT** | **未映射**（KhiCAS 中无反应） | — |
| **APPS** | **未映射**（KhiCAS 中无反应） | — |
| ALPHA / SHIFT | 修饰键状态切换（case 245/263） | — |
| ON/C | 特殊处理（shift+ON/C 退出 KhiCAS） | — |

### 数学键

| 物理键 | 普通 | SHIFT |
|--------|------|-------|
| SIN | sin( | asin( |
| COS | cos( | acos( |
| TAN | tan( | atan( |
| LN | ln( | e^( |
| LOG | log( | 10^( |
| X² | ^2 | √( |
| X^Y | ^ | 开方根( ^n√ ) |

### 字符/括号键

| 物理键 | 普通 | SHIFT |
|--------|------|-------|
| ( | ( | 复制（CLIP） |
| ) | ) | 粘贴（PASTE） |
| , | , | , |
| . | . | = |

### 数字键

| 物理键 | 普通 | SHIFT |
|--------|------|-------|
| 0 | 0 | **命令目录**（CATALOG） |
| 1 | 1 | 程序菜单（PRGM） |
| 2 | 2 | 虚数 i |
| 3 | 3 | π |
| 4 | 4 | 矩阵（MAT） |
| 5 | 5 | [ |
| 6 | 6 | ] |
| 7 | 7 | 列表（LIST） |
| 8 | 8 | { |
| 9 | 9 | } |

### 运算符键

| 物理键 | 普通 | SHIFT |
|--------|------|-------|
| + | + | + |
| - | - | ∠ 角度（ANGLE，org:∠） |
| × | × | ! |
| ÷ | ÷ | 倒数（RECIP） |
| (-) | 负号 | \| \|（绝对值） |

### 执行键

| 物理键 | 普通 | SHIFT | ALPHA |
|--------|------|-------|-------|
| ENTER | 执行（EXE） | ANS | 回车符 |

### ALPHA 字符分配

`VARS→a`、`MATH→b`、`A B/C→c`、`X,T,θ,N→d`、`SIN→e`、`COS→f`、`TAN→g`、`LN→h`、`LOG→i`、`X²→j`、`X^Y→k`、`(→l`、`)→m`、`÷→n`、`,→o`、`7→p`、`8→q`、`9→r`、`×→s`、`4→t`、`5→u`、`6→v`、`-→w`、`1→x`、`2→y`、`3→z`、`+→空格`、`0→"`、`.→:`、`(-)→;`

## 四、ExistOS 各页面按键功能（UICore.cpp）

### Apps 页（page 0）

| 按键 | 功能 |
|------|------|
| LEFT/RIGHT | 切换应用页 |
| ENTER | 启动选中的应用 |
| F6 | 切换到 Status 页 |

### Console 页（page 1）

| 按键 | 功能 |
|------|------|
| 任意键 | 输入到控制台 |
| F3 | 切换到 Apps 页 |

### Files 页（page 2）

| 按键 | 功能 |
|------|------|
| UP/DOWN | 选择文件/文件夹 |
| ENTER | 打开文件夹 |
| LEFT | 返回上级目录 |
| RIGHT | 进入选中的文件夹 |
| F3 | 切换到 Apps 页 |
| F5 | 删除文件/文件夹 |
| F6 | 切换到 Status 页 |
| +/- | 翻页 |

### Status 页（page 3）

| 按键 | 功能 |
|------|------|
| UP/DOWN | 切换子页面 |
| 1 | 省电模式循环：标准(120MHz) → 省电(80MHz) → 加速(240MHz) → 标准 |
| 2 | 充电开关 |
| F3 | 减少子页面 |
| F4 | 增加子页面 |
| F6 | 切换到 Apps 页 |

## 五、系统全局快捷键

| 组合键 | 功能 |
|--------|------|
| ON/C + F5 | 维护菜单（清除数据/格式化/重启） |
| ON/C + [-] | 减小对比度 |
| ON/C + [+] | 增大对比度 |
| Shift + ON/C | 关机 |

## 六、按键状态 API

```c
typedef enum Key_state_t {
    KEY_PRESS = 0,    // 按下
    KEY_RELEASE = 1   // 释放
} Key_state_t;

int key = api_get_key(-1);          // 阻塞等待
int pressed = api_get_key(KEY_ENTER); // 非阻塞查询

// OSLoader 层：raw = ll_vm_check_key(); key = raw & 0xFFFF; press = raw >> 16;
```

## 七、源代码参考

| 文件 | 说明 |
|------|------|
| `System/drivers/keyboard_gii39.h` | 按键枚举（编码权威） |
| `System/applications/user/khicas/khicas_stub.cpp` | KhiCAS 按键翻译（INPUT_TRANSLATE） |
| `System/graphics/UICore.cpp` | ExistOS UI 按键处理 |
| `System/utils/basic_api.c` | App SDK 按键 API |
| `keymap_org.md` | **原厂键位表**（功能定义参考，不改动） |
## 五、RPN39 键位设计（当前实现，build 131）

| 键 | 功能 | 说明 |
|------|------|------|
| ON/C | **取消/后退**（短按）；Shift+ON/C 关机 | HP 原版 ON/C 语义（开机后为取消键） |
| VIEWS | 系统/新计算器**无定义**（释放）；KhiCAS 保留（切换公式输入） | 原 View 的后退已转移至 ON/C |
| MATH | **数学功能菜单**（原厂语义保留） | 5 页：角度/常量基础/双曲/概率/幂；循环翻页（左右/上下）+ 数字键 1-5 跳页；标题中英文对照；ON 退出 |
| VARS | **寄存器列表**（26 个 A-Z 寄存器：名称+数值） | 选择 + ENTER = RCL（值压入 X，栈上移）；Shift+backspace = 清空全部（带确认） |
| F1-F6 | x<>y / R↓ / DROP / __ / __ / __ | 无物理键的 RPN 操作（F4-F6 空） |
| X,T,θ,N | EEX 科学计数输入（追加 e+指数位） | STO/RCL 上下文 = d 寄存器；指数区 CHS 翻转符号 |
| ( / ) | STO / RCL（存数/取数） | STO 后直接按字母（自动 ALPHA） |
| ENTER | 输入中=压栈；非输入中=复制 X 压栈 | HP RPN 惯例；计算后输入数字自动压栈（auto-lift） |
| 数字区 | 0-9 . (-)CHS +−×÷ backspace | 原厂键位直接对齐；backspace 非输入态从当前 X 开始编辑 |
| 函数键（正常层） | x² x^y（Y^X） SIN/COS/TAN LN/LOG a b/c（小数↔分数） | 原厂键位直接对齐；a b/c 仅切换 X 行显示（连分数，转不出保持小数） |
| 函数键（Shift 层） | 1/x（÷） 阶乘（×） √（x²） Y次根（x^y） ASIN/ACOS/ATAN（SIN/COS/TAN） e^x/10^x（LN/LOG） π（3） ABS（(-)） CLx（backspace） 退出（ON） | 动作完成自动退出 Shift |
| 角度模式 | DEG/RAD/GRAD（MATH 页 1 或标题右上状态） | 三角/反三角按当前模式；持久化 |
