// RPN39 扩展模块：复数（CPLX）/矩阵（MATX）/统计（STAT）——阶段 3（方案 A：复寄存器式）
// 设计见 docs/RPN39-design.md §阶段 3（42S 功能参考，实现原创）
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

// ================= 共用小工具 =================

// 压缩数值格式（≤7 字符，格/复数分量用）
static const char *fmt7(double v, char *b) {
    if (v > -1e8 && v < 1e8 && v == (double)(long long)v)
        sprintf(b, "%lld", (long long)v);
    else
        sprintf(b, "%.6g", v);
    if ((int)strlen(b) > 7) sprintf(b, "%.4e", v);
    return b;
}

// 复数格式化：rect = "a+bi"；polar = "r∠θ"（θ 按当前角度单位显示，标 ° 于 DEG）
// 返回长度（含 GBK 双字节计 1）
static int cplx2str(double re, double im, char *out, int polar) {
    char ra[16], rb[16];
    if (!polar) {
        fmt7(re, ra); fmt7(fabs(im), rb);
        if (im < 0)
            return sprintf(out, "%s-%si", ra, rb);
        return sprintf(out, "%s+%si", ra, rb);
    }
    double r = hypot(re, im);
    double th = (im == 0 && re == 0) ? 0 : fromRad(atan2(im, re));
    fmt7(r, ra); fmt7(fabs(th), rb);
    int l = sprintf(out, "%s", ra);
    out[l++] = (char)0xA1; out[l++] = (char)0xCF; // ∠
    if (th < 0) out[l++] = '-';
    strcpy(out + l, rb); l += (int)strlen(rb);
    if (angMode == 0) { out[l++] = (char)0xA1; out[l++] = (char)0xE3; } // °（仅 DEG 标注）
    out[l] = 0;
    return l;
}

// ==================== 复数 CPLX（rpnMode=5）====================
// Z1 先入（Y 位），Z0 后入（X 位=结果显示位）：Z0 = Z1 op Z0
static double cZ0re = 0, cZ0im = 0, cZ1re = 0, cZ1im = 0;
static int cplxPage = 0;   // 0/1/2
static int cplxPolar = 0;  // 0=RECT 1=POLAR
static int cplxInit = 0;

static const char *cplxMenus[3][6] = {
    {"LOAD1", "LOAD0", "+", "-", "x", "/"},
    {"CONJ", "|Z0|", "1/Z0", "ARG", "Z0XY", "R/P"},
    {"R>P", "P>R", "Z1Z0", "CL1", "CL0", ""}
};
static const char *cplxHints[3] = {
    "LOAD reads stack (Y=Re, X=Im)",
    "pg3: R>P P>R act on stack (Y,X)",
    "F4 CL1 / F5 CL0 clear Z1/Z0"
};

static void saveCplx(void) {
    FIL f;
    if (f_open(&f, "/rpn39_cplx.dat", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        UINT bw;
        double d[5] = {cZ0re, cZ0im, cZ1re, cZ1im, (double)cplxPolar}; // 第 5 值=显示模式
        f_write(&f, d, sizeof(d), &bw);
        f_close(&f);
    }
}
static void loadCplx(void) {
    FIL f; UINT br = 0;
    if (f_open(&f, "/rpn39_cplx.dat", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        double d[5] = {0};
        f_read(&f, d, sizeof(d), &br);
        if (br >= 4 * sizeof(double)) { cZ0re = d[0]; cZ0im = d[1]; cZ1re = d[2]; cZ1im = d[3]; }
        if (br >= 5 * sizeof(double)) cplxPolar = (int)d[4]; // 旧 4 值文件保持默认 0
        f_close(&f);
    }
}

// 从主栈 (Y,X) 载入复数（42S：RE 在 Y、IM 在 X）
static void cplxLoad(int which) {
    double x, y;
    if (entering) { x = atof(inbuf); entering = 0; inlen = 0; }
    else x = stX;
    y = stY;
    if (which == 0) { cZ0re = y; cZ0im = x; }
    else { cZ1re = y; cZ1im = x; }
    cplxPolar = 0; // 载入是直角坐标输入：自动回 RECT 显示（F6 可再切 POLAR）
    saveCplx();
}

static void cplxOp(int op) {
    double a = cZ1re, b = cZ1im, c = cZ0re, d = cZ0im, r, s;
    switch (op) {
        case 0: r = a + c; s = b + d; break;                              // +
        case 1: r = a - c; s = b - d; break;                              // -
        case 2: r = a * c - b * d; s = a * d + b * c; break;              // ×
        default: {                                                        // ÷（Z1/Z0）
            double den = c * c + d * d;
            if (den == 0) { r = 0; s = 0; break; }
            r = (a * c + b * d) / den;
            s = (b * c - a * d) / den;
        }
    }
    cZ0re = r; cZ0im = s;
    saveCplx();
}

static void cplxOp1(int op) { // 页 1：CONJ/|Z0|/1/Z0/ARG
    double c = cZ0re, d = cZ0im;
    if (op == 0) { cZ0im = -d; }                       // CONJ
    else if (op == 1) { cZ0re = hypot(c, d); cZ0im = 0; }        // |Z0|
    else if (op == 2) { double den = c * c + d * d;   // 1/Z0
        if (den != 0) { cZ0re = c / den; cZ0im = -d / den; } }
    else if (op == 3) { cZ0re = fromRad(atan2(d, c)); cZ0im = 0; } // ARG
    saveCplx();
}

// Z0 → 主栈 (Y,X)
static void cplxToXY(void) {
    stY = cZ0re; stX = cZ0im;
    autoLift = 0;
}

// R→P：主栈 (Y,X) 直角 → Y=r X=θ（θ 按角度模式）；P→R 逆
static void cplxRP(int toPolar) {
    double x, y;
    if (entering) { x = atof(inbuf); entering = 0; inlen = 0; }
    else x = stX;
    y = stY;
    if (toPolar) { // R→P：主栈 (Y,X)=(re,im) → (r, θ)
        stY = hypot(x, y);
        stX = fromRad(atan2(x, y)); // θ=atan2(im=X, re=Y)
    } else {       // P→R：主栈 (Y,X)=(r, θ) → (re,im)
        double t = toRad(x);
        stY = y * cos(t);   // re = r·cosθ
        stX = y * sin(t);   // im = r·sinθ
    }
    autoLift = 1;
}

static void cplxDraw(void) {
    char buf[64];
    int i;
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    char hdr[40];
    sprintf(hdr, "CPLX %d/3  %s", cplxPage + 1, cplxPolar ? "POLAR" : "RECT");
    drawTextMix(0, 0, hdr, 0, 255);
    cplx2str(cZ1re, cZ1im, buf, cplxPolar);
    uidisp->draw_printf(0, 16, 12, 0, 255, "Z1 (first) : %s", buf);
    // Z0 黑底白字强调条
    uidisp->draw_box(0, 34, 255, 55, 255, 0);
    cplx2str(cZ0re, cZ0im, buf, cplxPolar);
    drawTextMix(4, 37, buf, 255, 0);
    uidisp->draw_printf(0, 60, 12, 0, 255, "Z1 op Z0 -> Z0   (F1=F5 chain)");
    uidisp->draw_printf(0, 74, 12, 0, 255, "%s", cplxHints[cplxPage]);
    uidisp->draw_printf(0, 90, 12, 0, 255, "<>/pg  BKSP:CL Z0  ON exit");
    // 菜单条
    uidisp->draw_box(0, 112, 255, 127, 255, 0);
    for (i = 1; i < 6; i++) uidisp->draw_line(i * 42, 114, i * 42, 126, 255);
    for (i = 0; i < 6; i++) {
        const char *t = cplxMenus[cplxPage][i];
        if (t[0] == 0) t = "_";
        uidisp->draw_printf(i * 42 + 2, 114, 12, 255, 0, "%s", t);
    }
    uidisp->flush();
}

static int cplxKey(int key, int shift) {
    (void)shift;
    if (key == KEY_LEFT || key == KEY_UP) { cplxPage = (cplxPage + 2) % 3; return 0; }
    if (key == KEY_RIGHT || key == KEY_DOWN) { cplxPage = (cplxPage + 1) % 3; return 0; }
    if (key == KEY_ON || key == KEY_VIEWS || key == KEY_HOME) { rpnMode = 0; saveCplx(); return 0; }
    if (key == KEY_BACKSPACE) { // 任何页退格清 Z0（=主界面 CLx 语义）
        cZ0re = 0; cZ0im = 0;
        saveCplx();
        return 0;
    }
    int slot = -1;
    switch (key) {
        case KEY_F1: slot = 0; break;
        case KEY_F2: slot = 1; break;
        case KEY_F3: slot = 2; break;
        case KEY_F4: slot = 3; break;
        case KEY_F5: slot = 4; break;
        case KEY_F6: slot = 5; break;
        default: break;
    }
    if (slot < 0) return 0;
    if (cplxPage == 0) {
        if (slot == 0) cplxLoad(1);   // F1 LOAD Z1（先入/左操作数）
        else if (slot == 1) cplxLoad(0); // F2 LOAD Z0（后入/右操作数+结果位）
        else if (slot >= 2 && slot <= 5) cplxOp(slot - 2);
    } else if (cplxPage == 1) {
        if (slot == 0) cplxOp1(0);       // CONJ
        else if (slot == 1) cplxOp1(1);  // |Z0|
        else if (slot == 2) cplxOp1(2);  // 1/Z0
        else if (slot == 3) cplxOp1(3);  // ARG
        else if (slot == 4) cplxToXY();
        else cplxPolar = !cplxPolar;
    } else {
        if (slot == 0) cplxRP(1);
        else if (slot == 1) cplxRP(0);
        else if (slot == 2) { double t; t = cZ0re; cZ0re = cZ1re; cZ1re = t; t = cZ0im; cZ0im = cZ1im; cZ1im = t; saveCplx(); }
        else if (slot == 3) { cZ1re = 0; cZ1im = 0; saveCplx(); }
        else if (slot == 4) { cZ0re = 0; cZ0im = 0; saveCplx(); }
    }
    return 0;
}

// ==================== 矩阵 MATX（rpnMode=6）====================
// 槽 A/B/R：n×n（n=1..4，容量 4×4）；运算 R=A±B / R=A×B / R=T(A) / R=INV(A)；DET→主栈 X
static double mA[16] = {0}, mB[16] = {0}, mR[16] = {0};
static int dA = 1, dB = 1, dR = 1;
static int edSlot = 0;    // 0=A 1=B 2=R（编辑槽）
static int cx = 0, cy = 0; // 高亮格
static int matxPage = 0;
static int matxInit = 0;
static char matxMsg[24] = "";
static int mEdOn = 0;    // 矩阵格文本编辑中（支持小数/负号/退格）
static char mBuf[16];
static int mLen = 0;

static const char *matxMenus[3][6] = {
    {"A+B", "A-B", "A*B", "T(A)", "T(B)", "Ed:R"},
    {"INV A", "INV B", "DET A", "DET B", "CLR R", "Ed:A"},
    {"SIZE", "R->A", "R->B", "CLR A", "CLR B", "Ed:B"}
};

static double *edM(int s) { return s == 0 ? mA : (s == 1 ? mB : mR); }
static int *edD(int s) { return s == 0 ? &dA : (s == 1 ? &dB : &dR); }

static void saveMatx(void) {
    FIL f;
    if (f_open(&f, "/rpn39_matx.dat", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        UINT bw;
        f_write(&f, mA, sizeof(mA), &bw);
        f_write(&f, mB, sizeof(mB), &bw);
        f_write(&f, mR, sizeof(mR), &bw);
        f_write(&f, edD(0), sizeof(int), &bw);
        f_write(&f, edD(1), sizeof(int), &bw);
        f_write(&f, edD(2), sizeof(int), &bw);
        f_write(&f, &edSlot, sizeof(int), &bw);
        f_close(&f);
    }
}
static void loadMatx(void) {
    FIL f; UINT br = 0;
    if (f_open(&f, "/rpn39_matx.dat", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        f_read(&f, mA, sizeof(mA), &br);
        if (br == sizeof(mA)) {
            f_read(&f, mB, sizeof(mB), &br);
            f_read(&f, mR, sizeof(mR), &br);
            f_read(&f, &dA, sizeof(int), &br);
            f_read(&f, &dB, sizeof(int), &br);
            f_read(&f, &dR, sizeof(int), &br);
            f_read(&f, &edSlot, sizeof(int), &br);
        }
        f_close(&f);
    }
    if (dA < 1 || dA > 4) dA = 1;
    if (dB < 1 || dB > 4) dB = 1;
    if (dR < 1 || dR > 4) dR = 1;
    if (edSlot < 0 || edSlot > 2) edSlot = 0;
    if (cx < 0 || cx > 3) cx = 0;
    if (cy < 0 || cy > 3) cy = 0;
}

static void matxOp(int op) { // 0=A+B 1=A-B 2=A*B 3=T(A) 4=T(B)（结果→R）
    int nA = dA, nB = dB, i, j, k;
    matxMsg[0] = 0;
    if ((op == 0 || op == 1) && nA != nB) { strcpy(matxMsg, "dim A != dim B"); return; }
    if (op == 2 && nA != nB) { strcpy(matxMsg, "dim A != dim B"); return; }
    int n = (op == 0 || op == 1 || op == 2) ? nA : (op == 3 ? nA : nB);
    const double *src = (op <= 2) ? mA : (op == 3 ? mA : mB);
    if (op == 2) { // R = A×B
        for (i = 0; i < n; i++)
            for (j = 0; j < n; j++) {
                double s = 0;
                for (k = 0; k < n; k++) s += mA[i * 4 + k] * mB[k * 4 + j];
                mR[i * 4 + j] = s;
            }
    } else if (op <= 1) {
        for (i = 0; i < n; i++)
            for (j = 0; j < n; j++)
                mR[i * 4 + j] = mA[i * 4 + j] + (op == 0 ? mB[i * 4 + j] : -mB[i * 4 + j]);
    } else { // 转置
        for (i = 0; i < n; i++)
            for (j = 0; j < n; j++)
                mR[i * 4 + j] = src[j * 4 + i];
    }
    dR = n;
    saveMatx();
}

// 高斯消元求 det（复制输入不改原矩阵）；奇异常返回 0
static double matDet(const double *m, int n) {
    double a[16];
    int i, j, k;
    memcpy(a, m, sizeof(a));
    double det = 1;
    for (i = 0; i < n; i++) {
        int p = i;
        for (j = i + 1; j < n; j++)
            if (fabs(a[j * 4 + i]) > fabs(a[p * 4 + i])) p = j;
        if (a[p * 4 + i] == 0) return 0;
        if (p != i) {
            for (k = 0; k < n; k++) { double t = a[i * 4 + k]; a[i * 4 + k] = a[p * 4 + k]; a[p * 4 + k] = t; }
            det = -det;
        }
        for (j = i + 1; j < n; j++) {
            double f = a[j * 4 + i] / a[i * 4 + i];
            for (k = i; k < n; k++) a[j * 4 + k] -= f * a[i * 4 + k];
        }
    }
    for (i = 0; i < n; i++) det *= a[i * 4 + i];
    return det;
}

static int matInv(const double *m, int n, double *out) {
    double a[16], b[16];
    int i, j, k;
    memcpy(a, m, 16 * sizeof(double));
    memset(b, 0, sizeof(b));
    for (i = 0; i < n; i++) b[i * 4 + i] = 1;
    for (i = 0; i < n; i++) {
        int p = i;
        for (j = i + 1; j < n; j++)
            if (fabs(a[j * 4 + i]) > fabs(a[p * 4 + i])) p = j;
        if (a[p * 4 + i] == 0) return -1;
        if (p != i) {
            for (k = 0; k < n; k++) {
                double t = a[i * 4 + k]; a[i * 4 + k] = a[p * 4 + k]; a[p * 4 + k] = t;
                t = b[i * 4 + k]; b[i * 4 + k] = b[p * 4 + k]; b[p * 4 + k] = t;
            }
        }
        double d = a[i * 4 + i];
        for (k = 0; k < n; k++) { a[i * 4 + k] /= d; b[i * 4 + k] /= d; }
        for (j = 0; j < n; j++) {
            if (j == i) continue;
            double f = a[j * 4 + i];
            if (f == 0) continue;
            for (k = 0; k < n; k++) { a[j * 4 + k] -= f * a[i * 4 + k]; b[j * 4 + k] -= f * b[i * 4 + k]; }
        }
    }
    memcpy(out, b, 16 * sizeof(double));
    return 0;
}

static void matxOp2(int op) { // 页 2：INV A / INV B / DET A / DET B / CLR R
    matxMsg[0] = 0;
    if (op == 0 || op == 1) {
        int n = op == 0 ? dA : dB;
        double out[16];
        if (matInv(op == 0 ? mA : mB, n, out) != 0) { strcpy(matxMsg, "singular"); return; }
        memcpy(mR, out, sizeof(mR));
        dR = n;
    } else if (op == 2 || op == 3) { // DET → 主栈 X
        int n = op == 2 ? dA : dB;
        if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
        lastX = stX;
        stX = matDet(op == 2 ? mA : mB, n);
        autoLift = 1;
        rpnMode = 0; // 回主界面显示 X
        return;
    } else { // CLR R
        memset(mR, 0, sizeof(mR));
        dR = 1;
    }
    saveMatx();
}

static void matxDraw(void) {
    int i, j, n;
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    char hdr[40];
    n = *edD(edSlot);
    sprintf(hdr, "MATX %d/3  edit %c (%dx%d)", matxPage + 1, 'A' + edSlot, n, n);
    drawTextMix(0, 0, hdr, 0, 255);
    if (matxMsg[0])
        uidisp->draw_printf(0, 14, 12, 255, 0, "%s", matxMsg);
    else
        uidisp->draw_printf(0, 14, 12, 0, 255, "digits edit  F6:slot  b:page  ON:exit");
    double *m = edM(edSlot);
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            int hl = (i == cy && j == cx);
            const char *txt;
            char tbuf[16];
            if (mEdOn && hl) { txt = mBuf; }
            else { fmt7(m[i * 4 + j], tbuf); txt = tbuf; }
            uidisp->draw_box(2 + j * 63, 30 + i * 20, 2 + j * 63 + 60, 30 + i * 20 + 17, hl ? 255 : 0, hl ? 0 : 255);
            uidisp->draw_printf(4 + j * 63, 32 + i * 20, 12, hl ? 255 : 0, hl ? 0 : 255, "%s", txt);
        }
    }
    uidisp->draw_printf(0, 92, 12, 0, 255, "F1-F5 op  F6: Ed slot  ON:exit");
    // 菜单条（3 页标签）
    uidisp->draw_box(0, 112, 255, 127, 255, 0);
    for (i = 1; i < 6; i++) uidisp->draw_line(i * 42, 114, i * 42, 126, 255);
    for (i = 0; i < 6; i++) {
        const char *t = matxMenus[matxPage][i];
        if (t[0] == 0) t = "_";
        uidisp->draw_printf(i * 42 + 2, 114, 12, 255, 0, "%s", t);
    }
    uidisp->flush();
}

static int matxDigit(int key) { // 数字键码不连续（行 9 的 0-9 与行 6-8 分开）：显式映射
    switch (key) {
        case KEY_0: return 0; case KEY_1: return 1; case KEY_2: return 2;
        case KEY_3: return 3; case KEY_4: return 4; case KEY_5: return 5;
        case KEY_6: return 6; case KEY_7: return 7; case KEY_8: return 8;
        case KEY_9: return 9;
        default: return -1;
    }
}

static int matxEditKey(int key) {
    int n = *edD(edSlot);
    double *v = &edM(edSlot)[cy * 4 + cx];
    int d;
    switch (key) {
        case KEY_UP: case KEY_DOWN: case KEY_LEFT: case KEY_RIGHT:
            if (mEdOn) { *v = atof(mBuf); mEdOn = 0; saveMatx(); }
            if (key == KEY_RIGHT) { // 行尾换行（wrap 格导航，不翻页）
                if (cx < n - 1) cx++;
                else { cx = 0; cy = (cy + 1) % n; }
            } else if (key == KEY_LEFT) {
                if (cx > 0) cx--;
                else { cx = n - 1; cy = (cy + n - 1) % n; }
            } else if (key == KEY_DOWN) cy = (cy + 1) % n;
            else cy = (cy + n - 1) % n;
            return 0;
        case KEY_ENTER:
            if (mEdOn) { *v = atof(mBuf); mEdOn = 0; saveMatx(); }
            if (cx < n - 1) cx++;
            else if (cy < n - 1) { cy++; cx = 0; }
            return 0;
        default: break;
    }
    if (key == KEY_BACKSPACE) {
        if (!mEdOn) { mEdOn = 1; mLen = 0; mBuf[0] = 0; }
        if (mLen > 0) { mLen--; mBuf[mLen] = 0; }
        return 0;
    }
    if (!mEdOn) {
        if (matxDigit(key) >= 0 || key == KEY_DOT || key == KEY_NEGATIVE) {
            mEdOn = 1; mLen = 0; mBuf[0] = 0;
        } else return 1;
    }
    if (key == KEY_NEGATIVE) {
        if (mLen == 0) { mBuf[mLen++] = '-'; mBuf[mLen] = 0; }
        else if (mBuf[0] == '-') { memmove(mBuf, mBuf + 1, (size_t)mLen); mLen--; }
        else if (mLen < 14) { memmove(mBuf + 1, mBuf, (size_t)mLen + 1); mBuf[0] = '-'; mLen++; }
        return 0;
    }
    if (key == KEY_DOT) {
        if (!strchr(mBuf, '.') && mLen < 14) { mBuf[mLen++] = '.'; mBuf[mLen] = 0; }
        return 0;
    }
    d = matxDigit(key);
    if (d >= 0 && mLen < 14) {
        mBuf[mLen++] = (char)('0' + d);
        mBuf[mLen] = 0;
        return 0;
    }
    return 0;
}

static void matxCommit(void) { // 提交正在编辑的格（切槽/翻页/退出/运算前防丢值）
    if (mEdOn) { edM(edSlot)[cy * 4 + cx] = atof(mBuf); mEdOn = 0; saveMatx(); }
}

static int matxKey(int key, int shift) {
    (void)shift;
    if (key == KEY_ON || key == KEY_VIEWS || key == KEY_HOME) { matxCommit(); rpnMode = 0; return 0; }
    if (key == KEY_MATH) { matxCommit(); matxPage = (matxPage + 1) % 3; return 0; } // b 键翻页（方向键不再翻页：只做格导航）
    int slot = -1;
    switch (key) {
        case KEY_F1: slot = 0; break;
        case KEY_F2: slot = 1; break;
        case KEY_F3: slot = 2; break;
        case KEY_F4: slot = 3; break;
        case KEY_F5: slot = 4; break;
        case KEY_F6: slot = 5; break;
        default: break;
    }
    if (slot < 0) return matxEditKey(key); // 方向/数字/编辑键
    matxCommit(); // 运算/切槽前提交当前编辑格（A/B 输入后半途操作值不丢）
    if (slot == 5) { // F6：编辑槽循环 A→B→R
        edSlot = (edSlot + 1) % 3;
        cx = cy = 0;
        return 0;
    }
    if (matxPage == 0) matxOp(slot);
    else if (matxPage == 1) matxOp2(slot);
    else if (matxPage == 2) {
        if (slot == 0) { // SIZE：当前槽维度循环 1-4（清矩阵）
            int *d = edD(edSlot);
            *d = (*d % 4) + 1;
            memset(edM(edSlot), 0, 16 * sizeof(double));
            if (cx > *d - 1) cx = 0;
            if (cy > *d - 1) cy = 0;
            saveMatx();
        } else if (slot == 1) { memcpy(mA, mR, sizeof(mA)); dA = dR; saveMatx(); }
        else if (slot == 2) { memcpy(mB, mR, sizeof(mB)); dB = dR; saveMatx(); }
        else if (slot == 3) { memset(mA, 0, sizeof(mA)); dA = 1; saveMatx(); }
        else if (slot == 4) { memset(mB, 0, sizeof(mB)); dB = 1; saveMatx(); }
    }
    return 0;
}

// ==================== 统计 STAT（rpnMode=7）====================
// 主界面 F4=Σ+（收集 X）F5=Σ−（撤最后点）；Shift+7 查看；↑↓ 选择 + ENT = 值→主栈 X
#define STAT_MAX 256
static double statData[STAT_MAX] = {0};
static int statN = 0;
static int statSel = 0; // 高亮统计行 0-7

static const char *statNames[8] = {"N", "MEAN", "POP sd", "SMP sd", "MIN", "MAX", "SUM", "SQ SUM"};

static void saveStat(void) {
    FIL f;
    if (f_open(&f, "/rpn39_stat.dat", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        UINT bw;
        f_write(&f, &statN, sizeof(int), &bw);
        f_write(&f, statData, statN * sizeof(double), &bw);
        f_close(&f);
    }
}
static void loadStat(void) {
    FIL f; UINT br = 0;
    if (f_open(&f, "/rpn39_stat.dat", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        f_read(&f, &statN, sizeof(int), &br);
        if (statN < 0 || statN > STAT_MAX) statN = 0;
        if (statN > 0) f_read(&f, statData, statN * sizeof(double), &br);
        f_close(&f);
    }
}

// 主界面收集入口（F4/F5 调用）：Σ+ 收 X；Σ− 撤最后点
// 注意：不自动加载持久化旧数据（新会话从空收集；查看旧数据先进 Shift+7 页加载）
void rpn39StatAccum(int add) {
    double x;
    if (add) {
        if (entering) { x = atof(inbuf); entering = 0; inlen = 0; }
        else x = stX;
        if (statN >= STAT_MAX) return;
        statData[statN++] = x;
    } else {
        if (statN > 0) statN--;
    }
    saveStat();
}

static void statDraw(void) {
    char buf[64], vb[40];
    int i;
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    drawTextMix(0, 0, "STAT \xcd\xb3\xbc\xc6", 0, 255);
    double s = 0, s2 = 0, mn = 0, mx = 0;
    for (i = 0; i < statN; i++) {
        double v = statData[i];
        s += v; s2 += v * v;
        if (i == 0) mn = mx = v;
        else { if (v < mn) mn = v; if (v > mx) mx = v; }
    }
    double vals[8];
    vals[0] = statN;
    vals[1] = statN ? s / statN : 0;
    double varp = statN ? s2 / statN - vals[1] * vals[1] : 0;
    if (varp < 0) varp = 0;
    vals[2] = statN ? sqrt(varp) : 0;
    vals[3] = statN > 1 ? sqrt(s2 / (statN - 1) - s * s / (statN * (statN - 1.0))) : 0;
    vals[4] = statN ? mn : 0;
    vals[5] = statN ? mx : 0;
    vals[6] = s;
    vals[7] = s2;
    for (i = 0; i < 8; i++) {
        fmtNum(vals[i], vb);
        sprintf(buf, "%-8s %s", statNames[i], vb);
        if (i == statSel) {
            uidisp->draw_box(0, 16 + i * 12, 255, 16 + i * 12 + 11, 0, 255);
            uidisp->draw_printf(2, 16 + i * 12, 12, 255, 0, "%s", buf);
        } else
            uidisp->draw_printf(2, 16 + i * 12, 12, 0, 255, "%s", buf);
    }
    uidisp->draw_box(0, 112, 255, 127, 255, 0);
    for (i = 1; i < 6; i++) uidisp->draw_line(i * 42, 114, i * 42, 126, 255);
    static const char *sm[6] = {"Sum+", "Sum-", "CLR", "_", "_", "_"};
    for (i = 0; i < 6; i++)
        uidisp->draw_printf(i * 42 + 2, 114, 12, 255, 0, "%s", sm[i]);
    uidisp->flush();
}

static int statKey(int key, int shift) {
    (void)shift;
    if (key == KEY_UP) { if (statSel > 0) statSel--; return 0; }
    if (key == KEY_DOWN) { if (statSel < 7) statSel++; return 0; }
    if (key == KEY_ENTER) { // 选中统计值 → 主栈 X
        double s = 0, s2 = 0, mn = 0, mx = 0, vv;
        int i;
        for (i = 0; i < statN; i++) {
            vv = statData[i];
            s += vv; s2 += vv * vv;
            if (i == 0) mn = mx = vv;
            else { if (vv < mn) mn = vv; if (vv > mx) mx = vv; }
        }
        double vals[8];
        vals[0] = statN;
        vals[1] = statN ? s / statN : 0;
        double varp = statN ? s2 / statN - vals[1] * vals[1] : 0;
        if (varp < 0) varp = 0;
        vals[2] = statN ? sqrt(varp) : 0;
        vals[3] = statN > 1 ? sqrt(s2 / (statN - 1) - s * s / (statN * (statN - 1.0))) : 0;
        vals[4] = statN ? mn : 0;
        vals[5] = statN ? mx : 0;
        vals[6] = s;
        vals[7] = s2;
        if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
        lastX = stX;
        stX = vals[statSel];
        autoLift = 1;
        rpnMode = 0;
        return 0;
    }
    if (key == KEY_ON || key == KEY_VIEWS || key == KEY_HOME) { rpnMode = 0; saveStat(); return 0; }
    int slot = -1;
    switch (key) {
        case KEY_F1: slot = 0; break;
        case KEY_F2: slot = 1; break;
        case KEY_F3: slot = 2; break;
        default: break;
    }
    if (slot == 0) rpn39StatAccum(1);       // Σ+
    else if (slot == 1) rpn39StatAccum(0);  // Σ−
    else if (slot == 2) { statN = 0; statSel = 0; saveStat(); }
    return 0;
}

// ==================== 分发（rpn39_int.h）====================
void rpn39ExtEnter(int mode) {
    if (mode == 5) { // CPLX：每次进入固定页 1/3（LOAD 区）——避免页残留导致 F 键功能错位
        if (!cplxInit) { loadCplx(); cplxInit = 1; }
        cplxPage = 0;
    }
    if (mode == 6) { // MATX：进入固定页 1/3（运算区）
        if (!matxInit) { loadMatx(); matxInit = 1; }
        matxPage = 0;
    }
    if (mode == 7) { if (statN == 0) loadStat(); statSel = 0; }
}
void rpn39ExtExit(int mode) {
    if (mode == 5) saveCplx();
    else if (mode == 6) saveMatx();
    else if (mode == 7) saveStat();
}
int rpn39ExtKey(int mode, int key, int shift) {
    if (mode == 5) return cplxKey(key, shift);
    if (mode == 6) return matxKey(key, shift);
    if (mode == 7) return statKey(key, shift);
    return 0;
}
void rpn39ExtDraw(int mode) {
    if (mode == 5) cplxDraw();
    else if (mode == 6) matxDraw();
    else if (mode == 7) statDraw();
}
