// RPN39 - RPN 计算器（阶段 2：科学计算）
// 设计见 docs/RPN39-design.md（42S 基准，实现原创）
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <stdarg.h>
#define LCD_PIX_W 256
#define LCD_PIX_H 127
#define EXCLUDE_UI_LANGUAGE
#include "FreeRTOS.h"
#include "task.h"
#if FS_TYPE == FS_FATFS
    #include "filesystem/fatfs/ff.h"
#else
    #include "filesystem/littlefs/lfs.h"
#endif
#include "../../../core/SystemConfig.h"
#include "../../../third_party/freertos/include/SysConf.h"
#include "../../graphics/UICore.h"
#include "../../drivers/keyboard_gii39.h"
#include "rpn39_int.h"

extern UI_Display *uidisp;
extern "C" {
uint32_t ll_vm_check_key();
void SystemUISuspend();
void SystemUIResume();
}
int shiftHeld = 0; // Shift 状态（全局，供绘制显示）

double stX = 0, stY = 0, stZ = 0, stT = 0; // 4 层栈
double lastX = 0;                          // LAST X
int entering = 0;                          // 数字输入中
int autoLift = 0;                           // 栈提升标志（42S：计算后输入数字自动压栈）
char inbuf[40];
int inlen = 0;
int rpn39Running = 0;

// ---- 寄存器（A-Z，42S STO/RCL）----
double regs[26] = {0};
int rpnMode = 0;       // 0=正常 1=STO 等字母 2=RCL 等字母 3=寄存器列表 4=MATH 菜单
int stoOp = 0;         // STO 运算（42S：0=普通 STO，1/2/3/4 = STO+ − × ÷；Shift+四则在 STO 态选择）
static int clearConfirm = 0;  // CLEAR ALL 确认态
static int regSel = 0;        // 列表高亮（0-25）
static int regTop = 0;        // 列表滚动顶

// ---- 阶段 2：角度模式 / 分数显示 / MATH 菜单 ----
int angMode = 0;       // 0=DEG 1=RAD 2=GRAD（持久化）
int fracMode = 0;      // X 行分数显示（0=小数）
int mathPage = 0;      // MATH 菜单当前页（0-4）

// 字母键映射（key 码 -> 寄存器索引）：VARS=a MATH=b ABC=c XTPHIN=d SIN=e COS=f TAN=g LN=h LOG=i
// X2=j XY=k ( =l )=m / =n , =o 7=p 8=q 9=r x=s 4=t 5=u 6=v -=w 1=x 2=y 3=z
static int regKeyToIdx(int key) {
    switch (key) {
        case KEY_VARS: return 0;            // a
        case KEY_MATH: return 1;            // b
        case KEY_ABC: return 2;             // c
        case KEY_XTPHIN: return 3;          // d
        case KEY_SIN: return 4;             // e
        case KEY_COS: return 5;             // f
        case KEY_TAN: return 6;             // g
        case KEY_LN: return 7;              // h
        case KEY_LOG: return 8;             // i
        case KEY_X2: return 9;              // j
        case KEY_XY: return 10;             // k
        case KEY_LEFTBRACKET: return 11;    // l
        case KEY_RIGHTBRACKET: return 12;   // m
        case KEY_DIVISION: return 13;       // n
        case KEY_COMMA: return 14;          // o
        case KEY_7: return 15;              // p
        case KEY_8: return 16;              // q
        case KEY_9: return 17;              // r
        case KEY_MULTIPLICATION: return 18; // s
        case KEY_4: return 19;              // t
        case KEY_5: return 20;              // u
        case KEY_6: return 21;              // v
        case KEY_SUBTRACTION: return 22;    // w
        case KEY_1: return 23;              // x
        case KEY_2: return 24;              // y
        case KEY_3: return 25;              // z
    }
    return -1;
}

// 掉电持久化（/rpn39_sto.dat：26 寄存器 + 4 栈 + 角度模式；旧版 208B/240B 兼容）
void saveRegs(void) {
    FIL f;
    if (f_open(&f, "/rpn39_sto.dat", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        UINT bw = 0;
        f_write(&f, regs, sizeof(regs), &bw);          // 26 regs
        double st[5] = {stX, stY, stZ, stT, (double)angMode};
        f_write(&f, st, sizeof(st), &bw);              // 栈 + 角度模式
        f_close(&f);
    }
}
void loadRegs(void) {
    FIL f;
    UINT br = 0;
    memset(regs, 0, sizeof(regs));
    angMode = 0;
    if (f_open(&f, "/rpn39_sto.dat", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        f_read(&f, regs, sizeof(regs), &br); // regs 区（br 出参）
        if (br >= sizeof(regs)) {            // 有栈区（新版文件）
            double st[5] = {0};
            UINT br2 = 0;
            f_read(&f, st, sizeof(st), &br2);
            if (br2 >= 4 * sizeof(double)) { stX = st[0]; stY = st[1]; stZ = st[2]; stT = st[3]; }
            if (br2 >= 5 * sizeof(double)) { angMode = (int)st[4]; if (angMode < 0 || angMode > 2) angMode = 0; } // 角度模式（含 31 值新版；坏文件钳制）
        }
        f_close(&f);
    }
}

// ---- 栈操作（HP RPN 语义）----
void stackLift() { stT = stZ; stZ = stY; stY = stX; }
void stackDrop() { stX = stY; stY = stZ; stZ = stT; stT = 0; } // DROP：X 丢弃，栈上移，T 清空

// ---- 数字格式化（整数直显，否则 12 位有效数字）----
const char *fmtNum(double v, char *buf) {
    if (v > -1e9 && v < 1e9 && v == (double)(long long)v) // 范围先判（防 2^63 UB），整数 <=9 位直显
        sprintf(buf, "%lld", (long long)v);
    else
        sprintf(buf, "%.12g", v);
    if ((int)strlen(buf) > 13) sprintf(buf, "%.6e", v); // 显示预算统一：超 13 字符压缩（T/Z/Y 与 X 同阈值）
    return buf;
}

// ---- 阶段 2：角度转换（DEG/RAD/GRAD）----
double toRad(double v) {
    if (angMode == 1) return v;                 // RAD
    if (angMode == 2) return v * M_PI / 200.0;  // GRAD
    return v * M_PI / 180.0;                    // DEG
}
double fromRad(double v) {
    if (angMode == 1) return v;
    if (angMode == 2) return v * 200.0 / M_PI;
    return v * 180.0 / M_PI;
}

int valid(double r) { return !(r != r || r == HUGE_VAL || r == -HUGE_VAL); }

// 单操作数函数：X -> f(X)（LAST X=旧 X，autoLift=1）
// angIn：输入先转弧度（三角）；angOut：结果转当前角度（反三角）
void unaryOp(double (*f)(double), int angIn, int angOut) {
    double x;
    if (entering) { x = atof(inbuf); entering = 0; inlen = 0; }
    else x = stX;
    if (angIn) x = toRad(x);
    double r = f(x);
    if (!valid(r)) r = 0;
    if (angIn && fabs(r) < 1e-13) r = 0; // 二转十噪声舍去（90° cos → 0）
    if (angOut) r = fromRad(r);
    lastX = stX;
    stX = r;
    autoLift = 1;
}

// 双操作数：Y g X -> X（栈下移，同四则）
void binaryOp(double (*g)(double, double)) {
    double x, y;
    if (entering) { x = atof(inbuf); entering = 0; inlen = 0; }
    else x = stX;
    y = stY;
    lastX = x;
    double r = g(y, x);
    if (!valid(r)) r = 0;
    stX = r;
    stY = stZ; stZ = stT;
    autoLift = 1;
}

// 常量压栈（42S：常数输入，旧 X 上移，可连续输入参与运算）
void pushConst(double v) {
    if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
    lastX = stX;
    stackLift();
    stX = v;
    autoLift = 1;
}

// 辅助函数（math 风格，C++11 lambda 不可用场景用静态函数）
static double f_abs (double x) { return fabs(x); }
static double f_sq  (double x) { return x * x; }
static double f_cube(double x) { return x * x * x; }
static double f_rec  (double x) { return x != 0 ? 1.0 / x : 0; }
static double f_pow10(double x) { return pow(10.0, x); }
static double f_exp2 (double x) { return exp2(x); }
static double f_fact (double x) { double r = tgamma(x + 1); return valid(r) ? r : 0; }
static double f_round(double x) { return round(x); }
static double f_sign (double x) { return x > 0 ? 1.0 : (x < 0 ? -1.0 : 0.0); }
static double g_pow  (double y, double x) { double r = pow(y, x); return valid(r) ? r : 0; }
static double g_yroot(double y, double x) {
    double r = pow(x, 1.0 / y);
    if (!valid(r) && x < 0) {                 // 负数奇次根：pow 返 NaN，补 -pow(-x)
        long long yi = (long long)y;
        if (y == (double)yi && (yi & 1)) r = -pow(-x, 1.0 / y);
    }
    return valid(r) ? r : 0;
}
static double g_nCr  (double y, double x) {
    long long n = (long long)y, k = (long long)x;
    if (n != y || k != x || n < 0 || k < 0 || k > n) return 0;
    if (k > n - k) k = n - k;
    if (k > 10000000) return 0; // 迭代上限（防大参数循环冻结 UI）
    double r = 1;
    for (long long i = 1; i <= k; i++) r = r * (n - k + i) / i;
    return valid(r) ? r : 0;
}
static double g_nPr  (double y, double x) {
    long long n = (long long)y, k = (long long)x;
    if (n != y || k != x || n < 0 || k < 0 || k > n) return 0;
    if (k > 10000000) return 0; // 迭代上限
    double r = 1;
    for (long long i = 0; i < k; i++) r *= (n - i);
    return valid(r) ? r : 0;
}

// ---- 阶乘/幂族在 MATH 页复用正常层函数 ----

// ---- 分数显示（连分数，仅 X 行显示，内部仍 double）----
static const char *fmtFrac(double v, char *out) {
    if (v != v) { sprintf(out, "%.6e", v); return out; } // NaN
    double av = fabs(v);
    if (av > 1e9 || av < 1e-6) { sprintf(out, "%.6e", v); return out; } // 界外保持小数
    if (av == (double)(long long)av) { sprintf(out, "%lld", (long long)v); return out; } // 整数
    double d = av;
    long long n0 = 0, d0 = 1, n1 = 1, d1 = 0;  // 连分数逼近（0/1, 1/0）
    double eps = fabs(v) * 1e-12 + 1e-12;
    for (int i = 0; i < 30; i++) {
        long long a = (long long)floor(d + 1e-12); // 当前项
        long long n2 = a * n1 + n0, d2 = a * d1 + d0;
        if (d2 > 999999 || n2 > 999999) break;     // 分子/分母位数上限
        n0 = n1; d0 = d1; n1 = n2; d1 = d2;
        double rem = d - a;
        if (fabs(av - (double)n1 / d1) < eps) {    // 已达精度（终止阈值 1e-12*|v|）
            if (v < 0) sprintf(out, "-%lld/%lld", n1, d1);
            else sprintf(out, "%lld/%lld", n1, d1);
            return out;
        }
        if (rem < 1e-15) break;
        d = 1.0 / rem;
    }
    sprintf(out, "%.12g", v); // 转不出（√2/π 等）保持 12 位小数显示
    return out;
}

// ---- MATH 菜单（5 页 × 6 项）----
// 标题中英文对照（GBK 中文，HZK16S 渲染需混排绘制）
static const char *mathTitles[5] = {
    "\xBD\xC7\xB6\xC8\xA1\xA4\xB0\xD9\xB7\xD6\xB1\xC8/ANGLE\xA1\xA4PCT", // 角度·百分比/ANGLE·PCT
    "\xB3\xA3\xC1\xBF/BASIC",     // 常量/BASIC
    "\xCB\xAB\xC7\xFA/HYPER",     // 双曲/HYPER
    "\xB8\xC5\xC2\xCA/PROB",      // 概率/PROB
    "\xC3\xDD/POWER"              // 幂/POWER
};

// 混排绘制（GBK 双字节中文 16px + ASCII 16px；返回新 x）
int drawTextMix(int x, int y, const char *s, uint8_t fg, int16_t bg) {
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c >= 0x81 && c < 0xFF && s[1]) {
            unsigned char c2 = (unsigned char)s[1];
            if (c2 >= 0x40 && c2 < 0xFF && c2 != 0x7F) {
                uidisp->draw_char_GBK16(x, y, (uint16_t)((c << 8) | c2), fg, bg);
                x += 16;
                s += 2;
                continue;
            }
        }
        uidisp->draw_char_ascii(x, y, *s, 16, fg, bg);
        x += 8;
        s++;
    }
    return x;
}
static const char *mathItems[5][6] = {
    {"DEG", "RAD", "GRAD", "%", "\xDC%", "%T"},   // 页 1：角度切换 + 12C 百分比（% / Δ% / %T）
    {"e", "sign", "round", "floor", "ceil", ""},
    {"sinh", "cosh", "tanh", "asinh", "acosh", "atanh"},
    {"nCr", "nPr", "RAND", "", "", ""},
    {"x3", "10^x", "2^x", "", "", ""}
};

// 百分比（12C 语义：只改 X，Y 保持——算完可直接 + 得合计价）；lastX=旧 X，autoLift=1
// k=0: %（Y 的 X%）X=Y*X/100；k=1: Δ%（相对变化 (X-Y)/Y*100）；k=2: %T（X 占 Y 的 % X/Y*100）
static void percentOp(int k) {
    double x, y;
    if (entering) { x = atof(inbuf); entering = 0; inlen = 0; }
    else x = stX;
    y = stY;
    lastX = x;
    double r;
    if (k == 0) r = y * x / 100.0;
    else if (k == 1) r = (y != 0) ? (x - y) * 100.0 / y : 0;
    else r = (y != 0) ? x * 100.0 / y : 0;
    if (!valid(r)) r = 0;
    stX = r;
    autoLift = 1;
}

static void mathExec(int slot) {
    switch (mathPage) {
        case 0: // 角度 + 百分比（% / Δ% / %T）
            if (slot == 0) { angMode = 0; saveRegs(); }
            else if (slot == 1) { angMode = 1; saveRegs(); }
            else if (slot == 2) { angMode = 2; saveRegs(); }
            else if (slot >= 3 && slot <= 5) percentOp(slot - 3);
            break;
        case 1: // e/sign/round/floor/ceil（abs 已移到 Shift+(-) 物理键）
            if (slot == 0) pushConst(M_E);
            else if (slot == 1) unaryOp(f_sign, 0, 0);
            else if (slot == 2) unaryOp(f_round, 0, 0);
            else if (slot == 3) unaryOp(floor, 0, 0);
            else if (slot == 4) unaryOp(ceil, 0, 0);
            break;
        case 2: // 双曲
            if (slot == 0) unaryOp(sinh, 0, 0);
            else if (slot == 1) unaryOp(cosh, 0, 0);
            else if (slot == 2) unaryOp(tanh, 0, 0);
            else if (slot == 3) unaryOp(asinh, 0, 0);
            else if (slot == 4) unaryOp(acosh, 0, 0);
            else if (slot == 5) unaryOp(atanh, 0, 0);
            break;
        case 3: // nCr/nPr/RAND
            if (slot == 0) binaryOp(g_nCr);
            else if (slot == 1) binaryOp(g_nPr);
            else if (slot == 2) {
                double x = (entering ? atof(inbuf) : stX);
                if (entering) { entering = 0; inlen = 0; }
                static uint32_t randCtr = 0;
                randCtr = randCtr * 1664525 + 1013904223; // LCG 递推（计数器熵，防同 X 恒同值）
                srand((unsigned)(xTaskGetTickCount() ^ (unsigned)(x * 1e6) ^ randCtr)); // tick+X+计数器混合种子
                lastX = stX;
                stX = rand() / (RAND_MAX + 1.0);
                autoLift = 1;
            }
            break;
        case 4: // x³/10^x/2^x
            if (slot == 0) unaryOp(f_cube, 0, 0);
            else if (slot == 1) unaryOp(f_pow10, 0, 0);
            else if (slot == 2) unaryOp(f_exp2, 0, 0);
            break;
    }
}

// 菜单项（F1-F6；Sum+=Σ+ 统计收集、Sum-=Σ- 撤最后）
static const char *menuItems[6] = {"x<>y", "Rdn", "DROP", "Sum+", "Sum-", ""};

void drawXLine(void); // 前置声明（draw 内调用）
static void drawMathPage(void);

// ---- 寄存器列表绘制（rpnMode==3）----
static void drawRegList(void) {
    char buf[48];
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    uidisp->draw_printf(0, 0, 12, 0, 255, "VARS: UP/DOWN ENT=RCL");
    for (int i = 0; i < 8; i++) {
        int idx = regTop + i;
        if (idx >= 26) break;
        char nm[2] = {(char)('A' + idx), 0};
        const char *val = fmtNum(regs[idx], buf);
        if (idx == regSel)
            uidisp->draw_printf(0, 12 + i * 14, 12, 255, 0, "> %s: %s", nm, val); // 反色高亮
        else
            uidisp->draw_printf(0, 12 + i * 14, 12, 0, 255, "  %s: %s", nm, val);
    }
    if (clearConfirm)
        uidisp->draw_printf(0, 118, 12, 255, 0, "Confirm? Shift+BKSP clear, ON cancel");
    else
        uidisp->draw_printf(0, 118, 12, 0, 255, "A-Z; Shift+BKSP = clear all");
    uidisp->flush();
}

// ---- MATH 菜单绘制（rpnMode==4）----
static void drawMathPage(void) {
    char buf[64];
    uidisp->draw_box(0, 0, 255, 127, 255, 255); // 白底
    sprintf(buf, "MATH %d/5: %s", mathPage + 1, mathTitles[mathPage]);
    drawTextMix(0, 0, buf, 0, 255); // 中英对照标题（混排绘制）
    for (int i = 0; i < 6; i++) {
        const char *t = mathItems[mathPage][i];
        if (t[0] == 0) t = "--";
        char lb[48];
        sprintf(lb, "F%d %s", i + 1, t);
        uidisp->draw_printf(0, 16 + i * 18, 16, 0, 255, "%s", lb);
    }
    uidisp->draw_printf(0, 119, 8, 0, 255, "L/R page  ON exit");
    uidisp->flush();
}

// ---- 绘制 ----（左对齐，T/Z/Y 24px，X 32px 大字，菜单 12px 六段均分）
static void draw(void) {
    char buf[48];
    int i;
    uidisp->draw_box(0, 0, 255, 127, 255, 255); // 白底
    if (rpnMode == 1) {
        const char *op = stoOp == 0 ? "" : stoOp == 1 ? "+" : stoOp == 2 ? "-" : stoOp == 3 ? "*" : "/";
        uidisp->draw_printf(0, 0, 12, 255, 0, "STO%s _", op);
    }
    else if (rpnMode == 2)
        uidisp->draw_printf(0, 0, 12, 255, 0, "RCL _");
    else if (rpnMode == 4)
        ; // MATH 态由 drawMathPage 单独绘制
    else if (shiftHeld)
        uidisp->draw_printf(0, 0, 12, 255, 0, "RPN39 S");
    else
        uidisp->draw_printf(0, 0, 12, 0, 255, "RPN39");
    // 角度状态：标题右上角（正常态）
    if (rpnMode == 0) {
        const char *a = angMode == 0 ? "DEG" : angMode == 1 ? "RAD" : "GRAD";
        if (shiftHeld) uidisp->draw_printf(100, 0, 12, 255, 0, "%s", a);
        else uidisp->draw_printf(100, 0, 12, 0, 255, "%s", a);
    }
    uidisp->draw_printf(0, 14, 24, 0, 255, "T: %s", fmtNum(stT, buf));
    uidisp->draw_printf(0, 40, 24, 0, 255, "Z: %s", fmtNum(stZ, buf));
    uidisp->draw_printf(0, 66, 24, 0, 255, "Y: %s", fmtNum(stY, buf));
    drawXLine();
    // 菜单行：黑底条 + 六段均分（每段 42px），空位显示占位
    uidisp->draw_box(0, 112, 255, 127, 255, 0);
    for (i = 1; i < 6; i++)
        uidisp->draw_line(i * 42, 114, i * 42, 126, 255); // 段间隔竖线
    for (i = 0; i < 6; i++) {
        const char *t = menuItems[i];
        if (t[0] == 0) t = "_";
        uidisp->draw_printf(i * 42 + 2, 114, 12, 255, 0, "%s", t);
    }
    uidisp->flush();
}

// X 行：标签 X: + 数字（32px 大字，最多 13 字符；分数模式显示分数）
void drawXLine(void) {
    char buf[48], buf2[48], fbuf[48];
    const char *num = NULL;
    if (entering) num = inbuf;
    else if (fracMode) { num = fmtFrac(stX, fbuf); }
    else num = fmtNum(stX, buf);
    if ((int)strlen(num) > 13) { // 13 字符 x 16px = 208px + 标签 48px
        if (entering) { // 编辑态超长：截前 12 字符保留可读性（非渲染旧栈值）
            memcpy(buf2, inbuf, 12); buf2[12] = 0;
            num = buf2;
        } else { // 非编辑态：科学计数压缩
            sprintf(buf2, "%.6e", stX);
            num = buf2;
        }
    }
    uidisp->draw_printf(0, 92, 24, 0, 255, "X: %s", num);
}

// X 行区域刷新（输入变化专用）
static void drawX(void) {
    uidisp->draw_box(0, 92, 255, 115, 255, 255);
    drawXLine();
    uidisp->flushRect(0, 92, 255, 115);
}

// ---- MATH 菜单处理（rpnMode==4，先于其他分支）----
static int mathKey(int key) {
    // KEY_F1-F6 物理码不连续（0/9/1/2/3/11），必须显式映射 slot
    switch (key) {
        case KEY_F1: mathExec(0); rpnMode = 0; return 0;
        case KEY_F2: mathExec(1); rpnMode = 0; return 0;
        case KEY_F3: mathExec(2); rpnMode = 0; return 0;
        case KEY_F4: mathExec(3); rpnMode = 0; return 0;
        case KEY_F5: mathExec(4); rpnMode = 0; return 0;
        case KEY_F6: mathExec(5); rpnMode = 0; return 0;
        default: break;
    }
    if (key == KEY_LEFT || key == KEY_UP) { mathPage = (mathPage + 4) % 5; return 0; } // 循环：第 1 页左跳第 5 页
    if (key == KEY_RIGHT || key == KEY_DOWN) { mathPage = (mathPage + 1) % 5; return 0; }
    switch (key) { // 快捷键：数字键 1-5 跳对应页
        case KEY_1: mathPage = 0; return 0;
        case KEY_2: mathPage = 1; return 0;
        case KEY_3: mathPage = 2; return 0;
        case KEY_4: mathPage = 3; return 0;
        case KEY_5: mathPage = 4; return 0;
        default: break;
    }
    if (key == KEY_ON || key == KEY_VIEWS || key == KEY_HOME) { rpnMode = 0; return 0; }
    return 0;
}

// ---- 按键处理（返回 1 = 仅 X 行变化（区域刷新），0 = 全屏刷新）----
static int handleKey(int key, int shift) {
    // 扩展模式（5=CPLX 复数 6=MATX 矩阵 7=STAT 统计）
    if (rpnMode >= 5) return rpn39ExtKey(rpnMode, key, shift);
    // 寄存器列表模式（rpnMode==3）：方向键移动/ENT=RCL/VIEWS/ON 退出
    if (rpnMode == 3) {
        if (key == KEY_UP) { if (regSel > 0) { regSel--; if (regSel < regTop) regTop = regSel; } return 0; }
        if (key == KEY_DOWN) { if (regSel < 25) { regSel++; if (regSel > regTop + 7) regTop = regSel - 7; } return 0; }
        if (key == KEY_ENTER) { // ENT：RCL 选中寄存器（栈提升，42S）
            double v = regs[regSel];
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            lastX = stX;
            stackLift();
            stX = v;
            autoLift = 0;
            rpnMode = 0;
            return 0;
        }
        if (key == KEY_VIEWS || key == KEY_ON || key == KEY_HOME) { rpnMode = 0; clearConfirm = 0; return 0; }
        return 0;
    }
    // STO/RCL 等待字母（rpnMode 1/2）
    if (rpnMode == 1 || rpnMode == 2) {
        int mode = rpnMode; // 保存（下面清 rpnMode）
        // STO 运算前缀（42S STO+−×÷）：STO 态按 Shift+四则 → 挂起运算，等字母后执行
        if (shift && mode == 1) {
            switch (key) {
                case KEY_PLUS: stoOp = 1; return 0;
                case KEY_SUBTRACTION: stoOp = 2; return 0;
                case KEY_MULTIPLICATION: stoOp = 3; return 0;
                case KEY_DIVISION: stoOp = 4; return 0;
                default: break;
            }
        }
        int idx = regKeyToIdx(key);
        rpnMode = 0; // 无论是否命中先退出等待态（非字母=取消）
        if (idx >= 0) {
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            if (mode == 1) {          // STO：存 X（或 STO 运算 R op= X 后存回）
                if (stoOp == 0) regs[idx] = stX;
                else {
                    double v = stX;
                    if (stoOp == 1) regs[idx] += v;
                    else if (stoOp == 2) regs[idx] -= v;
                    else if (stoOp == 3) regs[idx] *= v;
                    else if (v != 0) regs[idx] /= v; // 除 0：寄存器保持
                }
                saveRegs();
            } else {                  // RCL：寄存器值压栈（42S stack lift）
                lastX = stX;
                stackLift();
                stX = regs[idx];
                autoLift = 0;
            }
        }
        stoOp = 0; // 复位挂起运算（无论成功/取消）
        return 0;
    }
    // MATH 菜单（rpnMode 4）
    if (rpnMode == 4) return mathKey(key);
    // Shift 层：直接键位的 shift 功能（无定义的组合忽略）
    if (shift) {
        switch (key) {
            case KEY_SIN: unaryOp(asin, 0, 1);  return 0; // ASIN（结果转当前角度）
            case KEY_COS: unaryOp(acos, 0, 1);  return 0; // ACOS
            case KEY_TAN: unaryOp(atan, 0, 1);  return 0; // ATAN
            case KEY_LN:  unaryOp(exp, 0, 0);   return 0; // e^x
            case KEY_LOG: unaryOp(f_pow10, 0, 0); return 0; // 10^x
            case KEY_X2:  unaryOp(sqrt, 0, 0);  return 0; // √X
            case KEY_XY:  binaryOp(g_yroot);    return 0; // X 的 Y 次根
            case KEY_DIVISION: unaryOp(f_rec, 0, 0); return 0; // 1/X
            case KEY_MULTIPLICATION: unaryOp(f_fact, 0, 0); return 0; // X!
            case KEY_NEGATIVE: unaryOp(f_abs, 0, 0); return 0; // ABS（原厂 Shift+(-) = ABS）
            case KEY_3:  pushConst(M_PI);       return 0; // π 压栈
            case KEY_7:  rpnMode = 7; rpn39ExtEnter(7); return 0; // 统计（原 LIST 键位）
            case KEY_8:  rpnMode = 6; rpn39ExtEnter(6); return 0; // 矩阵（原 MATRIX 键位）
            case KEY_9:  rpnMode = 5; rpn39ExtEnter(5); return 0; // 复数（原 PRGM 键位）
            default: return 0;                            // 未定义 shift 组合忽略
        }
    }
    int d = -1;
    switch (key) {
        case KEY_0: d = 0; break;
        case KEY_1: d = 1; break;
        case KEY_2: d = 2; break;
        case KEY_3: d = 3; break;
        case KEY_4: d = 4; break;
        case KEY_5: d = 5; break;
        case KEY_6: d = 6; break;
        case KEY_7: d = 7; break;
        case KEY_8: d = 8; break;
        case KEY_9: d = 9; break;
        default: break;
    }
    if (d >= 0) {
        if (!entering) {
            if (autoLift) { stackLift(); autoLift = 0; } // 42S：计算后输入自动压栈（X->Y）
            entering = 1; inlen = 0; inbuf[0] = 0;
        }
        if (inlen < 12) { inbuf[inlen++] = (char)('0' + d); inbuf[inlen] = 0; }
        return 1;
    }
    switch (key) {
        case KEY_DOT:
            if (!entering) {
                if (autoLift) { stackLift(); autoLift = 0; }
                entering = 1; inlen = 0; inbuf[0] = 0;
            }
            if (inlen < 12 && !strchr(inbuf, '.')) { inbuf[inlen++] = '.'; inbuf[inlen] = 0; }
            return 1;
        case KEY_NEGATIVE:
            if (entering) {
                char *ep = strchr(inbuf, 'e'); // EEX 指数区符号
                if (ep) {
                    if (ep[1] == '-') { memmove(ep + 1, ep + 2, strlen(ep + 1) + 1); inlen--; }
                    else if (inlen < 12) { memmove(ep + 2, ep + 1, strlen(ep + 1) + 1); ep[1] = '-'; inlen++; }
                } else if (inbuf[0] == '-') { memmove(inbuf, inbuf + 1, inlen); inlen--; }
                else if (inlen < 12) { memmove(inbuf + 1, inbuf, inlen + 1); inbuf[0] = '-'; inlen++; }
            } else {
                stX = -stX;
                autoLift = 0;
            }
            return 1;
        case KEY_XTPHIN: // EEX 科学计数输入（正常层；STO/RCL 上下文 = d 寄存器）
            if (!entering) {
                if (autoLift) { stackLift(); autoLift = 0; }
                entering = 1; inlen = 0; inbuf[0] = 0;
                inbuf[inlen++] = '0';
            }
            if (!strchr(inbuf, 'e') && inlen < 12) { inbuf[inlen++] = 'e'; inbuf[inlen] = 0; }
            return 1;
        case KEY_BACKSPACE:
            if (entering && inlen > 0) { inlen--; inbuf[inlen] = 0; return 1; }
            if (!entering) { // 非输入态：从当前 X 开始编辑（退格再继续）
                char tmp[40];
                sprintf(tmp, "%.12g", stX);
                strcpy(inbuf, tmp);
                inlen = (int)strlen(inbuf);
                entering = 1;
                if (inlen > 0) { inlen--; inbuf[inlen] = 0; }
                autoLift = 0;
                return 1;
            }
            break;
        case KEY_ENTER:
            if (entering) {
                double v = atof(inbuf);
                entering = 0; inlen = 0;
                lastX = stX;
                stX = v;
                stackLift(); // Y=输入值（压栈），X 保持（42S）
            } else {
                stackLift(); // 复制 X 压栈（42S：X 保持）
            }
            autoLift = 0;
            break;
        case KEY_PLUS:
        case KEY_SUBTRACTION:
        case KEY_MULTIPLICATION:
        case KEY_DIVISION: {
            double x, y, r;
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            y = stY; x = stX;
            lastX = x;
            switch (key) {
                case KEY_PLUS: r = y + x; break;
                case KEY_SUBTRACTION: r = y - x; break;
                case KEY_MULTIPLICATION: r = y * x; break;
                default: r = (x != 0) ? y / x : 0; break; // 除零 -> 0（阶段 1 简化）
            }
            stX = r;
            stY = stZ; stZ = stT; // Y 被消费，栈下移
            autoLift = 1;         // 42S：计算后输入自动压栈
            break;
        }
        // ---- 阶段 2：直接键位科学函数 ----
        case KEY_SIN: unaryOp(sin, 1, 0);  return 0; // 角度按当前模式
        case KEY_COS: unaryOp(cos, 1, 0);  return 0;
        case KEY_TAN: unaryOp(tan, 1, 0);  return 0;
        case KEY_LN:  unaryOp(log, 0, 0);  return 0; // ln
        case KEY_LOG: unaryOp(log10, 0, 0); return 0; // log10
        case KEY_X2:  unaryOp(f_sq, 0, 0); return 0; // X²
        case KEY_XY:  binaryOp(g_pow);     return 0; // Y^X
        case KEY_MATH: // MATH 数学功能菜单（翻页）
            rpnMode = 4; mathPage = 0;
            return 0;
        case KEY_ABC: // a b/c：小数/分数显示切换（仅 X 行；输入中先收尾）
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            fracMode = !fracMode;
            return 1;
        case KEY_F1: // x<>y（输入中先收尾：值进 X）
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            { double t = stX; stX = stY; stY = t; autoLift = 0; }
            break;
        case KEY_F2: // R↓
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            { double t = stT; stT = stZ; stZ = stY; stY = stX; stX = t; autoLift = 0; }
            break;
        case KEY_F3: // DROP
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            stackDrop(); autoLift = 0;
            break;
        case KEY_F4: // Sum+（Σ+）：X 收集进统计
            rpn39StatAccum(1);
            break;
        case KEY_F5: // Sum-（Σ-）：撤销最后一个收集点
            rpn39StatAccum(0);
            break;
        case KEY_LEFTBRACKET: // ( : STO（等字母）
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            rpnMode = 1;
            break;
        case KEY_RIGHTBRACKET: // ) : RCL（等字母）
            rpnMode = 2;
            break;
        case KEY_VARS: // VARS：寄存器列表
            rpnMode = 3;
            regSel = 0; regTop = 0;
            break;
        case KEY_ON:
            if (entering) { entering = 0; inlen = 0; autoLift = 0; return 1; }
            break; // ON 短按非输入无操作（退出用 Shift+ON 或 HOME）
        case KEY_HOME:
            rpn39Running = 0;
            break;
        default:
            break;
    }
    return 0;
}

// ---- 任务 ----
void rpn39Task(void *_) {
    SystemUISuspend();
    uidisp->restoreBuffer(); // UI_Suspend 已释放 disp_buf（releaseBuffer），重新分配堆缓冲（panic 尝试：避开 emergencyBuffer 固定区）
    rpn39Running = 1;
    loadRegs();
    draw();
    int lastKey = -1;
    shiftHeld = 0;
    while (rpn39Running) {
        uint32_t keys = ll_vm_check_key();
        uint32_t kp = keys >> 16;
        uint32_t key = keys & 0xFFFF;
        if (kp) {
            if (key == KEY_SHIFT) { // Shift 按下沿：激活（指示 + 标题 S）
                if (key != (uint32_t)lastKey) {
                    shiftHeld = 1;
                    ll_disp_set_indicator(INDICATE_LEFT, -1);
                    if (rpnMode == 3) drawRegList(); // 列表模式保持列表视图
                    else draw();
                }
                lastKey = key;
            } else if (key != (uint32_t)lastKey) { // 按下沿（防重复）
                lastKey = key;
                if (key == KEY_BACKSPACE && shiftHeld) {
                    if (rpnMode == 3) { // Vars 列表：Shift+backspace = CLEAR ALL（带确认）
                        if (clearConfirm) {
                            memset(regs, 0, sizeof(regs));
                            saveRegs();
                            clearConfirm = 0;
                            regSel = 0; regTop = 0;
                        } else {
                            clearConfirm = 1; // 第一次：进确认态
                        }
                    } else {
                        stX = 0; entering = 0; inlen = 0; autoLift = 0; // CLx（Shift+backspace）
                    }
                    shiftHeld = 0; ll_disp_set_indicator(0, -1);   // 动作完成自动退 shift
                    if (rpnMode == 3) drawRegList(); // 统一绘制（单次刷新）
                    else draw();
                } else if (key == KEY_ON && shiftHeld) {
                    rpn39Running = 0; // Shift+ON 退出（不触发系统关机）
                } else {
                    int r = handleKey((int)key, shiftHeld);
                    if (shiftHeld) { // shift + 普通键：动作后自动退 shift
                        shiftHeld = 0;
                        ll_disp_set_indicator(0, -1);
                        if (rpnMode == 3) drawRegList();       // 保持列表/菜单视图
                        else if (rpnMode == 4) drawMathPage();
                        else if (rpnMode >= 5) rpn39ExtDraw(rpnMode);
                        else draw();
                    } else {
                        if (rpnMode == 3) drawRegList(); // 列表模式全屏
                        else if (rpnMode == 4) drawMathPage();
                        else if (rpnMode >= 5) rpn39ExtDraw(rpnMode);
                        else if (r) drawX();             // 输入变化：X 行区域刷新
                        else draw();                     // 栈/菜单变化：全屏刷新
                    }
                }
            }
        } else {
            lastKey = -1; // 松开：仅复位边沿检测（shift 保持到动作完成）
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (shiftHeld) { shiftHeld = 0; ll_disp_set_indicator(0, -1); }
    if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; } // 输入中退出：先落值
    saveRegs(); // 退出保存（寄存器 + 栈 + 角度模式——HP 关机保留语义）
    if (rpnMode >= 5) rpn39ExtExit(rpnMode); // 扩展态退出持久化（复数/矩阵/统计）
    entering = 0; inlen = 0; inbuf[0] = 0; // 清输入态（避免重进残留）
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    uidisp->flush();
    SystemUIResume();
    vTaskDelete(NULL);
}

extern "C" void StartRPN39() {
    xTaskCreate(rpn39Task, "RPN39", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
}