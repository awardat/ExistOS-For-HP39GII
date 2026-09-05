// FormCalc - 表单计算器（P1：框架 + TVM 货币时间价值）
// 设计见 docs/FormCalc-design.md（12C 全集表单化，实现原创）
// 导航：L0 功能列表（HOME 回此/退出用 Shift+ON）→ L1 模块列表 → L2 计算表单（View=公式视图）
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

extern UI_Display *uidisp;
extern "C" {
uint32_t ll_vm_check_key();
void SystemUISuspend();
void SystemUIResume();
}

// ---- 中文 GBK 字面量（HZK16S 渲染，勿改 UTF-8）----
#define ZH_BIAODAN  "\xB1\xED\xB5\xA5\xBC\xC6\xCB\xE3"          // 表单计算
#define ZH_JINRONG "\xBD\xF0\xC8\xDA"                            // 金融
#define ZH_JINRONGQI "\xBD\xF0\xC8\xDA\xBC\xC6\xCB\xE3\xC6\xF7"  // 金融计算器
#define ZH_DIANZI  "\xB5\xE7\xD7\xD3\xB9\xA4\xB3\xCC"            // 电子工程
#define ZH_DANWEI  "\xB5\xA5\xCE\xBB\xBB\xBB\xCB\xE3"            // 单位换算
#define ZH_HSJZ    "\xBB\xF5\xB1\xD2\xCA\xB1\xBC\xE4\xBC\xDB\xD6\xB5" // 货币时间价值
#define ZH_QISHU   "\xC6\xDA\xCA\xFD"                            // 期数
#define ZH_NLILV   "\xC0\xFB\xC2\xCA"                            // 利率（每期，12C）
#define ZH_XIANZHI "\xCF\xD6\xD6\xB5"                            // 现值
#define ZH_FUKUAN  "\xB8\xB6\xBF\xEE"                            // 付款
#define ZH_ZHONGZHI "\xD6\xD5\xD6\xB5"                           // 终值
#define ZH_ZHIFU   "\xD6\xA7\xB8\xB6"                            // 支付
#define ZH_QIMMO   "\xC6\xDA\xC4\xA9"                            // 期末
#define ZH_QICHU   "\xC6\xDA\xB3\xF5"                            // 期初
#define ZH_WEISHU  "\xCE\xBB\xCA\xFD"                            // 位数
#define ZH_JSZHONG "\xBD\xA8\xC9\xE8\xD6\xD0"                    // 建设中
#define ZH_GONGCHENG "\xB9\xA4\xB3\xCC"                          // 工程
#define ZH_JISUAN  "\xBC\xC6\xCB\xE3"                            // 计算
#define ZH_MULU    "\xC4\xBF\xC2\xBC"                            // 目录
#define ZH_WUJIE   "\xCE\xDE\xBD\xE2"                            // 无解

// ---- 混排绘制（GBK 双字节中文 16px + ASCII 16px；返回新 x）----
static int fDrawMix(int x, int y, const char *s, uint8_t fg, int16_t bg) {
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

// ---- 层/模块/选择状态 ----
static int fcRun = 0;   // 运行标志（Shift+ON 清）
static int fcLevel = 0; // 0=L0 功能列表 1=L1 模块列表 2=L2 表单 3=公式视图 4=FIX 设置
static int fcMod = 1;   // 1=金融 2=电子工程 3=单位换算
static int fcSel = 0;   // L0/L1 高亮（绝对下标）
static int fcTop = 0;   // L1 滚动窗口顶（可视 5 行）
static int fcForm = 0;  // L2 表单 id（P1 仅 0=TVM 实现；其余占位）
static int fcFix = 2;   // 金融金额小数位（12C FIX，默认 2）
static int fcRowFoc = -1; // >=0：行级局部刷新（仅 TVM 聚焦行重画+flush；-1 全刷）
static int fcFixSel = 0; // FIX 设置页高亮
static int foc = 0;     // 表单聚焦字段（0-4）

// ---- 模块列表数据（名称 GBK + 缩写；P1 其余表单建设中）----
static const char *finItems[8] = {
    ZH_HSJZ, "\xCF\xD6\xBD\xF0\xC1\xF7", "\xCC\xAF\xCF\xFA", "\xD5\xAE\xC8\xAF",
    "\xD5\xDB\xBE\xC9", "\xC8\xD5\xC6\xDA", "\xC0\xFB\xC2\xCA\xBB\xBB\xCB\xE3", "\xC0\xFB\xC8\xF3" }; // 货币时间价值/现金流/摊销/债券/折旧/日期/利率换算/利润
static const char *finAbbr[8] = { "TVM", "CFLOW", "AMORT", "BOND", "DEPREC", "DATE", "ICONV", "MARGIN" };
static const char *eeItems[9] = {
    "\xC5\xB7\xC4\xB7\xB6\xA8\xC2\xC9", "\xB7\xD6\xD1\xB9\xC6\xF7", "\xB5\xE7\xD7\xE8\xB2\xA2\xC1\xAA",
    "\xCA\xB1\xBC\xE4\xB3\xA3\xCA\xFD", "\xD0\xB3\xD5\xF1\xC6\xB5\xC2\xCA", "\xC6\xB5\xC2\xCA\xD6\xDC\xC6\xDA",
    "\xD5\xFD\xCF\xD2\xB7\xF9\xD6\xB5", "dBm \xB9\xA6\xC2\xCA", "\xB1\xE4\xD1\xB9\xC6\xF7" }; // 欧姆定律/分压器/电阻并联/时间常数/谐振频率/频率周期/正弦幅值/dBm 功率/变压器
static const char *eeAbbr[9] = { "OHM", "VDIV", "RPAR", "RC", "RESO", "FREQ", "SINE", "DBM", "XFMR" };
static const char *unItems[10] = {
    "\xB3\xA4\xB6\xC8", "\xC3\xE6\xBB\xFD", "\xCC\xE5\xBB\xFD", "\xD6\xCA\xC1\xBF", "\xCE\xC2\xB6\xC8",
    "\xCB\xD9\xB6\xC8", "\xD1\xB9\xC1\xA6", "\xC4\xDC\xC1\xBF", "\xB9\xA6\xC2\xCA", "\xCA\xFD\xBE\xDD" }; // 长度/面积/体积/质量/温度/速度/压力/能量/功率/数据
static const char *unAbbr[10] = { "LEN", "AREA", "VOL", "MASS", "TEMP", "VEL", "PRES", "ENER", "POW", "DATA" };

static int finCount = 8, eeCount = 9, unCount = 10;

// ---- 输入缓冲（当前聚焦字段）----
static char ebuf[24];
static int elen = 0;
static int eAct = 0; // 编辑激活

// ---- 消息（标题右侧短暂显示）----
static char fcMsg[24] = {0};

// ---- 金融显示 FIX ----
static void fmtFin(double v, char *buf) {
    if (v != v) { strcpy(buf, "nan"); return; }
    if (v != v * 0) { /* inf */ if (v > 1e308) { strcpy(buf, "Inf"); return; } }
    if (v == 0) v = 0; // 去负零
    if (fcFix < 0) fcFix = 0;
    if (fcFix > 9) fcFix = 9;
    if (fabs(v) < 1e15) {
        char tmp[40];
        sprintf(tmp, "%.*f", fcFix, v);
        if ((int)strlen(tmp) <= 13) { strcpy(buf, tmp); return; }
    }
    sprintf(buf, "%.6e", v); // 超宽回退
    if (strlen(buf) > 13) buf[13] = 0;
}

// ---- 持久化 /formcalc.dat：magic(4) fix(1) bgn(1) has(5) tvm(5*8) ----
static double tvmV[5] = {0}; // N I% PV PMT FV
static unsigned char tvmHas[5] = {0};
static int tvmBgn = 0;       // 1=期初支付
static unsigned char saveBuf[64];
static FIL gfile;

static void fcSave(void) {
    memcpy(saveBuf, "FC01", 4);
    saveBuf[4] = (unsigned char)fcFix;
    saveBuf[5] = (unsigned char)tvmBgn;
    memcpy(saveBuf + 6, tvmHas, 5);
    memcpy(saveBuf + 11, tvmV, 5 * sizeof(double));
    if (f_open(&gfile, "/formcalc.dat", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        UINT bw;
        f_write(&gfile, saveBuf, 51, &bw);
        f_close(&gfile);
    }
}

static void fcLoad(void) {
    if (f_open(&gfile, "/formcalc.dat", FA_READ) == FR_OK) {
        UINT br = 0;
        f_read(&gfile, saveBuf, 51, &br);
        f_close(&gfile);
        if (br == 51 && memcmp(saveBuf, "FC01", 4) == 0) {
            fcFix = saveBuf[4];
            tvmBgn = saveBuf[5];
            memcpy(tvmHas, saveBuf + 6, 5);
            memcpy(tvmV, saveBuf + 11, 5 * sizeof(double));
            if (fcFix < 0 || fcFix > 9) fcFix = 2;
            if (tvmBgn != 0 && tvmBgn != 1) tvmBgn = 0;
        }
    }
}

// ---- 编辑 ----
static void fcCommit(void) { // 当前编辑落值到聚焦字段（仅 TVM 表单）
    if (fcLevel == 2 && fcForm == 0 && eAct && foc >= 0 && foc < 5) {
        tvmV[foc] = atof(ebuf);
        tvmHas[foc] = 1;
        fcSave();
    }
    eAct = 0;
    elen = 0;
    ebuf[0] = 0;
}

static int fcDigit(int key) {
    switch (key) {
        case KEY_0: return 0;
        case KEY_1: return 1;
        case KEY_2: return 2;
        case KEY_3: return 3;
        case KEY_4: return 4;
        case KEY_5: return 5;
        case KEY_6: return 6;
        case KEY_7: return 7;
        case KEY_8: return 8;
        case KEY_9: return 9;
        default: return -1;
    }
}

static void fcEditAppend(char c) {
    fcMsg[0] = 0;
    fcRowFoc = foc; // 行级刷新（输入仅聚焦行变化）
    if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; }
    if (elen < 20) {
        ebuf[elen++] = c;
        ebuf[elen] = 0;
    }
}

static void fcEditNeg(void) { // (-)：翻转编辑符号
    fcMsg[0] = 0;
    fcRowFoc = foc; // 行级刷新
    if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; }
    if (ebuf[0] == '-') { memmove(ebuf, ebuf + 1, elen); elen--; }
    else { memmove(ebuf + 1, ebuf, elen + 1); ebuf[0] = '-'; elen++; }
}

// ---- TVM 算法（12C 语义：n 期 i=每期利率；END 方程 PMT 项不乘 (1+i)）----
// ann(i) = ((1+i)^n - 1)/i（数值稳定；i=0 时 = n）
static double annF(double n, double i) {
    if (fabs(i) < 1e-12) return n;
    return expm1(n * log1p(i)) / i;
}

// f(i) = (1+i)^n*PV + PMT*ann*(1+i*b) + FV  （b=1 期初）
static double tvmF(double i, double n, double pv, double pmt, double fv, int b) {
    double x = pow(1.0 + i, n);
    double a = annF(n, i);
    if (b) a *= 1.0 + i;
    return x * pv + pmt * a + fv;
}

// 求解 i（二分扫描；pmt==0 时直接解）
static int solveI(double n, double pv, double pmt, double fv, int b, double *ri) {
    if (pmt == 0) { // PV(1+i)^n + FV = 0
        if (pv == 0 || fv == 0 || n == 0) return -1;
        double x = -fv / pv;
        if (x <= 0) return -1;
        *ri = pow(x, 1.0 / n) - 1.0;
        return (*ri > -1) ? 0 : -1;
    }
    // 通用：f(i) 变号扫描 + 二分（从 0 向两侧）
    double f0 = tvmF(0, n, pv, pmt, fv, b); // = PV+PMT*n+FV
    double lo, hi, flo, fhi;
    // 右支 (0, hi]：hi 从 1 起倍增
    lo = 0; flo = f0;
    hi = 1e-3;
    int found = 0;
    for (int k = 0; k < 60; k++) {
        fhi = tvmF(hi, n, pv, pmt, fv, b);
        if (fhi != fhi) { hi *= 4; continue; } // 溢出跳过
        if (fhi * flo <= 0) { found = 1; break; }
        hi *= 4;
        if (hi > 1e12) break;
    }
    if (!found) { // 左支 (-0.999999, 0)
        hi = 0; fhi = f0;
        lo = -1e-3;
        for (int k = 0; k < 60; k++) {
            flo = tvmF(lo, n, pv, pmt, fv, b);
            if (flo != flo) { lo *= 4; continue; }
            if (flo * fhi <= 0) { found = 1; break; }
            lo *= 4;
            if (lo < -0.999999) break;
        }
    }
    if (!found) return -1;
    for (int k = 0; k < 200; k++) { // 二分 200 次
        double mid = (lo + hi) * 0.5;
        double fm = tvmF(mid, n, pv, pmt, fv, b);
        if (fm * flo <= 0) { hi = mid; fhi = fm; }
        else { lo = mid; flo = fm; }
        if (fabs(hi - lo) < 1e-14 * (fabs(hi) + 1)) break;
    }
    *ri = (lo + hi) * 0.5;
    return 0;
}

// 解 TVM 未知量 target（0=N 1=I 2=PV 3=PMT 4=FV）；返回 0 成功
// 字段语义（12C）：I% 字段存百分数（9 = 9%/期），方程内部用比率 i=I%/100
static int tvmSolve(int target) {
    double n = tvmV[0], i = tvmV[1] / 100.0, pv = tvmV[2], pmt = tvmV[3], fv = tvmV[4];
    int b = tvmBgn;
    double r = 0;
    if (target != 0 && !tvmHas[0]) return -1; // N 未知时其他量缺
    if (target != 1 && !tvmHas[1]) return -1;
    if (target != 2 && !tvmHas[2]) return -1;
    if (target != 3 && !tvmHas[3]) return -1;
    if (target != 4 && !tvmHas[4]) return -1;
    switch (target) {
        case 0: { // N
            if (fabs(i) < 1e-12) {
                if (pmt == 0) return -1;
                r = -(pv + fv) / pmt;
            } else {
                double k = pmt * (1.0 + i * b) / i;
                double x = (k - fv) / (pv + k); // (1+i)^n
                if (x <= 0) return -1;
                r = log(x) / log1p(i);
            }
            break;
        }
        case 1: { // I%
            double ri;
            if (solveI(n, pv, pmt, fv, b, &ri) != 0) return -1;
            r = ri * 100.0; // I% = 每期利率*100（显示百分数）
            break;
        }
        case 2: { // PV
            double x = pow(1.0 + i, n);
            double a = annF(n, i);
            if (b) a *= 1.0 + i;
            r = -(pmt * a + fv) / x;
            break;
        }
        case 3: { // PMT
            double x = pow(1.0 + i, n);
            double a = annF(n, i);
            if (b) a *= 1.0 + i;
            if (fabs(a) < 1e-300) return -1;
            r = -(x * pv + fv) / a;
            break;
        }
        case 4: { // FV
            double x = pow(1.0 + i, n);
            double a = annF(n, i);
            if (b) a *= 1.0 + i;
            r = -(x * pv + pmt * a);
            break;
        }
    }
    if (r != r || r > 1e300 || r < -1e300) return -1;
    tvmV[target] = r;
    tvmHas[target] = 1;
    fcSave();
    return 0;
}

// ---- 通用绘制：标题 16px + 消息/状态右区 ----

static void drawTitle(const char *left, const char *right) {
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    fDrawMix(2, 0, left, 0, 255);
    if (fcMsg[0]) { // 短暂消息（覆盖右区）
        int l = (int)strlen(fcMsg);
        uidisp->draw_printf(254 - l * 8, 2, 12, 0, 255, "%s", fcMsg);
    } else if (right) {
        int l = (int)strlen(right);
        uidisp->draw_printf(254 - l * 8, 2, 12, 0, 255, "%s", right);
    }
}

static void drawMenu(const char *m[6]) {
    int i;
    uidisp->draw_box(0, 112, 255, 127, 255, 0);
    for (i = 1; i < 6; i++)
        uidisp->draw_line(i * 42, 114, i * 42, 126, 255);
    for (i = 0; i < 6; i++) {
        const char *t = m[i];
        if (!t || t[0] == 0) t = "_";
        uidisp->draw_printf(i * 42 + 2, 114, 12, 255, 0, "%s", t);
    }
}

static void fcListSel(void) { // 选择进入（L0/L1 通用：返回进入的模块列表项绝对下标）
    if (fcLevel == 0) {
        fcMod = fcSel + 1;
        fcSel = 0;
        fcTop = 0;
        fcLevel = 1;
        fcMsg[0] = 0;
        return;
    }
    // L1 → L2
    int cnt = fcMod == 1 ? finCount : (fcMod == 2 ? eeCount : unCount);
    int idx = fcTop + fcSel;
    if (idx >= cnt) { idx = cnt - 1; fcSel = cnt - 1 - fcTop; }
    fcForm = idx;
    fcLevel = 2;
    fcMsg[0] = 0;
    foc = 0;
    eAct = 0;
    elen = 0;
    ebuf[0] = 0;
}

static const char *fcItemName(int mod, int idx) {
    if (mod == 1) return finItems[idx];
    if (mod == 2) return eeItems[idx];
    return unItems[idx];
}
static const char *fcItemAbbr(int mod, int idx) {
    if (mod == 1) return finAbbr[idx];
    if (mod == 2) return eeAbbr[idx];
    return unAbbr[idx];
}
static int fcModCount(void) {
    return fcMod == 1 ? finCount : (fcMod == 2 ? eeCount : unCount);
}

// ---- 各层绘制 ----
static const char *l0Names[3] = { ZH_JINRONGQI, ZH_DIANZI, ZH_DANWEI };

static void drawL0(void) {
    const char *menus[6] = { "FIN", "EE", "UNIT", "_", "_", "FIX" };
    drawTitle("FormCalc " ZH_BIAODAN, NULL);
    for (int i = 0; i < 3; i++) {
        int y = 20 + i * 24;
        if (i == fcSel) { // 反显
            uidisp->draw_box(0, y - 2, 255, y + 15, 255, 0);
            fDrawMix(4, y, l0Names[i], 255, 0);
        } else {
            fDrawMix(4, y, l0Names[i], 0, 255);
        }
    }
    drawMenu(menus);
}

static void drawL1(void) {
    char right[20];
    int cnt = fcModCount();
    sprintf(right, "%d-%d/%d", fcTop + 1, fcTop + 5 > cnt ? cnt : fcTop + 5, cnt);
    const char *title = fcMod == 1 ? "\xBD\xF0\xC8\xDA FIN" : (fcMod == 2 ? "EE " ZH_DIANZI : "UNIT " ZH_DANWEI);
    drawTitle(title, right);
    for (int r = 0; r < 5; r++) {
        int idx = fcTop + r;
        if (idx >= cnt) break;
        int y = 18 + r * 18;
        int sel = (idx == fcTop + fcSel && fcSel < 5);
        if (sel) uidisp->draw_box(0, y - 1, 255, y + 15, 255, 0);
        // 序号（1-based，右对齐编号区）+ 中文名 + 缩写
        char pre[8];
        sprintf(pre, "%d ", idx + 1);
        int x = 4;
        uidisp->draw_printf(x, y, 12, sel ? 255 : 0, sel ? 0 : 255, "%s", pre);
        x += 4 * 8;
        fDrawMix(x, y, fcItemName(fcMod, idx), sel ? 255 : 0, sel ? 0 : 255);
        x = 150;
        uidisp->draw_printf(x, y, 12, sel ? 255 : 0, sel ? 0 : 255, "%s", fcItemAbbr(fcMod, idx));
    }
    // 菜单：留空（用户定：模块选择菜单不用 F 键快捷键）
    const char *m[6] = { "_", "_", "_", "_", "_", "_" };
    drawMenu(m);
}

static const char *tvmNames[5] = { ZH_QISHU, ZH_NLILV, ZH_XIANZHI, ZH_FUKUAN, ZH_ZHONGZHI };
static const char *tvmAbbr[5] = { "N", "i%", "PV", "PMT", "FV" };

static void drawTvmRow(int i) { // 单行绘制（行级局部刷新用：整行 x0=0 全宽）
    int y = 20 + i * 18;
    char tmp[40];
    if (i == foc) { // 聚焦整行反显
        uidisp->draw_box(0, y - 1, 255, y + 15, 255, 0);
        fDrawMix(4, y, tvmNames[i], 255, 0);
        uidisp->draw_printf(84, y, 12, 255, 0, "%s", tvmAbbr[i]);
        if (eAct) {
            uidisp->draw_printf(120, y, 16, 255, 0, "%s", ebuf);
        } else if (tvmHas[i]) {
            fmtFin(tvmV[i], tmp);
            uidisp->draw_printf(120, y, 16, 255, 0, "%s", tmp);
        } else {
            uidisp->draw_printf(120, y, 16, 255, 0, "--"); // 未填
        }
    } else {
        uidisp->draw_box(0, y - 1, 255, y + 15, 255, 255); // 清白底
        fDrawMix(4, y, tvmNames[i], 0, 255);
        uidisp->draw_printf(84, y, 12, 0, 255, "%s", tvmAbbr[i]);
        if (tvmHas[i]) {
            fmtFin(tvmV[i], tmp);
            uidisp->draw_printf(120, y, 16, 0, 255, "%s", tmp);
        }
    }
}

static void drawTVM(void) {
    char right[24];
    sprintf(right, "FIX%d %s", fcFix, tvmBgn ? ZH_QICHU : ZH_QIMMO);
    drawTitle("TVM " ZH_HSJZ, right);
    const char *menus[6] = { "N", "i%", "PV", "PMT", "FV", "B/E" };
    for (int i = 0; i < 5; i++) drawTvmRow(i);
    drawMenu(menus);
}

static void drawFormula(void) { // TVM 公式视图（只读，ASCII 文本）
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    fDrawMix(2, 0, "TVM " ZH_HSJZ " formula", 0, 255); // 公式
    const char *lines[] = {
        "END: (1+i)^n*PV + PMT*((1+i)^n-1)/i + FV = 0",
        "BGN: PMT term times (1+i)  (begin pay)",
        "n periods, i = I%/100 (per period)",
        "i=0 case:  PV + PMT*n + FV = 0",
        "pmt=0:     PV*(1+i)^n + FV = 0",
        "",
        "F1-F5: solve that unknown (others filled)",
        "F6: END <-> BGN toggle",
        "",
        "View/ON: back to form",
    };
    for (int i = 0; i < 10; i++)
        uidisp->draw_printf(2, 16 + i * 10, 8, 0, 255, "%s", lines[i]);
}

static void drawUnderConstruction(void) { // 建设中占位（P2 表单）
    char right[8];
    sprintf(right, "%s", fcItemAbbr(fcMod, fcForm));
    drawTitle(fcItemName(fcMod, fcForm), right);
    const char *menus[6] = { "_", "_", "_", "_", "_", "_" };
    fDrawMix(40, 50, "\xB4\xCB\xB9\xA6\xC4\xDC " ZH_JSZHONG, 0, 255); // 此功能 建设中（P2）
    uidisp->draw_printf(40, 74, 8, 0, 255, "(P2 release)");
    drawMenu(menus);
}

static void drawFixPage(void) {
    char line[32];
    drawTitle("\xCE\xBB\xCA\xFD FIX", "0-9 select");
    const char *menus[6] = { "_", "_", "_", "_", "_", "OK" };
    for (int i = 0; i < 10; i++) {
        int y = 20 + i * 9;
        int sel = (i == fcFixSel);
        if (sel) uidisp->draw_box(0, y - 1, 255, y + 7, 255, 0);
        sprintf(line, "%d - %d decimals%s", i, i, (i == fcFix) ? " *" : "");
        uidisp->draw_printf(8, y, 8, sel ? 255 : 0, sel ? 0 : 255, "%s", line);
    }
    drawMenu(menus);
}

// ---- 重绘 ----
static void fcDraw(void) {
    if (fcRowFoc >= 0 && fcLevel == 2 && fcMod == 1 && fcForm == 0) {
        int y = 20 + fcRowFoc * 18;
        drawTvmRow(fcRowFoc);
        uidisp->flushRect(0, y - 1, 255, y + 15); // 整行宽局部刷新（x0=0 才正确）
        fcRowFoc = -1;
        return;
    }
    fcRowFoc = -1;
    if (fcLevel == 3) drawFormula();
    else if (fcLevel == 2) {
        if (fcMod == 1 && fcForm == 0) drawTVM();
        else drawUnderConstruction();
    } else if (fcLevel == 4) drawFixPage();
    else if (fcLevel == 1) drawL1();
    else drawL0();
    uidisp->flush();
}

// ---- 按键处理（返回 1=重绘）----
static int fcHandleKey(int key) {
    if (fcLevel == 3) { // 公式视图：View/ON 返回表单；其余忽略（方向滚动无内容）
        if (key == KEY_VIEWS || key == KEY_ON) { fcLevel = 2; return 1; }
        return 0;
    }
    if (fcLevel == 2 && fcMod == 1 && fcForm == 0) {
        // ---- TVM 表单 ----
        int d = fcDigit(key);
        if (d >= 0) { fcEditAppend('0' + d); return 1; }
        if (key == KEY_DOT) { fcEditAppend('.'); return 1; }
        if (key == KEY_NEGATIVE) { fcEditNeg(); return 1; }
        if (key == KEY_BACKSPACE) {
            fcMsg[0] = 0;
            fcRowFoc = foc; // 行级刷新
            if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; return 1; }
            if (elen > 0) { elen--; ebuf[elen] = 0; if (elen == 0) { eAct = 0; } return 1; }
            return 1;
        }
        if (key == KEY_ENTER) { // 提交 → 下一字段（循环）
            fcMsg[0] = 0;
            fcCommit();
            foc = (foc + 1) % 5;
            return 1;
        }
        if (key == KEY_UP || key == KEY_DOWN) { // 切换字段（循环）
            fcMsg[0] = 0;
            fcCommit();
            if (key == KEY_UP) foc = (foc + 4) % 5;
            else foc = (foc + 1) % 5;
            return 1;
        }
        int slot = -1; // F1-F6 物理码非连续——显式映射（勿用范围判断）
        switch (key) {
            case KEY_F1: slot = 0; break;
            case KEY_F2: slot = 1; break;
            case KEY_F3: slot = 2; break;
            case KEY_F4: slot = 3; break;
            case KEY_F5: slot = 4; break;
            case KEY_F6: slot = 5; break;
            default: break;
        }
        if (slot >= 0) {
            fcCommit(); // 编辑值落字段（若正在编辑 foc）
            if (slot <= 4) { // 求解
                if (tvmSolve(slot) != 0) strcpy(fcMsg, "no sol");
                else fcMsg[0] = 0;
            } else { // B/E
                tvmBgn = !tvmBgn;
                fcMsg[0] = 0;
                fcSave();
            }
            return 1;
        }
        if (key == KEY_VIEWS) { fcLevel = 3; return 1; }
        return 0; // HOME/APPS/ON 等在通用段处理
    }
    if (fcLevel == 2) { // 建设中页：任意返回键退出
        if (key == KEY_ON || key == KEY_HOME || key == KEY_BACKSPACE || key == KEY_APPS) {
            fcLevel = 1;
            fcMsg[0] = 0;
            return 1;
        }
        return 0;
    }
    if (fcLevel == 4) { // FIX 页
        int d = fcDigit(key);
        if (d >= 0) { fcFix = d; fcFixSel = d; fcSave(); fcLevel = 0; return 1; }
        if (key == KEY_UP) { if (fcFixSel > 0) fcFixSel--; return 1; }
        if (key == KEY_DOWN) { if (fcFixSel < 9) fcFixSel++; return 1; }
        if (key == KEY_ENTER) { fcFix = fcFixSel; fcSave(); fcLevel = 0; return 1; }
        if (key == KEY_F6) { fcFix = fcFixSel; fcSave(); fcLevel = 0; return 1; } // F6=OK
        if (key == KEY_ON || key == KEY_HOME || key == KEY_BACKSPACE) { fcLevel = 0; return 1; }
        return 0;
    }
    if (fcLevel == 1) { // L1 模块列表
        int cnt = fcModCount();
        if (key == KEY_UP) { if (fcTop + fcSel > 0) { if (fcSel == 0 && fcTop > 0) fcTop--; else fcSel--; } return 1; }
        if (key == KEY_DOWN) { if (fcTop + fcSel < cnt - 1) { if (fcSel == 4 && fcTop + 4 < cnt - 1) fcTop++; else fcSel++; } return 1; }
        if (key == KEY_ENTER) { fcListSel(); return 1; }
        return 0;
    }
    // L0 功能列表
    if (key == KEY_UP) { if (fcSel > 0) fcSel--; return 1; }
    if (key == KEY_DOWN) { if (fcSel < 2) fcSel++; return 1; }
    if (key == KEY_ENTER) { fcListSel(); return 1; }
    if (key == KEY_F1) { fcSel = 0; fcListSel(); return 1; }
    if (key == KEY_F2) { fcSel = 1; fcListSel(); return 1; }
    if (key == KEY_F3) { fcSel = 2; fcListSel(); return 1; }
    if (key == KEY_F6) { fcLevel = 4; fcFixSel = fcFix; return 1; }
    return 0;
}

// ---- 任务 ----
static void formcalcTask(void *_) {
    SystemUISuspend();
    uidisp->restoreBuffer();
    fcRun = 1;
    fcLevel = 0;
    fcSel = 0;
    fcTop = 0;
    fcMod = 1;
    fcMsg[0] = 0;
    fcLoad();
    drawL0();
    uidisp->flush();
    int lastKey = -1;
    int shiftHeld = 0;
    while (fcRun) {
        uint32_t keys = ll_vm_check_key();
        uint32_t kp = keys >> 16;
        uint32_t key = keys & 0xFFFF;
        if (kp) {
            if (key == KEY_SHIFT) {
                if (key != (uint32_t)lastKey) {
                    if (shiftHeld) { shiftHeld = 0; ll_disp_set_indicator(0, -1); }
                    else { shiftHeld = 1; ll_disp_set_indicator(INDICATE_LEFT, -1); }
                    fcDraw();
                }
                lastKey = key;
            } else if (key != (uint32_t)lastKey) {
                lastKey = key;
                if (shiftHeld) {
                    // Shift 层：数字=FIX n；ON=退出；其余忽略
                    int d = fcDigit((int)key);
                    if (d >= 0) { fcFix = d; fcSave(); shiftHeld = 0; ll_disp_set_indicator(0, -1); fcDraw(); }
                    else if (key == KEY_ON) { fcRun = 0; shiftHeld = 0; ll_disp_set_indicator(0, -1); }
                    else { shiftHeld = 0; ll_disp_set_indicator(0, -1); fcDraw(); }
                } else if (key == KEY_ON) { // ON/C：逐层后退（公式视图→表单→L1→L0；根无操作）
                    if (fcLevel == 3) { fcLevel = 2; fcMsg[0] = 0; fcDraw(); } // 公式视图 ON=回表单
                    else if (fcLevel == 2) { fcLevel = 1; fcMsg[0] = 0; fcDraw(); }
                    else if (fcLevel == 1) { fcLevel = 0; fcMsg[0] = 0; fcDraw(); }
                    else if (fcLevel == 4) { fcLevel = 0; fcDraw(); }
                } else if (key == KEY_HOME) { // HOME：一步回 L0（L0 内无操作）
                    if (fcLevel != 0) { fcCommit(); fcLevel = 0; fcMsg[0] = 0; fcDraw(); }
                } else if (key == KEY_APPS) { // APPS：金融模块回退（表单→金融 L1→L0）
                    if (fcLevel >= 2 && fcMod == 1) { fcLevel = 1; fcMsg[0] = 0; fcDraw(); }
                    else if (fcLevel == 1 && fcMod == 1) { fcLevel = 0; fcDraw(); }
                } else if (key == KEY_VIEWS) { // View 键公式切换（表单态）
                    if (fcLevel == 2 && fcMod == 1 && fcForm == 0) { fcLevel = 3; fcDraw(); }
                } else {
                    if (fcHandleKey((int)key)) fcDraw();
                }
            }
        } else {
            if (lastKey != -1) lastKey = -1; // 键释放：清防重——同键再次按下可触发
            vTaskDelay(5); // 释放 CPU（省电：空闲让位给 IDLE 降频）
        }
    }
    fcCommit();
    fcSave();
    fcMsg[0] = 0;
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    uidisp->flush();
    SystemUIResume();
    vTaskDelete(NULL);
}

extern "C" void StartFormCalc() {
    xTaskCreate(formcalcTask, "FormCalc", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
}
