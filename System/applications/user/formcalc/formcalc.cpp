// FormCalc - 表单计算器（P1 框架+TVM / P2 金融全集 7 表单 / 后续工程+换算）
// 设计见 docs/FormCalc-design.md（12C 全集表单化，实现原创）
// 导航：L0 功能列表（HOME 回此/退出用 Shift+ON）→ L1 模块列表 → L2 计算表单（View=公式视图）
// 通用：Shift+数字=FIX n；Shift+BKSP=清空所有（两遍确认）
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
#define ZH_JINRONGQI "\xBD\xF0\xC8\xDA\xBC\xC6\xCB\xE3\xC6\xF7"  // 金融计算器
#define ZH_DIANZI  "\xB5\xE7\xD7\xD3\xB9\xA4\xB3\xCC"            // 电子工程
#define ZH_DANWEI  "\xB5\xA5\xCE\xBB\xBB\xBB\xCB\xE3"            // 单位换算
#define ZH_HSJZ    "\xBB\xF5\xB1\xD2\xCA\xB1\xBC\xE4\xBC\xDB\xD6\xB5" // 货币时间价值
#define ZH_QISHU   "\xC6\xDA\xCA\xFD"                            // 期数
#define ZH_LILV    "\xC0\xFB\xC2\xCA"                            // 利率（每期，12C）
#define ZH_XIANZHI "\xCF\xD6\xD6\xB5"                            // 现值
#define ZH_FUKUAN  "\xB8\xB6\xBF\xEE"                            // 付款
#define ZH_ZHONGZHI "\xD6\xD5\xD6\xB5"                           // 终值
#define ZH_QIMMO   "\xC6\xDA\xC4\xA9"                            // 期末
#define ZH_QICHU   "\xC6\xDA\xB3\xF5"                            // 期初
#define ZH_WEISHU  "\xCE\xBB\xCA\xFD"                            // 位数
#define ZH_JSZHONG "\xBD\xA8\xC9\xE8\xD6\xD0"                    // 建设中
#define ZH_BENJIN  "\xB1\xBE\xBD\xF0"                            // 本金
#define ZH_QISHIQI "\xC6\xF0\xCA\xBC\xC6\xDA"                    // 起始期
#define ZH_JSQI    "\xBD\xE1\xCA\xF8\xC6\xDA"                    // 结束期
#define ZH_JSRI    "\xBD\xE1\xCB\xE3\xC8\xD5"                    // 结算日
#define ZH_DQRI    "\xB5\xBD\xC6\xDA\xC8\xD5"                    // 到期日
#define ZH_PLX     "\xC6\xB1\xCF\xA2\xC2\xCA"                    // 票息率
#define ZH_SYL     "\xCA\xD5\xD2\xE6\xC2\xCA"                    // 收益率
#define ZH_MZ      "\xC3\xE6\xD6\xB5"                            // 面值
#define ZH_CB      "\xB3\xC9\xB1\xBE"                            // 成本
#define ZH_CZ      "\xB2\xD0\xD6\xB5"                            // 残值
#define ZH_SM      "\xCA\xDC\xC3\xFC"                            // 寿命
#define ZH_QJ      "\xC6\xDA\xBC\xE4"                            // 期间
#define ZH_ZJ      "\xD5\xDB\xBE\xC9"                            // 折旧
#define ZH_TIANSHU "\xCC\xEC\xCA\xFD"                            // 天数
#define ZH_MYLL    "\xC3\xFB\xD2\xE5\xC0\xFB\xC2\xCA"            // 名义利率
#define ZH_YXLL    "\xD3\xD0\xD0\xA7\xC0\xFB\xC2\xCA"            // 有效利率
#define ZH_FLQS    "\xB8\xB4\xC0\xFB\xC6\xDA\xCA\xFD"            // 复利期数
#define ZH_SHOUJIA "\xCA\xDB\xBC\xDB"                            // 售价
#define ZH_LRL     "\xC0\xFB\xC8\xF3\xC2\xCA"                    // 利润率
#define ZH_JIEGUO  "\xBD\xE1\xB9\xFB"                            // 结果
#define ZH_RIQI    "\xC8\xD5\xC6\xDA"                            // 日期
#define ZH_DAIKUAN "\xB4\xFB\xBF\xEE"                            // 贷款
#define ZH_QINGKONG "\xC7\xE5\xBF\xD5"                           // 清空
#define ZH_XIANJINLIU "\xCF\xD6\xBD\xF0\xC1\xF7"                  // 现金流

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

// ---- 层/模块/状态 ----
static int fcRun = 0;   // 运行标志（Shift+ON 清）
static int fcLevel = 0; // 0=L0 1=L1 2=L2 3=公式视图(TVM) 4=FIX
static int fcMod = 1;   // 1=金融 2=电子工程 3=单位换算
static int fcSel = 0;   // L0/L1 高亮
static int fcTop = 0;   // L1 滚动窗口顶
static int fcForm = 0;  // L2 表单 id（金融 0-7）
static int fcFix = 2;   // 小数位（12C FIX，默认 2）
static int fcFixSel = 0;
static int foc = 0;     // 表单聚焦字段
static int fcRowFoc = -1; // >=0 行级刷新
static const double bondFreq[4] = { 1, 2, 4, 12 }; // 付息频
static void drawFocRowOnly(void);
static int fcClrArm = 0;  // Shift+BKSP 清空确认臂

// ---- 表单数据（[模块][表单][字段]）----
#define FC_MAXF 16
#define FC_NMOD 3
#define FC_NFORM 10
static double fv_[FC_NMOD][FC_NFORM][FC_MAXF] = {{{0}}};
static unsigned char fh_[FC_NMOD][FC_NFORM][FC_MAXF] = {{{0}}};
static unsigned char fs_[FC_NMOD][FC_NFORM] = {{0}}; // 表单开关（TVM bgn / BOND freq 等）

// 字段类型：0 金额（FIX 显示） 1 日期（MM.DDYYYY） 2 整数（%.0f） 3 百分数（FIX 显示）
struct FcFld { const char *nm; const char *ab; int ft; };
// 表单定义（名称在 finItems/finAbbr；fields 描述）
struct FcFormDef { int nf; int rowBase, rowStep; const FcFld *f; };
static struct FcFormDef finFmts[8];
static struct FcFormDef eeFmts[9];

// ---- 模块列表数据（GBK + 缩写）----
static const char *finItems[8] = {
    ZH_HSJZ, "\xCF\xD6\xBD\xF0\xC1\xF7", "\xCC\xAF\xCF\xFA", "\xD5\xAE\xC8\xAF",
    "\xD5\xDB\xBE\xC9", "\xC8\xD5\xC6\xDA", "\xC0\xFB\xC2\xCA\xBB\xBB\xCB\xE3", "\xC0\xFB\xC8\xF3" };
static const char *finAbbr[8] = { "TVM", "CFLOW", "AMORT", "BOND", "DEPREC", "DATE", "ICONV", "MARGIN" };
static const char *eeItems[9] = {
    "\xC5\xB7\xC4\xB7\xB6\xA8\xC2\xC9", "\xB7\xD6\xD1\xB9\xC6\xF7", "\xB5\xE7\xD7\xE8\xB2\xA2\xC1\xAA",
    "\xCA\xB1\xBC\xE4\xB3\xA3\xCA\xFD", "\xD0\xB3\xD5\xF1\xC6\xB5\xC2\xCA", "\xC6\xB5\xC2\xCA\xD6\xDC\xC6\xDA",
    "\xD5\xFD\xCF\xD2\xB7\xF9\xD6\xB5", "dBm \xB9\xA6\xC2\xCA", "\xB1\xE4\xD1\xB9\xC6\xF7" };
static const char *eeAbbr[9] = { "OHM", "VDIV", "RPAR", "RC", "RESO", "FREQ", "SINE", "DBM", "XFMR" };
static const char *unItems[10] = {
    "\xB3\xA4\xB6\xC8", "\xC3\xE6\xBB\xFD", "\xCC\xE5\xBB\xFD", "\xD6\xCA\xC1\xBF", "\xCE\xC2\xB6\xC8",
    "\xCB\xD9\xB6\xC8", "\xD1\xB9\xC1\xA6", "\xC4\xDC\xC1\xBF", "\xB9\xA6\xC2\xCA", "\xCA\xFD\xBE\xDD" };
static const char *unAbbr[10] = { "LEN", "AREA", "VOL", "MASS", "TEMP", "VEL", "PRES", "ENER", "POW", "DATA" };
static int finCount = 8, eeCount = 9, unCount = 10;

// ---- 输入/消息 ----
static char ebuf[24];
static int elen = 0;
static int eAct = 0;
static char fcMsg[24] = {0};

// ---- FIX 显示 ----
static void fmtFin(double v, char *buf) {
    if (v != v) { strcpy(buf, "nan"); return; }
    if (v > 1e308) { strcpy(buf, "Inf"); return; }
    if (v < -1e308) { strcpy(buf, "-Inf"); return; }
    if (v == 0) v = 0;
    if (fcFix < 0) fcFix = 0;
    if (fcFix > 9) fcFix = 9;
    if (fabs(v) < 1e15) {
        char tmp[40];
        sprintf(tmp, "%.*f", fcFix, v);
        if ((int)strlen(tmp) <= 13) { strcpy(buf, tmp); return; }
    }
    sprintf(buf, "%.6e", v);
    if (strlen(buf) > 13) buf[13] = 0;
}

// ---- 日期工具（值 = DD.MMYYYY double：整数=日，小数前2=月，后4=年）----
static void parseDate(double v, int *m, int *d, int *y) {
    *d = (int)v;                  // 日
    double frac = v - *d;         // .MMYYYY
    *m = (int)(frac * 100.0 + 0.5);
    frac = frac * 100.0 - *m;
    *y = (int)(frac * 10000.0 + 0.5);
    if (*y < 100) *y += 2000;     // 容错短年
    if (*m < 1 || *m > 12) *m = 1;
    if (*d < 1 || *d > 31) *d = 1;
}
static void fmtDate(double v, char *buf) {
    int m, d, y;
    parseDate(v, &m, &d, &y);
    sprintf(buf, "%02d.%02d%04d", d, m, y);
}
// 儒略日（实际日基）
static long jdn(int y, int m, int d) {
    int a = (14 - m) / 12, yy = y + 4800 - a, mm = m + 12 * a - 3;
    return (long)d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
}
static void jdnYmd(long j, int *y, int *m, int *d) {
    long a = j + 32044, b = (4 * a + 3) / 146097, c = a - 146097 * b / 4;
    long dd = (4 * c + 3) / 1461, e = c - 1461 * dd / 4, mm2 = (5 * e + 2) / 153;
    *d = (int)(e - (153 * mm2 + 2) / 5 + 1);
    *m = (int)(mm2 + 3 - 12 * (mm2 / 10));
    *y = (int)(100 * b + dd - 4800 + mm2 / 10);
}
static long dateJdn(double v) {
    int m, d, y;
    parseDate(v, &m, &d, &y);
    return jdn(y, m, d);
}
// 简化 30/360（日不做 31 调整）
static long days360(double a, double b) {
    int m1, d1, y1, m2, d2, y2;
    parseDate(a, &m1, &d1, &y1);
    parseDate(b, &m2, &d2, &y2);
    return 360L * (y2 - y1) + 30L * (m2 - m1) + (d2 - d1);
}
static long daysAct(double a, double b) { return dateJdn(b) - dateJdn(a); }

// ---- 持久化 /formcalc/formcalc.dat（FC03）：金融 8 表 + 工程 9 表
// FC03 = magic4 + fix1 + fs17 + has(17*2) + val(17*16*8) = 4+1+17+34+2176 = 2232
// FC02 = 金融 8 表（1053B）——兼容读取（工程数据保持空）
static unsigned char saveBuf[2300];
static FIL gfile;

static void fcSave(void) {
    unsigned char *p = saveBuf;
    memcpy(p, "FC03", 4); p += 4;
    *p++ = (unsigned char)fcFix;
    memcpy(p, fs_[1], 8); p += 8;      // 金融 8 表单开关
    memcpy(p, fs_[2], 9); p += 9;      // 工程 9 表单开关
    for (int m = 1; m <= 2; m++) {
        int n = (m == 1) ? 8 : 9;
        for (int f = 0; f < n; f++) {
            unsigned short msk = 0;
            for (int i = 0; i < 16; i++) if (fh_[m][f][i]) msk |= (1 << i);
            *p++ = (unsigned char)msk;
            *p++ = (unsigned char)(msk >> 8);
        }
    }
    for (int m = 1; m <= 2; m++) {
        int n = (m == 1) ? 8 : 9;
        for (int f = 0; f < n; f++)
            for (int i = 0; i < 16; i++) {
                double d = fv_[m][f][i];
                memcpy(p, &d, 8); p += 8;
            }
    }
    if (f_open(&gfile, "/formcalc/formcalc.dat", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        UINT bw;
        f_write(&gfile, saveBuf, p - saveBuf, &bw);
        f_close(&gfile);
    }
}

static void fcLoad(void) {
    f_mkdir("/formcalc"); // session 子目录（2026-09-06：程序文件分类存放；已存在返回 FR_EXIST 忽略）
    if (f_open(&gfile, "/formcalc/formcalc.dat", FA_READ) != FR_OK) { // 新路径
        if (f_open(&gfile, "/formcalc.dat", FA_READ) != FR_OK) return; // 旧路径兼容（自动迁移：下次保存写新路径）
    }
    UINT br = 0;
    f_read(&gfile, saveBuf, sizeof(saveBuf), &br);
    f_close(&gfile);
    unsigned char *p = saveBuf;
    if (br < 4) return;
    int ver = -1;
    if (memcmp(p, "FC03", 4) == 0 && br >= 2232) ver = 3;
    else if (memcmp(p, "FC02", 4) == 0 && br >= 1053) ver = 2;
    if (ver < 0) return;
    p += 4;
    fcFix = *p++;
    if (fcFix > 9) fcFix = 2;
    int mcnt = (ver == 3) ? 2 : 1; // FC02 无工程数据
    int nf[3] = {0, 8, 9};
    memcpy(fs_[1], p, 8); p += 8;
    if (ver == 3) { memcpy(fs_[2], p, 9); p += 9; }
    for (int m = 1; m <= mcnt; m++) {
        for (int f = 0; f < nf[m]; f++) {
            unsigned short msk = *p | (unsigned short)(p[1] << 8); p += 2;
            for (int i = 0; i < 16; i++) fh_[m][f][i] = (msk >> i) & 1;
        }
    }
    for (int m = 1; m <= mcnt; m++) {
        for (int f = 0; f < nf[m]; f++)
            for (int i = 0; i < 16; i++) {
                double d;
                memcpy(&d, p, 8); p += 8;
                fv_[m][f][i] = d;
            }
    }
}

// ---- 清空所有（Shift+BKSP 两遍确认）----
static void fcClearAll(void) {
    memset(fv_, 0, sizeof(fv_));
    memset(fh_, 0, sizeof(fh_));
    memset(fs_, 0, sizeof(fs_));
    eAct = 0; elen = 0; ebuf[0] = 0;
    fcSave();
}

// ---- 编辑 ----
static void fcCommit(void) { // 当前编辑落值到聚焦字段（L2 表单内）
    if (fcLevel == 2 && eAct && foc >= 0 && foc < FC_MAXF) {
        if (fcMod == 1 && fcForm == 1) { // CFLOW：foc 0=r 1-13=CF0-12
            if (foc < 14) {
                fv_[1][1][foc] = atof(ebuf);
                fh_[1][1][foc] = 1;
                fcSave();
            }
        } else if (fcMod == 1 && fcForm < 8 && foc < finFmts[fcForm].nf) {
            fv_[1][fcForm][foc] = atof(ebuf);
            fh_[1][fcForm][foc] = 1;
            fcSave();
        } else if (fcMod == 2 && fcForm < 9 && foc < eeFmts[fcForm].nf) {
            fv_[2][fcForm][foc] = atof(ebuf);
            fh_[2][fcForm][foc] = 1;
            fcSave();
        }
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

static void fcStatusRight(char *buf) { // 标题右侧状态文本（输入行级刷新时局部重绘用）
    buf[0] = 0;
    if (fcLevel != 2) return;
    if (fcMod == 1) {
        if (fcForm == 0) sprintf(buf, "FIX%d %s", fcFix, (fs_[1][0] & 1) ? ZH_QICHU : ZH_QIMMO);
        else if (fcForm == 3) sprintf(buf, "FIX%d frq%d", fcFix, (int)bondFreq[fs_[1][3] & 3]);
        else if (fcForm == 5) strcpy(buf, (fs_[1][5] & 1) ? "ACT" : "360");
        else sprintf(buf, "FIX%d", fcFix);
    }
}

static void fcTitleRightRedraw(void) { // 标题行右侧消息/状态文本区局部重绘（行级输入时不整刷标题）
    uidisp->draw_box(150, 0, 255, 17, 255, 255);
    if (fcMsg[0]) {
        // 消息统一 ASCII（12px）；防 GBK 误入 ascii 渲染成雪花——长度自适应起点
        int l = (int)strlen(fcMsg);
        uidisp->draw_printf(254 - l * 8, 2, 12, 0, 255, "%s", fcMsg);
    } else {
        char rb[24];
        fcStatusRight(rb);
        if (rb[0]) fDrawMix(150, 1, rb, 0, 255); // 状态可含 GBK（期初/期末）——16px 混排
    }
    uidisp->flushRect(150, 0, 255, 17);
}

static void fcEditAppend(char c) {
    fcMsg[0] = 0;
    fcRowFoc = foc;
    if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; }
    if (elen < 20) {
        ebuf[elen++] = c;
        ebuf[elen] = 0;
    }
    drawFocRowOnly();
    fcTitleRightRedraw();
    fcRowFoc = -1;
}

static void fcEditNeg(void) {
    fcMsg[0] = 0;
    fcRowFoc = foc;
    if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; }
    if (ebuf[0] == '-') { memmove(ebuf, ebuf + 1, elen); elen--; }
    else { memmove(ebuf + 1, ebuf, elen + 1); ebuf[0] = '-'; elen++; }
    drawFocRowOnly();
    fcTitleRightRedraw();
    fcRowFoc = -1;
}

// ---- 显示字段值（按类型）----
static void fmtVal(int ft, double v, char *buf) {
    if (ft == 1) { fmtDate(v, buf); return; }
    if (ft == 2) { sprintf(buf, "%.0f", v); return; }
    if (ft == 4) { // 工程通用（8 位有效——不用 FIX）
        if (v != v) { strcpy(buf, "nan"); return; }
        double a = v < 0 ? -v : v;
        if (a != 0 && (a >= 1e12 || a < 1e-6)) sprintf(buf, "%.6e", v);
        else sprintf(buf, "%.8g", v);
        return;
    }
    fmtFin(v, buf);
}

// ---- 通用绘制 ----
static void drawTitle(const char *left, const char *right) {
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    fDrawMix(2, 0, left, 0, 255);
    if (fcMsg[0]) {
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

// ---- CFLOW 滚动 ----
static int cfTop = 0; // CF 窗口顶（CF0..12 可视 4 行，行号 1-13）

static int cfRowY(int idx) { // 聚焦绝对下标 → 行 y
    if (idx == 0) return 18;
    return 34 + (idx - 1 - cfTop) * 16;
}

// ---- 金融表单绘制 ----
// 标准字段行（除 CFLOW 特殊）
static void fcFormDef(int form, int *nf, int *rb, int *rs, const FcFld **f); // 前置（定义见下）
static void drawStdRow(int form, int i) {
    int base, step, nf;
    const FcFld *ff;
    fcFormDef(form, &nf, &base, &step, &ff);
    if (i >= nf) return;
    int y = base + i * step;
    char tmp[40];
    const FcFld *f = &ff[i];
    int vx = 136; // 值列起点：abbr 最长 5 字符（x84+40=124）不与其重叠
    int has = fh_[fcMod][form][i];
    double val = fv_[fcMod][form][i];
    if (i == foc) {
        uidisp->draw_box(0, y - 1, 255, y + 15, 255, 0);
        fDrawMix(4, y, f->nm, 255, 0);
        uidisp->draw_printf(84, y, 12, 255, 0, "%s", f->ab);
        if (eAct) uidisp->draw_printf(vx, y, 16, 255, 0, "%s", ebuf);
        else if (has) { fmtVal(f->ft, val, tmp); uidisp->draw_printf(vx, y, 16, 255, 0, "%s", tmp); }
        else uidisp->draw_printf(vx, y, 16, 255, 0, "--");
    } else {
        uidisp->draw_box(0, y - 1, 255, y + 15, 255, 255);
        fDrawMix(4, y, f->nm, 0, 255);
        uidisp->draw_printf(84, y, 12, 0, 255, "%s", f->ab);
        if (has) { fmtVal(f->ft, val, tmp); uidisp->draw_printf(vx, y, 16, 0, 255, "%s", tmp); }
    }
}

static void drawCflowRow(int idx) { // idx 0=r 1-13=CF0-12
    int y = cfRowY(idx);
    char tmp[40];
    const char *nm;
    char ab[16]; // CF13 最长 4 字符——扩缓冲防格式告警
    if (idx == 0) {
        nm = ZH_LILV;             // 名：利率（中文前，与标准表单一致）
        strcpy(ab, "r%");         // 缩写位：r%（x84）
    } else {
        nm = ZH_XIANJINLIU;
        sprintf(ab, "CF%d", idx - 1);
    }
    int vx = 136;
    int isFoc = (idx == foc);
    if (isFoc) {
        uidisp->draw_box(0, y - 1, 255, y + 15, 255, 0);
        fDrawMix(4, y, nm, 255, 0);
        uidisp->draw_printf(84, y, 12, 255, 0, "%s", ab);
        if (eAct) uidisp->draw_printf(vx, y, 16, 255, 0, "%s", ebuf);
        else if (fh_[1][1][idx]) { fmtFin(fv_[1][1][idx], tmp); uidisp->draw_printf(vx, y, 16, 255, 0, "%s", tmp); }
        else uidisp->draw_printf(vx, y, 16, 255, 0, "--");
    } else {
        uidisp->draw_box(0, y - 1, 255, y + 15, 255, 255);
        fDrawMix(4, y, nm, 0, 255);
        uidisp->draw_printf(84, y, 12, 0, 255, "%s", ab);
        if (fh_[1][1][idx]) { fmtFin(fv_[1][1][idx], tmp); uidisp->draw_printf(vx, y, 16, 0, 255, "%s", tmp); }
        else uidisp->draw_printf(vx, y, 16, 0, 255, "--");
    }
}

static void drawCflow(void) {
    char tt[48];
    sprintf(tt, "CFLOW %s", finItems[1]);
    drawTitle(tt, NULL);
    drawCflowRow(0);
    for (int r = 0; r < 4; r++) {
        int idx = 1 + cfTop + r;
        if (idx <= 13) drawCflowRow(idx);
    }
    const char *menus[6] = { "NPV", "IRR", "_", "_", "CLR", "_" };
    drawMenu(menus);
}

// ---- 各表单 F1-F6 动作（act）----

// TVM（form 0）：槽 0-4 解未知量；槽 5 B/E
// ann(i) = ((1+i)^n - 1)/i（数值稳定；i=0 = n）
static double annF(double n, double i) {
    if (fabs(i) < 1e-12) return n;
    return expm1(n * log1p(i)) / i;
}
static double tvmF(double i, double n, double pv, double pmt, double fv, int b) {
    double x = pow(1.0 + i, n);
    double a = annF(n, i);
    if (b) a *= 1.0 + i;
    return x * pv + pmt * a + fv;
}
static int solveI(double n, double pv, double pmt, double fv, int b, double *ri) {
    if (pmt == 0) {
        if (pv == 0 || fv == 0 || n == 0) return -1;
        double x = -fv / pv;
        if (x <= 0) return -1;
        *ri = pow(x, 1.0 / n) - 1.0;
        return (*ri > -1) ? 0 : -1;
    }
    double f0 = tvmF(0, n, pv, pmt, fv, b);
    double lo, hi, flo, fhi;
    lo = 0; flo = f0;
    hi = 1e-3;
    int found = 0;
    for (int k = 0; k < 60; k++) {
        fhi = tvmF(hi, n, pv, pmt, fv, b);
        if (fhi != fhi) { hi *= 4; continue; }
        if (fhi * flo <= 0) { found = 1; break; }
        hi *= 4;
        if (hi > 1e12) break;
    }
    if (!found) {
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
    for (int k = 0; k < 200; k++) {
        double mid = (lo + hi) * 0.5;
        double fm = tvmF(mid, n, pv, pmt, fv, b);
        if (fm * flo <= 0) { hi = mid; fhi = fm; }
        else { lo = mid; flo = fm; }
        if (fabs(hi - lo) < 1e-14 * (fabs(hi) + 1)) break;
    }
    *ri = (lo + hi) * 0.5;
    return 0;
}
static int actTvm(int slot) {
    if (slot == 5) { // B/E
        fs_[1][0] ^= 1;
        fcSave();
        return 0;
    }
    if (slot < 0 || slot > 4) return 1;
    double *v = fv_[1][0];
    unsigned char *h = fh_[1][0];
    // 12C 语义：寄存器总有值——未填字段当 0 参与计算（FV 留空即可解 PMT）
    double n = v[0], i = v[1] / 100.0, pv = v[2], pmt = v[3], fv2 = v[4];
    int b = fs_[1][0] & 1;
    double r = 0;
    switch (slot) {
        case 0:
            if (fabs(i) < 1e-12) {
                if (pmt == 0) return 1;
                r = -(pv + fv2) / pmt;
            } else {
                double k2 = pmt * (1.0 + i * b) / i;
                double x = (k2 - fv2) / (pv + k2);
                if (x <= 0) return 1;
                r = log(x) / log1p(i);
            }
            break;
        case 1: {
            double ri;
            if (solveI(n, pv, pmt, fv2, b, &ri) != 0) return 1;
            r = ri * 100.0;
            break;
        }
        case 2: {
            double x = pow(1.0 + i, n), a = annF(n, i);
            if (b) a *= 1.0 + i;
            r = -(pmt * a + fv2) / x;
            break;
        }
        case 3: {
            double x = pow(1.0 + i, n), a = annF(n, i);
            if (b) a *= 1.0 + i;
            if (fabs(a) < 1e-300) return 1;
            r = -(x * pv + fv2) / a;
            break;
        }
        case 4: {
            double x = pow(1.0 + i, n), a = annF(n, i);
            if (b) a *= 1.0 + i;
            r = -(x * pv + pmt * a);
            break;
        }
    }
    if (r != r || r > 1e300 || r < -1e300) return 1;
    v[slot] = r;
    h[slot] = 1;
    fcSave();
    return 0;
}

// CFLOW（form 1）：槽 0=NPV（用 r% + CF0-12） 1=IRR 4=CLR 现金流
static double npvAt(double r) { // r 比率
    double s = fv_[1][1][1]; // CF0
    double den = 1 + r;
    double dk = den;
    for (int k = 2; k < 14; k++) {
        if (fh_[1][1][k]) s += fv_[1][1][k] / dk;
        dk *= den;
    }
    return s;
}
static int actCflow(int slot) {
    if (slot == 4) { // CLR 现金流（含 r）
        for (int i = 0; i < 14; i++) { fv_[1][1][i] = 0; fh_[1][1][i] = 0; }
        fcSave();
        strcpy(fcMsg, "Cleared");
        return 0;
    }
    if (!fh_[1][1][0]) return 1; // r 缺
    int cnt = 0;
    for (int i = 1; i < 14; i++) if (fh_[1][1][i]) cnt++;
    if (cnt == 0) return 1;
    if (slot == 0) { // NPV
        double r = fv_[1][1][0] / 100.0;
        double s = npvAt(r);
        if (s != s) return 1;
        sprintf(fcMsg, "NPV=%.*f", fcFix, s);
        return 0;
    }
    if (slot == 1) { // IRR：NPV(r)=0 二分（r 百分数语义同 TVM i 求解）
        double f0 = npvAt(0);
        double lo = 0, flo = f0, hi = 1e-3, fhi;
        int found = 0;
        for (int k = 0; k < 60; k++) {
            fhi = npvAt(hi);
            if (fhi != fhi) { hi *= 4; continue; }
            if (fhi * flo <= 0) { found = 1; break; }
            hi *= 4;
            if (hi > 1e12) break;
        }
        if (!found) {
            hi = 0; fhi = f0; lo = -1e-3;
            for (int k = 0; k < 60; k++) {
                flo = npvAt(lo);
                if (flo != flo) { lo *= 4; continue; }
                if (flo * fhi <= 0) { found = 1; break; }
                lo *= 4;
                if (lo < -0.999999) break;
            }
        }
        if (!found) return 1;
        for (int k = 0; k < 200; k++) {
            double mid = (lo + hi) * 0.5, fm = npvAt(mid);
            if (fm * flo <= 0) { hi = mid; }
            else { lo = mid; flo = fm; }
            if (fabs(hi - lo) < 1e-14 * (fabs(hi) + 1)) break;
        }
        double ri = (lo + hi) * 0.5 * 100.0;
        if (ri != ri) return 1;
        sprintf(fcMsg, "IRR=%.*f%%", fcFix, ri);
        return 0;
    }
    return 0;
}

// AMORT（form 2）END 支付：字段 贷款PV/利率i%/期数N/起始期P1/结束期P2/结果RES
// 槽 0=区间利息 1=区间本金 2=P2 期末余额
static int actAmort(int slot) {
    // 12C 语义（finanx-12c 核对）：INT = |bal|*i 按当前 FIX 位舍入、符号随 PMT；
    // PRN = PMT - INT；余额逐期更新收敛（贷款场景 INT/PRN 与 PMT 同负号）
    double *v = fv_[1][2];
    unsigned char *h = fh_[1][2];
    if (!h[0] || !h[1] || !h[2] || !h[3]) return 1;
    double pv = v[0], ip = v[1], n = v[2];
    long p1 = (long)v[3], p2 = h[4] ? (long)v[4] : p1;
    if (p1 < 1) p1 = 1;
    if (p2 < p1) p2 = p1;
    long nn = (long)(n + 0.5);
    if (nn < 1) return 1;
    if (p2 > nn) p2 = nn;
    double i = ip / 100.0;
    double pmt;
    if (fabs(i) < 1e-12) pmt = -pv / nn;
    else {
        double x = pow(1.0 + i, nn);
        pmt = -pv * i * x / (x - 1.0);
    }
    if (pmt == 0 || pmt != pmt) return 1;
    double sgn = (pmt < 0) ? -1.0 : 1.0;
    double pp = 1.0;
    for (int k = 0; k < fcFix; k++) pp *= 10.0;
    double bal = pv, intSum = 0, prinSum = 0;
    for (long k = 1; k <= p2; k++) {
        double ik = floor(fabs(bal) * i * pp + 0.5) / pp; // 12C 内部按显示位舍入利息
        if (sgn < 0) ik = -ik;
        double pk = pmt - ik;
        if (k >= p1) { intSum += ik; prinSum += pk; }
        bal += pk;
    }
    double res;
    if (slot == 0) res = intSum;
    else if (slot == 1) res = prinSum;
    else if (slot == 2) res = bal;
    else return 0;
    v[5] = res;
    h[5] = 1;
    fcSave();
    return 0;
}

// BOND（form 3）：字段 结算/到期/票息%/面值/价格/收益%；fs bit0-1=付息频(1<<(2*f))  日期同日号假设

static double bondPrice(double yld, int Nc, double c, double rv, double y) {
    // y 比率；现值：票息现值年金 + 面值折现
    if (Nc <= 0) return rv;
    double pv;
    if (fabs(y) < 1e-12) pv = Nc; // y=0 退化
    else pv = (1.0 - pow(1.0 + y, -Nc)) / y;
    return c * pv + rv * pow(1.0 + y, -Nc);
}
static int bondCalcNc(int *Nc) {
    int m1, d1, y1, m2, d2, y2;
    parseDate(fv_[1][3][0], &m1, &d1, &y1);
    parseDate(fv_[1][3][1], &m2, &d2, &y2);
    double f = bondFreq[fs_[1][3] & 3];
    long months = 12L * (y2 - y1) + (m2 - m1);
    if (months <= 0 || d1 != d2) return -1;   // 需同日号
    if (months > 1200) return -1;             // 期限超 100 年视为输入错误（防 D.MYYYYY 误读）
    *Nc = (int)(months * f / 12.0 + 0.5);
    if (*Nc <= 0) return -1;
    return 0;
}
static int actBond(int slot) {
    double *v = fv_[1][3];
    unsigned char *h = fh_[1][3];
    if (slot == 5) { // FRQ 付息频循环 1/2/4/12
        fs_[1][3] = (unsigned char)(((fs_[1][3] & 3) + 1) & 3);
        fcSave();
        return 0;
    }
    if (!h[0] || !h[1] || !h[2] || !h[3]) return 1;
    int Nc;
    if (bondCalcNc(&Nc) != 0) return 1;
    double f = bondFreq[fs_[1][3] & 3];
    double rv = v[3];
    double c = v[2] / 100.0 * rv / f;      // 每期票息
    if (slot == 0) { // 解价格（收益已填）
        if (!h[4]) return 1;
        double y = v[4] / 100.0 / f;
        double pr = bondPrice(0, Nc, c, rv, y);
        v[5] = pr;
        h[5] = 1;
        fcSave();
        return 0;
    }
    if (slot == 1) { // 解收益（价格已填）
        if (!h[5]) return 1;
        double target = v[5];
        double lo = 0, hi = 1e-4;
        double flo = bondPrice(0, Nc, c, rv, 0) - target;
        int found = 0;
        for (int k = 0; k < 60; k++) {
            double fhi = bondPrice(0, Nc, c, rv, hi) - target;
            if (fhi != fhi) { hi *= 4; continue; }
            if (fhi * flo <= 0) { found = 1; break; }
            hi *= 4;
            if (hi > 1e8) break;
        }
        if (!found) return 1;
        for (int k = 0; k < 200; k++) {
            double mid = (lo + hi) * 0.5;
            double fm = bondPrice(0, Nc, c, rv, mid) - target;
            if (fm * flo <= 0) hi = mid;
            else { lo = mid; flo = fm; }
            if (fabs(hi - lo) < 1e-15 * (fabs(hi) + 1)) break;
        }
        double y = (lo + hi) * 0.5 * f * 100.0;
        if (y != y || y > 1e6) return 1;
        v[4] = y;
        h[4] = 1;
        fcSave();
        return 0;
    }
    return 0;
}

// DEPREC（form 4）：成本/残值/寿命/期间/结果；槽 0=SL 1=DB200 2=SYD
static int actDeprec(int slot) {
    double *v = fv_[1][4];
    unsigned char *h = fh_[1][4];
    if (!h[0] || !h[1] || !h[2] || !h[3]) return 1;
    double c = v[0], s = v[1], l = v[2];
    long per = (long)v[3];
    if (l <= 0 || per < 1 || per > (long)(l + 0.5)) return 1;
    double dep = 0;
    if (slot == 0) { // 直线
        dep = (c - s) / l;
    } else if (slot == 1) { // 双倍余额递减（200%）
        double rate = 2.0 / l;
        double bal = c;
        for (long k = 1; k < per; k++) {
            double dk = bal * rate;
            if (bal - dk < s) dk = bal - s;
            bal -= dk;
        }
        dep = bal * rate;
        if (bal - dep < s) dep = bal - s;
        if (dep < 0) dep = 0;
    } else if (slot == 2) { // 年数总和
        dep = (c - s) * (l - per + 1) * 2.0 / (l * (l + 1.0));
    } else return 0;
    v[4] = dep;
    h[4] = 1;
    fcSave();
    return 0;
}

// DATE（form 5）：日期1/日期2/天数；fs bit0 日基(0=360 1=ACT)；槽 0=ΔD 1=+D 5=日基
static int actDate(int slot) {
    double *v = fv_[1][5];
    unsigned char *h = fh_[1][5];
    if (slot == 5) {
        fs_[1][5] ^= 1;
        fcSave();
        return 0;
    }
    if (!h[0]) return 1;
    if (slot == 0) { // 天数：日期1→日期2
        if (!h[1]) return 1;
        long d = (fs_[1][5] & 1) ? daysAct(v[0], v[1]) : days360(v[0], v[1]);
        v[2] = d;
        h[2] = 1;
        fcSave();
        return 0;
    }
    if (slot == 1) { // 日期2 = 日期1 + 天数（实际日历加）
        if (!h[2]) return 1;
        long j = dateJdn(v[0]) + (long)v[2];
        int y, m, d;
        jdnYmd(j, &y, &m, &d);
        v[1] = d + m / 100.0 + y / 1000000.0; // 组装 DD.MMYYYY（15.012026 = 15日01月2026）
        h[1] = 1;
        fcSave();
        return 0;
    }
    return 0;
}

// ICONV（form 6）：名义%/有效%/复利期数；菜单键=所求目标（12C 语义）
// F1(NOM)=求名义利率（需有效%已填）；F2(EFF)=求有效利率（需名义%已填）
static int actIconv(int slot) {
    double *v = fv_[1][6];
    unsigned char *h = fh_[1][6];
    if (slot == 0) { // 求 NOM
        if (!h[1] || !h[2] || v[2] < 1) return 1;
        double cy = v[2];
        v[0] = (pow(1.0 + v[1] / 100.0, 1.0 / cy) - 1.0) * cy * 100.0;
        h[0] = 1;
        fcSave();
        return 0;
    }
    if (slot == 1) { // 求 EFF
        if (!h[0] || !h[2] || v[2] < 1) return 1;
        double cy = v[2];
        v[1] = (pow(1.0 + v[0] / 100.0 / cy, cy) - 1.0) * 100.0;
        h[1] = 1;
        fcSave();
        return 0;
    }
    return 0;
}

// MARGIN（form 7）：成本/售价/利润率；槽 0=售价 1=成本 2=利润率
static int actMargin(int slot) {
    double *v = fv_[1][7];
    unsigned char *h = fh_[1][7];
    if (slot == 0) {
        if (!h[0] || !h[2] || v[2] >= 100) return 1;
        v[1] = v[0] / (1.0 - v[2] / 100.0);
        h[1] = 1;
    } else if (slot == 1) {
        if (!h[1] || !h[2]) return 1;
        v[0] = v[1] * (1.0 - v[2] / 100.0);
        h[0] = 1;
    } else if (slot == 2) {
        if (!h[0] || !h[1] || v[1] == 0) return 1;
        v[2] = (v[1] - v[0]) / v[1] * 100.0;
        h[2] = 1;
    } else return 0;
    fcSave();
    return 0;
}

// ---- 字段定义与菜单 ----
static const FcFld tvmFlds[5] = {
    { ZH_QISHU, "N", 2 }, { ZH_LILV, "i%", 0 }, { ZH_XIANZHI, "PV", 0 },
    { ZH_FUKUAN, "PMT", 0 }, { ZH_ZHONGZHI, "FV", 0 } };
static const FcFld amortFlds[6] = {
    { ZH_DAIKUAN, "PV", 0 }, { ZH_LILV, "i%", 0 }, { ZH_QISHU, "N", 2 },
    { ZH_QISHIQI, "P1", 2 }, { ZH_JSQI, "P2", 2 }, { ZH_JIEGUO, "RES", 0 } };
static const FcFld bondFlds[6] = {
    { ZH_JSRI, "D1", 1 }, { ZH_DQRI, "D2", 1 }, { ZH_PLX, "CPN%", 0 },
    { ZH_MZ, "RV", 0 }, { "\xCA\xD5\xD2\xE6", "YLD%", 0 }, { "\xBC\xDB\xB8\xF1", "PRICE", 0 } }; // 收益/价格
static const FcFld deprecFlds[5] = {
    { ZH_CB, "COST", 0 }, { ZH_CZ, "SALV", 0 }, { ZH_SM, "LIFE", 2 },
    { ZH_QJ, "PER", 2 }, { ZH_ZJ, "DEP", 0 } };
static const FcFld dateFlds[3] = {
    { ZH_RIQI "1", "D1", 1 }, { ZH_RIQI "2", "D2", 1 }, { ZH_TIANSHU, "DAYS", 2 } };
static const FcFld iconvFlds[3] = {
    { ZH_MYLL, "NOM%", 0 }, { ZH_YXLL, "EFF%", 0 }, { ZH_FLQS, "C/Y", 2 } };
static const FcFld marginFlds[3] = {
    { ZH_CB, "COST", 0 }, { ZH_SHOUJIA, "PRICE", 0 }, { ZH_LRL, "MARG%", 0 } };

static const char *menuTvm[6] = { "N", "i%", "PV", "PMT", "FV", "B/E" };
static const char *menuAmort[6] = { "INT", "PRIN", "BAL", "_", "_", "_" };
static const char *menuBond[6] = { "PRICE", "YLD", "_", "_", "_", "FRQ" };
static const char *menuDeprec[6] = { "SL", "DB", "SYD", "_", "_", "_" };
static const char *menuDate[6] = { "DAYS", "+DYS", "_", "_", "_", "360A" };
static const char *menuIconv[6] = { "NOM", "EFF", "_", "_", "_", "_" };
static const char *menuMargin[6] = { "PRICE", "COST", "MARG", "_", "_", "_" };


// ==================== 电子工程（fcMod==2，P3） ====================
// 字段类型：ft=4 → 8 位有效工程格式（不用 FIX）
static void eeW(int f, int i, double v) { fv_[2][f][i] = v; fh_[2][f][i] = 1; }
static double eeV(int f, int i) { return fv_[2][f][i]; }
static int eeH(int f, int i) { return fh_[2][f][i]; }

static const FcFld ohmFlds[4] = {
    { "\xB5\xE7\xD1\xB9", "V", 0 }, { "\xB5\xE7\xC1\xF7", "A", 0 },
    { "\xB5\xE7\xD7\xE8", "OHM", 0 }, { "\xB9\xA6\xC2\xCA", "W", 0 } };
static const char *menuOhm[6] = { "V", "I", "R", "P", "_", "_" };
static int actOhm(int slot) {
    if (slot == 0) { if (!(eeH(0,1) && eeH(0,2))) return 1; eeW(0,0, eeV(0,1)*eeV(0,2)); }
    else if (slot == 1) { if (!(eeH(0,0) && eeH(0,2)) || eeV(0,2) == 0) return 1; eeW(0,1, eeV(0,0)/eeV(0,2)); }
    else if (slot == 2) { if (!(eeH(0,0) && eeH(0,1)) || eeV(0,1) == 0) return 1; eeW(0,2, eeV(0,0)/eeV(0,1)); }
    else if (slot == 3) {
        if (eeH(0,0) && eeH(0,1)) eeW(0,3, eeV(0,0)*eeV(0,1));
        else if (eeH(0,0) && eeH(0,2)) { if (eeV(0,2) == 0) return 1; eeW(0,3, eeV(0,0)*eeV(0,0)/eeV(0,2)); }
        else if (eeH(0,1) && eeH(0,2)) eeW(0,3, eeV(0,1)*eeV(0,1)*eeV(0,2));
        else return 1;
    } else return 1;
    fcSave();
    return 0;
}

static const FcFld vdivFlds[4] = {
    { "\xCA\xE4\xC8\xEB\xB5\xE7\xD1\xB9", "Vin", 0 }, { "\xC9\xCF\xB5\xE7\xD7\xE8", "R1", 0 },
    { "\xCF\xC2\xB5\xE7\xD7\xE8", "R2", 0 }, { "\xCA\xE4\xB3\xF6\xB5\xE7\xD1\xB9", "Vout", 0 } };
static const char *menuVdiv[6] = { "Vout", "R2", "R1", "_", "_", "_" };
static int actVdiv(int slot) {
    if (slot == 0) { if (!(eeH(1,0) && eeH(1,1) && eeH(1,2))) return 1; double d = eeV(1,1)+eeV(1,2); if (d == 0) return 1; eeW(1,3, eeV(1,0)*eeV(1,2)/d); }
    else if (slot == 1) { if (!(eeH(1,0) && eeH(1,2) && eeH(1,3))) return 1; double d = eeV(1,0)-eeV(1,3); if (d == 0) return 1; eeW(1,1, eeV(1,3)*eeV(1,2)/d); }
    else if (slot == 2) { if (!(eeH(1,0) && eeH(1,1) && eeH(1,3))) return 1; double v = eeV(1,3); if (v == 0) return 1; eeW(1,2, eeV(1,1)*(eeV(1,0)-v)/v); }
    else return 1;
    fcSave();
    return 0;
}

static const FcFld rparFlds[4] = {
    { "R1", "R1", 4 }, { "R2", "R2", 4 },
    { "\xB4\xAE\xC1\xAA", "Rs", 4 }, { "\xB2\xA2\xC1\xAA", "Rp", 4 } };
static const char *menuRpar[6] = { "CALC", "_", "_", "_", "_", "_" };
static int actRpar(int slot) {
    if (slot != 0) return 1;
    if (!(eeH(2,0) && eeH(2,1))) return 1;
    double d = eeV(2,0) + eeV(2,1);
    eeW(2,2, d);
    if (d != 0) eeW(2,3, eeV(2,0)*eeV(2,1)/d);
    else fh_[2][2][3] = 0;
    fcSave();
    return 0;
}

static const FcFld rcFlds[3] = {
    { "R", "OHM", 4 }, { "\xB5\xE7\xC8\xDD", "uF", 4 },
    { "\xCA\xB1\xBC\xE4\xB3\xA3\xCA\xFD", "TAU", 4 } };
static const char *menuRc[6] = { "TAU", "R", "C", "_", "_", "_" };
static int actRc(int slot) {
    if (slot == 0) { if (!(eeH(3,0) && eeH(3,1))) return 1; eeW(3,2, eeV(3,0)*eeV(3,1)*1e-3); } // τ(ms)=R×C(µF)×1e-3
    else if (slot == 1) { if (!(eeH(3,1) && eeH(3,2)) || eeV(3,1) == 0) return 1; eeW(3,0, eeV(3,2)*1000.0/eeV(3,1)); }
    else if (slot == 2) { if (!(eeH(3,0) && eeH(3,2)) || eeV(3,0) == 0) return 1; eeW(3,1, eeV(3,2)*1000.0/eeV(3,0)); }
    else return 1;
    fcSave();
    return 0;
}

static const FcFld resoFlds[3] = {
    { "\xB5\xE7\xB8\xD0", "uH", 4 }, { "\xB5\xE7\xC8\xDD", "pF", 4 },
    { "\xD0\xB3\xD5\xF1\xC6\xB5\xC2\xCA", "f0", 4 } };
static const char *menuReso[6] = { "f0", "L", "C", "_", "_", "_" };
static int actReso(int slot) {
    // L µH × C pF → f0(MHz) = 1000/(2π√(LC))
    if (slot == 0) { if (!(eeH(4,0) && eeH(4,1)) || eeV(4,0) <= 0 || eeV(4,1) <= 0) return 1;
        eeW(4,2, 1000.0/(2*3.14159265358979*sqrt(eeV(4,0)*eeV(4,1)))); }
    else if (slot == 1) { if (!(eeH(4,1) && eeH(4,2)) || eeV(4,1) <= 0 || eeV(4,2) <= 0) return 1;
        double k = 1000.0/(2*3.14159265358979*eeV(4,2)); eeW(4,0, k*k/eeV(4,1)); }
    else if (slot == 2) { if (!(eeH(4,0) && eeH(4,2)) || eeV(4,0) <= 0 || eeV(4,2) <= 0) return 1;
        double k = 1000.0/(2*3.14159265358979*eeV(4,2)); eeW(4,1, k*k/eeV(4,0)); }
    else return 1;
    fcSave();
    return 0;
}

static const FcFld freqFlds[2] = {
    { "\xC6\xB5\xC2\xCA", "Hz", 4 }, { "\xD6\xDC\xC6\xDA", "Tms", 4 } };
static const char *menuFreq[6] = { "T", "f", "_", "_", "_", "_" };
static int actFreq(int slot) {
    if (slot == 0) { if (!eeH(5,0) || eeV(5,0) == 0) return 1; eeW(5,1, 1000.0/eeV(5,0)); } // T(ms)=1000/f
    else if (slot == 1) { if (!eeH(5,1) || eeV(5,1) == 0) return 1; eeW(5,0, 1000.0/eeV(5,1)); }
    else return 1;
    fcSave();
    return 0;
}

static const FcFld sineFlds[3] = {
    { "\xB7\xE5\xD6\xB5", "Vp", 4 }, { "\xD3\xD0\xD0\xA7\xD6\xB5", "Vrms", 4 },
    { "\xB7\xE5\xB7\xE5\xD6\xB5", "Vpp", 4 } };
static const char *menuSine[6] = { "Vp", "Vrms", "Vpp", "_", "_", "_" };
static int actSine(int slot) {
    const double S2 = 1.4142135623730951;
    if (slot == 0) {
        if (eeH(6,1)) eeW(6,0, eeV(6,1)*S2);
        else if (eeH(6,2)) eeW(6,0, eeV(6,2)/2);
        else return 1;
    } else if (slot == 1) {
        if (eeH(6,0)) eeW(6,1, eeV(6,0)/S2);
        else if (eeH(6,2)) eeW(6,1, eeV(6,2)/(2*S2));
        else return 1;
    } else if (slot == 2) {
        if (eeH(6,0)) eeW(6,2, 2*eeV(6,0));
        else if (eeH(6,1)) eeW(6,2, 2*S2*eeV(6,1));
        else return 1;
    } else return 1;
    fcSave();
    return 0;
}

static const FcFld dbmFlds[2] = {
    { "\xB9\xA6\xC2\xCA", "W", 4 }, { "\xB7\xD6\xB1\xB4\xBA\xC1\xCD\xDF", "dBm", 4 } };
static const char *menuDbm[6] = { "dBm", "W", "_", "_", "_", "_" };
static int actDbm(int slot) {
    if (slot == 0) { if (!eeH(7,0) || eeV(7,0) <= 0) return 1; eeW(7,1, 10.0*log10(eeV(7,0)/1e-3)); }
    else if (slot == 1) { if (!eeH(7,1)) return 1; eeW(7,0, 1e-3*pow(10.0, eeV(7,1)/10.0)); }
    else return 1;
    fcSave();
    return 0;
}

static const FcFld xfmrFlds[4] = {
    { "\xB3\xF5\xBC\xB6\xB5\xE7\xD1\xB9", "Vp", 4 }, { "\xB4\xCE\xBC\xB6\xB5\xE7\xD1\xB9", "Vs", 4 },
    { "\xB3\xF5\xBC\xB6\xD4\xD1\xCA\xFD", "Np", 4 }, { "\xB4\xCE\xBC\xB6\xD4\xD1\xCA\xFD", "Ns", 4 } };
static const char *menuXfmr[6] = { "Vp", "Vs", "Np", "Ns", "_", "_" };
static int actXfmr(int slot) {
    if (slot == 0) { if (!(eeH(8,1) && eeH(8,2) && eeH(8,3)) || eeV(8,3) == 0) return 1; eeW(8,0, eeV(8,1)*eeV(8,2)/eeV(8,3)); }
    else if (slot == 1) { if (!(eeH(8,0) && eeH(8,2) && eeH(8,3)) || eeV(8,2) == 0) return 1; eeW(8,1, eeV(8,0)*eeV(8,3)/eeV(8,2)); }
    else if (slot == 2) { if (!(eeH(8,0) && eeH(8,1) && eeH(8,3)) || eeV(8,1) == 0) return 1; eeW(8,2, eeV(8,3)*eeV(8,0)/eeV(8,1)); } // Np = Ns*Vp/Vs
    else if (slot == 3) { if (!(eeH(8,0) && eeH(8,1) && eeH(8,2)) || eeV(8,0) == 0) return 1; eeW(8,3, eeV(8,2)*eeV(8,1)/eeV(8,0)); }
    else return 1;
    fcSave();
    return 0;
}

static int (*eeActTbl[9])(int slot) = { actOhm, actVdiv, actRpar, actRc, actReso,
                                        actFreq, actSine, actDbm, actXfmr };
static const char **eeMenuTbl[9] = { menuOhm, menuVdiv, menuRpar, menuRc, menuReso,
                                     menuFreq, menuSine, menuDbm, menuXfmr };
static const FcFld *eeFldSets[9] = { ohmFlds, vdivFlds, rparFlds, rcFlds, resoFlds,
                                     freqFlds, sineFlds, dbmFlds, xfmrFlds };
static const int eeFldCnt[9] = { 4, 4, 4, 3, 3, 2, 3, 2, 4 };

static void fcInitEeFmts(void) {
    for (int f = 0; f < 9; f++) {
        eeFmts[f].nf = eeFldCnt[f];
        eeFmts[f].rowBase = 20;
        eeFmts[f].rowStep = 18;
        eeFmts[f].f = eeFldSets[f];
    }
}

static void fcInitFmts(void) {
    finFmts[0].nf = 5; finFmts[0].rowBase = 20; finFmts[0].rowStep = 18; finFmts[0].f = tvmFlds;
    finFmts[1].nf = 0; // CFLOW 特殊
    finFmts[2].nf = 6; finFmts[2].rowBase = 14; finFmts[2].rowStep = 16; finFmts[2].f = amortFlds;
    finFmts[3].nf = 6; finFmts[3].rowBase = 14; finFmts[3].rowStep = 16; finFmts[3].f = bondFlds;
    finFmts[4].nf = 5; finFmts[4].rowBase = 20; finFmts[4].rowStep = 18; finFmts[4].f = deprecFlds;
    finFmts[5].nf = 3; finFmts[5].rowBase = 20; finFmts[5].rowStep = 18; finFmts[5].f = dateFlds;
    finFmts[6].nf = 3; finFmts[6].rowBase = 20; finFmts[6].rowStep = 18; finFmts[6].f = iconvFlds;
    finFmts[7].nf = 3; finFmts[7].rowBase = 20; finFmts[7].rowStep = 18; finFmts[7].f = marginFlds;
    fcInitEeFmts();
}

// 表单动作表
static int (*finActTbl[8])(int slot) = { actTvm, actCflow, actAmort, actBond,
                                          actDeprec, actDate, actIconv, actMargin };
static const char **finMenuTbl[8] = { menuTvm, 0, menuAmort, menuBond,
                                     menuDeprec, menuDate, menuIconv, menuMargin };

// ---- 绘制（层）----
static void drawUnderConstruction(void);
static void drawFormScreen(void) {
    if (fcMod == 1 && fcForm == 1) { drawCflow(); return; }
    const char **ms;
    if (fcMod == 2) ms = (const char **)eeMenuTbl[fcForm];
    else ms = (const char **)finMenuTbl[fcForm];
    const char *menus[6];
    for (int i = 0; i < 6; i++) menus[i] = ms[i];
    if (fcMod == 1 && fcForm == 0) { // TVM 标题（期末/期初）
        char right[24];
        sprintf(right, "FIX%d %s", fcFix, (fs_[1][0] & 1) ? ZH_QICHU : ZH_QIMMO);
        char tt[48];
        sprintf(tt, "TVM %s", finItems[0]);
        drawTitle(tt, right);
        for (int i = 0; i < 5; i++) drawStdRow(0, i);
    } else if (fcMod == 1 && fcForm == 3) { // BOND 标题（付息频）
        char right[24];
        sprintf(right, "FIX%d frq%d", fcFix, (int)bondFreq[fs_[1][3] & 3]);
        char tt[48];
        sprintf(tt, "BOND %s", finItems[3]);
        drawTitle(tt, right);
        for (int i = 0; i < 6; i++) drawStdRow(3, i);
    } else if (fcMod == 1 && fcForm == 5) { // DATE 标题（日基）
        char tt[48];
        sprintf(tt, "DATE %s", finItems[5]);
        drawTitle(tt, (fs_[1][5] & 1) ? "ACT" : "360");
        for (int i = 0; i < 3; i++) drawStdRow(5, i);
    } else {
        char tt[48];
        if (fcMod == 2) sprintf(tt, "%s %s", eeAbbr[fcForm], eeItems[fcForm]);
        else sprintf(tt, "%s %s", finAbbr[fcForm], finItems[fcForm]);
        drawTitle(tt, NULL);
        int nf;
        fcFormDef(fcForm, &nf, 0, 0, 0);
        for (int i = 0; i < nf; i++) drawStdRow(fcForm, i);
    }
    drawMenu(menus);
}

static void drawFormula(void) { // TVM 公式视图（只读）
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    fDrawMix(2, 0, "TVM " ZH_HSJZ " formula", 0, 255);
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

static const char *l0Names[3] = { ZH_DANWEI, ZH_JINRONGQI, ZH_DIANZI };
static const char *fcItemName(int mod, int idx) {
    if (mod == 1) return finItems[idx];
    if (mod == 2) return eeItems[idx];
    if (mod == 3) return unItems[idx];
    return l0Names[idx];
}
static const char *fcItemAbbr(int mod, int idx) {
    if (mod == 1) return finAbbr[idx];
    if (mod == 2) return eeAbbr[idx];
    return unAbbr[idx];
}
static int fcModCount(void) {
    return fcMod == 1 ? finCount : (fcMod == 2 ? eeCount : unCount);
}

static void drawL0(void) {
    const char *menus[6] = { "UNIT", "FIN", "EE", "_", "_", "FIX" };
    drawTitle("FormCalc " ZH_BIAODAN, NULL);
    for (int i = 0; i < 3; i++) {
        int y = 20 + i * 24;
        if (i == fcSel) {
            uidisp->draw_box(0, y - 2, 255, y + 15, 255, 0);
            fDrawMix(4, y, fcItemName(0, i), 255, 0);
        } else {
            fDrawMix(4, y, fcItemName(0, i), 0, 255);
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
        char pre[8];
        sprintf(pre, "%d ", idx + 1);
        int x = 4;
        uidisp->draw_printf(x, y, 12, sel ? 255 : 0, sel ? 0 : 255, "%s", pre);
        x += 4 * 8;
        fDrawMix(x, y, fcItemName(fcMod, idx), sel ? 255 : 0, sel ? 0 : 255);
        x = 150;
        uidisp->draw_printf(x, y, 12, sel ? 255 : 0, sel ? 0 : 255, "%s", fcItemAbbr(fcMod, idx));
    }
    const char *m[6] = { "_", "_", "_", "_", "_", "_" };
    drawMenu(m);
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

// 表单布局定义（fcMod 1 金融 / 2 工程）——绘制与按键共用
static void fcFormDef(int form, int *nf, int *rb, int *rs, const FcFld **f) {
    if (fcMod == 2 && form < 9) {
        if (nf) *nf = eeFmts[form].nf;
        if (rb) *rb = eeFmts[form].rowBase;
        if (rs) *rs = eeFmts[form].rowStep;
        if (f) *f = eeFmts[form].f;
    } else {
        if (nf) *nf = finFmts[form].nf;
        if (rb) *rb = finFmts[form].rowBase;
        if (rs) *rs = finFmts[form].rowStep;
        if (f) *f = finFmts[form].f;
    }
}

// ---- 表单字段行数（CFLOW 特殊）----
static int fcFieldCount(void) {
    if (fcMod == 1 && fcForm == 1) return 14;
    int nf;
    if (fcMod == 1 && fcForm < 8) fcFormDef(fcForm, &nf, 0, 0, 0);
    else if (fcMod == 2 && fcForm < 9) fcFormDef(fcForm, &nf, 0, 0, 0);
    else return 5;
    return nf;
}
static int fcRowBase(void) {
    if (fcMod == 1 && fcForm == 1) return 18;
    int rb;
    fcFormDef(fcForm, 0, &rb, 0, 0);
    return rb;
}
static int fcRowStep(void) {
    if (fcMod == 1 && fcForm == 1) return 16;
    int rs;
    fcFormDef(fcForm, 0, 0, &rs, 0);
    return rs;
}

// ---- 重绘（fcRowFoc>=0：仅重画聚焦行并局部 flush）----
static void drawFocRowOnly(void) {
    if (fcLevel != 2 || (fcMod != 1 && fcMod != 2)) return;
    int y;
    if (fcMod == 1 && fcForm == 1) {
        drawCflowRow(foc);
        y = cfRowY(foc);
    } else {
        drawStdRow(fcForm, foc);
        y = fcRowBase() + foc * fcRowStep();
    }
    uidisp->flushRect(0, y - 1, 255, y + 15);
}

// ---- 单位换算（fcMod==3，定义见文件尾）----
static void drawUnit(void);
static int fcUnitKey(int key);

static void fcDraw(void) {
    if (fcRowFoc >= 0 && fcLevel == 2 && (fcMod == 1 || fcMod == 2)) {
        fcRowFoc = -1;
        drawFocRowOnly();
        return;
    }
    fcRowFoc = -1;
    if (fcLevel == 3) drawFormula();
    else if (fcLevel == 2) {
        if ((fcMod == 1 && fcForm < 8) || (fcMod == 2 && fcForm < 9)) drawFormScreen();
        else if (fcMod == 3 && fcForm >= 0) drawUnit();
        else drawUnderConstruction();
    } else if (fcLevel == 4) drawFixPage();
    else if (fcLevel == 1) drawL1();
    else drawL0();
    uidisp->flush();
}

static void drawUnderConstruction(void) {
    const char *menus[6] = { "_", "_", "_", "_", "_", "_" };
    char tt[48];
    if (fcMod == 1)
        sprintf(tt, "%s %s", finAbbr[fcForm], finItems[fcForm]);
    else
        sprintf(tt, "%s", fcItemName(fcMod, fcForm));
    drawTitle(tt, NULL);
    fDrawMix(40, 50, "\xB4\xCB\xB9\xA6\xC4\xDC " ZH_JSZHONG, 0, 255);
    uidisp->draw_printf(40, 74, 8, 0, 255, "(later release)");
    drawMenu(menus);
}

// ---- L0/L1 进入 ----
static int uCat = 0; // 单位换算当前类别（==fcForm）
static int uSrc = 0; // 源单位索引
static int uTop = 0; // 结果滚动顶
static void fcListSel(void) {
    if (fcLevel == 0) {
        fcMod = fcSel == 0 ? 3 : (fcSel == 1 ? 1 : 2); // 显示序：单位换算/金融/电子工程
        fcSel = 0;
        fcTop = 0;
        fcLevel = 1;
        fcMsg[0] = 0;
        return;
    }
    int cnt = fcModCount();
    int idx = fcTop + fcSel;
    if (idx >= cnt) { idx = cnt - 1; fcSel = cnt - 1 - fcTop; }
    fcForm = idx;
    if (fcMod == 3) { uCat = idx; uSrc = 0; uTop = 0; } // 单位换算：类别进入
    fcLevel = 2;
    fcMsg[0] = 0;
    foc = 0;
    cfTop = 0;
    eAct = 0;
    elen = 0;
    ebuf[0] = 0;
}

// ---- 表单按键（L2 表单）----
static int fcFormKey(int key) {
    int d = fcDigit(key);
    if (d >= 0) { fcEditAppend('0' + d); return 0; }
    if (key == KEY_DOT) { fcEditAppend('.'); return 0; }
    if (key == KEY_NEGATIVE) { fcEditNeg(); return 0; }
    if (key == KEY_BACKSPACE) {
        fcMsg[0] = 0;
        fcRowFoc = foc;
        if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; }
        else if (elen > 0) { elen--; ebuf[elen] = 0; if (elen == 0) { eAct = 0; } }
        drawFocRowOnly();
        fcRowFoc = -1;
        return 0;
    }
    int nf = fcFieldCount();
    if (key == KEY_ENTER) {
        fcMsg[0] = 0;
        fcCommit();
        if (fcForm == 1) { // CFLOW：r → CF0.. 循环
            if (foc < 13) { foc++; if (foc > 1 && foc - 1 > cfTop + 3) cfTop = foc - 4; }
            else { foc = 0; cfTop = 0; }
        } else {
            foc = (foc + 1) % nf;
        }
        return 1;
    }
    if (key == KEY_UP || key == KEY_DOWN) {
        fcMsg[0] = 0;
        fcCommit();
        if (fcForm == 1) {
            int dir = (key == KEY_UP) ? -1 : 1;
            int nf2 = 14;
            int nfoc = foc + dir;
            if (nfoc < 0) nfoc = nf2 - 1;
            if (nfoc >= nf2) nfoc = 0;
            if (nfoc >= 1) {
                while (nfoc - 1 < cfTop) cfTop--;
                while (nfoc - 1 > cfTop + 3) cfTop++;
            } else cfTop = 0;
            if (cfTop < 0) cfTop = 0;
            if (cfTop > 9) cfTop = 9;
            foc = nfoc;
        } else {
            if (key == KEY_UP) foc = (foc + nf - 1) % nf;
            else foc = (foc + 1) % nf;
        }
        return 1;
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
    if (slot >= 0) {
        fcCommit();
        if (fcMod == 1) {
            if (fcForm == 1) { // CFLOW 动作：act 内自带结果消息（NPV=/IRR=/清空ed）；失败=err
                if (actCflow(slot) != 0) strcpy(fcMsg, "err");
            } else if (fcForm == 0) {
                if (actTvm(slot) != 0) strcpy(fcMsg, "no sol");
                else fcMsg[0] = 0;
            } else {
                if (finActTbl[fcForm](slot) != 0) strcpy(fcMsg, "err");
                else if (fcForm == 2 || fcForm == 4) { /* 结果写 RES——无消息 */ }
                else fcMsg[0] = 0;
            }
        } else { // 电子工程
            if (eeActTbl[fcForm](slot) != 0) strcpy(fcMsg, "err");
            else fcMsg[0] = 0;
        }
        return 1;
    }
    if (key == KEY_VIEWS) {
        if (fcMod == 1 && fcForm == 0) fcLevel = 3; // TVM 公式
        return 1;
    }
    return 0;
}

// ---- 按键处理 ----
static int fcHandleKey(int key) {
    if (fcLevel == 3) {
        if (key == KEY_VIEWS || key == KEY_ON) { fcLevel = 2; return 1; }
        return 0;
    }
    if (fcLevel == 2 && ((fcMod == 1 && fcForm < 8) || (fcMod == 2 && fcForm < 9))) return fcFormKey(key);
    if (fcLevel == 2 && fcMod == 3) return fcUnitKey(key);
    if (fcLevel == 2) { // 建设中页
        if (key == KEY_ON || key == KEY_HOME || key == KEY_BACKSPACE || key == KEY_APPS) {
            fcLevel = 1;
            fcMsg[0] = 0;
            return 1;
        }
        return 0;
    }
    if (fcLevel == 4) {
        int d = fcDigit(key);
        if (d >= 0) { fcFix = d; fcFixSel = d; fcSave(); fcLevel = 0; return 1; }
        if (key == KEY_UP) { if (fcFixSel > 0) fcFixSel--; return 1; }
        if (key == KEY_DOWN) { if (fcFixSel < 9) fcFixSel++; return 1; }
        if (key == KEY_ENTER) { fcFix = fcFixSel; fcSave(); fcLevel = 0; return 1; }
        if (key == KEY_F6) { fcFix = fcFixSel; fcSave(); fcLevel = 0; return 1; }
        if (key == KEY_ON || key == KEY_HOME || key == KEY_BACKSPACE) { fcLevel = 0; return 1; }
        return 0;
    }
    if (fcLevel == 1) {
        int cnt = fcModCount();
        if (key == KEY_UP) {
            if (fcTop + fcSel > 0) { if (fcSel == 0 && fcTop > 0) fcTop--; else fcSel--; }
            else { fcTop = cnt > 5 ? cnt - 5 : 0; fcSel = cnt - fcTop - 1; } // 顶部再上 → 跳底部（循环）
            return 1;
        }
        if (key == KEY_DOWN) {
            if (fcTop + fcSel < cnt - 1) { if (fcSel == 4 && fcTop + 4 < cnt - 1) fcTop++; else fcSel++; }
            else { fcTop = 0; fcSel = 0; } // 底部再下 → 跳顶部（循环）
            return 1;
        }
        if (key == KEY_ENTER) { fcListSel(); return 1; }
        return 0;
    }
    if (key == KEY_UP) { fcSel = (fcSel + 2) % 3; return 1; } // 循环
    if (key == KEY_DOWN) { fcSel = (fcSel + 1) % 3; return 1; } // 循环
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
    fcInitFmts();
    fcLevel = 0;
    fcSel = 0;
    fcTop = 0;
    fcMod = 1;
    fcMsg[0] = 0;
    fcClrArm = 0;
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
                    if (shiftHeld) { shiftHeld = 0; ll_disp_set_indicator(0, -1); fcClrArm = 0; }
                    else { shiftHeld = 1; ll_disp_set_indicator(INDICATE_LEFT, -1); }
                    fcDraw();
                }
                lastKey = key;
            } else if (key != (uint32_t)lastKey) {
                lastKey = key;
                if (shiftHeld) {
                    int d = fcDigit((int)key);
                    if (d >= 0) { fcFix = d; fcSave(); shiftHeld = 0; ll_disp_set_indicator(0, -1); fcDraw(); }
                    else if (key == KEY_ON) { fcRun = 0; shiftHeld = 0; ll_disp_set_indicator(0, -1); }
                    else if (key == KEY_BACKSPACE) { // 清空所有（两遍确认）
                        if (fcClrArm) {
                            fcClearAll();
                            fcClrArm = 0;
                            strcpy(fcMsg, "Cleared");
                        } else {
                            fcClrArm = 1;
                            strcpy(fcMsg, "CLR ALL? Sh+BKSP");
                        }
                        shiftHeld = 0;
                        ll_disp_set_indicator(0, -1);
                        fcDraw();
                    } else { shiftHeld = 0; ll_disp_set_indicator(0, -1); fcDraw(); }
                } else {
                    if (key == KEY_ON) {
                        if (fcClrArm) { fcClrArm = 0; fcMsg[0] = 0; }
                        if (fcLevel == 3) { fcLevel = 2; fcMsg[0] = 0; fcDraw(); }
                        else if (fcLevel == 2) { fcLevel = 1; fcMsg[0] = 0; fcDraw(); }
                        else if (fcLevel == 1) { fcLevel = 0; fcSel = fcMod - 1; fcMsg[0] = 0; fcDraw(); }
                        else if (fcLevel == 4) { fcLevel = 0; fcDraw(); }
                    } else if (key == KEY_HOME) {
                        if (fcLevel != 0) { fcCommit(); fcClrArm = 0; fcSel = fcMod - 1; fcLevel = 0; fcMsg[0] = 0; fcDraw(); }
                    } else if (key == KEY_APPS) {
                        if (fcLevel >= 2 && fcMod == 1) { fcLevel = 1; fcMsg[0] = 0; fcDraw(); }
                        else if (fcLevel == 1 && fcMod == 1) { fcLevel = 0; fcSel = 0; fcDraw(); }
                    } else if (key == KEY_VIEWS) {
                        if (fcLevel == 2 && fcMod == 1 && fcForm == 0) { fcLevel = 3; fcDraw(); }
                        else if (fcLevel == 3) { fcLevel = 2; fcDraw(); }
                    } else {
                        if (fcHandleKey((int)key)) fcDraw();
                    }
                }
            }
        } else {
            if (lastKey != -1) lastKey = -1;
            vTaskDelay(5);
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

// ==================== 单位换算（P4，fcMod==3） ====================
// 类别表：中文名在 unItems（L1 列表）；这里符号/系数/温度标志
struct UnItem { const char *nm; const char *sy; double f; }; // nm=中文 16px；sy=ASCII 符号；f=相对基准系数（温度类 f 未用）
struct UnCat  { const char *ab; int n; int tmp; const UnItem *it; }; // tmp=1 温度（公式对，非线性）

static const UnItem itLen[] = { // 基准 m
    { "\xB0\xAC\xC3\xD7", "mm", 1e-3 }, { "\xC0\xE5\xC3\xD7", "cm", 1e-2 },
    { "\xC3\xD7", "m", 1 }, { "\xC7\xA7\xC3\xD7", "km", 1e3 },
    { "\xD3\xA2\xB4\xE7", "in", 0.0254 }, { "\xD3\xA2\xB3\xDF", "ft", 0.3048 },
    { "\xC2\xEB", "yd", 0.9144 }, { "\xD3\xA2\xC0\xEF", "mile", 1609.344 } };
static const UnItem itAre[] = { // 基准 m2
    { "\xC6\xBD\xB7\xBD\xC3\xD7", "m2", 1 }, { "\xC6\xBD\xB7\xBD\xC0\xE5\xC3\xD7", "cm2", 1e-4 },
    { "\xC6\xBD\xB7\xBD\xD3\xA2\xB3\xDF", "ft2", 0.09290304 }, { "\xC6\xBD\xB7\xBD\xD3\xA2\xB4\xE7", "in2", 6.4516e-4 },
    { "\xC4\xB6", "mu", 666.6666667 } };
static const UnItem itVol[] = { // 基准 m3
    { "\xBA\xC1\xC9\xFD", "mL", 1e-6 }, { "\xC9\xFD", "L", 1e-3 },
    { "\xC1\xA2\xB7\xBD\xC3\xD7", "m3", 1 }, { "\xBC\xD3\xC2\xD8\x28\xC3\xC0\x29", "gal", 3.785411784e-3 },
    { "\xD2\xBA\xB0\xBB\xCB\xBE\x28\xC3\xC0\x29", "floz", 2.95735295625e-5 } };
static const UnItem itMas[] = { // 基准 kg
    { "\xBF\xCB", "g", 1e-3 }, { "\xC7\xA7\xBF\xCB", "kg", 1 },
    { "\xB6\xD6", "t", 1e3 }, { "\xB0\xBB\xCB\xBE", "oz", 0.028349523125 },
    { "\xB0\xF5", "lb", 0.45359237 } };
static const UnItem itTmp[] = { // 温度：C/F/K（公式对）
    { "\xC9\xE3\xCA\xCF\xB6\xC8", "C", 0 }, { "\xBB\xAA\xCA\xCF\xB6\xC8", "F", 0 },
    { "\xBF\xAA\xB6\xFB\xCE\xC4", "K", 0 } };
static const UnItem itVel[] = { // 基准 m/s
    { "\xC3\xD7\xC3\xBF\xC3\xEB", "m/s", 1 }, { "\xC7\xA7\xC3\xD7\xC3\xBF\xCA\xB1", "km/h", 1.0 / 3.6 },
    { "\xD3\xA2\xC0\xEF\xC3\xBF\xCA\xB1", "mph", 0.44704 }, { "\xBD\xDA", "kn", 0.5144444444 } };
static const UnItem itPre[] = { // 基准 Pa
    { "\xC5\xC1\xCB\xB9\xBF\xA8", "Pa", 1 }, { "\xC7\xA7\xC5\xC1", "kPa", 1e3 },
    { "\xD5\xD7\xC5\xC1", "MPa", 1e6 }, { "\xB0\xCD", "bar", 1e5 },
    { "\xB0\xF5\xC3\xBF\xC6\xBD\xB7\xBD\xD3\xA2\xB4\xE7", "psi", 6894.757 },
    { "\xB4\xF3\xC6\xF8\xD1\xB9", "atm", 101325.0 }, { "\xBA\xC1\xC3\xD7\xB9\xAF\xD6\xF9", "mmHg", 133.3224 } };
static const UnItem itEne[] = { // 基准 J
    { "\xBD\xB9\xB6\xFA", "J", 1 }, { "\xC7\xA7\xBD\xB9", "kJ", 1e3 },
    { "\xBF\xA8", "cal", 4.184 }, { "\xC7\xA7\xBF\xA8", "kcal", 4184 },
    { "\xCD\xDF\xCA\xB1", "Wh", 3600 }, { "\xC7\xA7\xCD\xDF\xCA\xB1", "kWh", 3.6e6 } };
static const UnItem itPow[] = { // 基准 W
    { "\xCD\xDF", "W", 1 }, { "\xC7\xA7\xCD\xDF", "kW", 1e3 },
    { "\xC2\xED\xC1\xA6", "hp", 745.699872 }, { "\xD3\xA2\xC8\xC8\xC3\xBF\xCA\xB1", "Btu/h", 0.29307107 } };
static const UnItem itDat[] = { // 基准 B（1024 进制）
    { "\xD7\xD6\xBD\xDA", "B", 1 }, { "\xC7\xA7\xD7\xD6\xBD\xDA", "KB", 1.024e3 },
    { "\xD5\xD7\xD7\xD6\xBD\xDA", "MB", 1.048576e6 }, { "\xBC\xAA\xD7\xD6\xBD\xDA", "GB", 1.073741824e9 },
    { "\xCC\xAB\xD7\xD6\xBD\xDA", "TB", 1.099511627776e12 }, { "\xB1\xC8\xCC\xD8", "bit", 0.125 } };

static const UnCat unCats[10] = {
    { "LEN",  8, 0, itLen }, { "AREA", 5, 0, itAre }, { "VOL",  5, 0, itVol },
    { "MASS", 5, 0, itMas }, { "TEMP", 3, 1, itTmp }, { "VEL",  4, 0, itVel },
    { "PRES", 7, 0, itPre }, { "ENER", 6, 0, itEne }, { "POW",  4, 0, itPow },
    { "DATA", 6, 0, itDat } };
static double uVal = 0; // 已提交值（eAct 编辑中以 ebuf 为准）

static double tmpToK(double v, int s) {
    if (s == 0) return v + 273.15;        // C
    if (s == 1) return (v - 32) * 5 / 9 + 273.15; // F
    return v;                              // K
}
static double tmpFromK(double k, int t) {
    if (t == 0) return k - 273.15;
    if (t == 1) return (k - 273.15) * 9 / 5 + 32;
    return k;
}
static double unConv(int cat, int src, int dst, double v) {
    if (unCats[cat].tmp) { double k = tmpToK(v, src); return tmpFromK(k, dst); }
    return v * unCats[cat].it[src].f / unCats[cat].it[dst].f;
}

// 数值格式化（12 位内：极值 e 记法，其余 %g）
static void unFmt(double v, char *b) {
    double a = v < 0 ? -v : v;
    if (a == 0) { strcpy(b, "0"); return; }
    if (a >= 1e10 || a < 1e-5) sprintf(b, "%.5e", v);
    else sprintf(b, "%.8g", v);
}

static const char *unMenus[6] = { "CLR", "_", "_", "_", "_", "_" };

static void drawUnit(void) {
    const UnCat *c = &unCats[uCat];
    char tt[48], rb[20];
    sprintf(tt, "UNIT %s", unItems[uCat]);
    sprintf(rb, "%d/%d", uSrc + 1, c->n);
    drawTitle(tt, rb);
    // 值行：整行黑底白字（输入焦点常驻样式——与金融聚焦行一致）
    uidisp->draw_box(0, 18, 255, 35, 255, 0);
    fDrawMix(4, 20, "\xCA\xFD\xD6\xB5", 255, 0);
    uidisp->draw_printf(84, 22, 12, 255, 0, "VALUE");
    {
        char vb[24];
        const char *txt;
        if (eAct) { txt = (elen > 12) ? ebuf + elen - 12 : ebuf; }
        else { unFmt(uVal, vb); txt = vb; }
        uidisp->draw_printf(252 - (int)strlen(txt) * 8, 20, 16, 255, 0, "%s", txt);
    }
    // 源单位行：中文名（16px）+ 符号（普通显示——选择指示在下方列表灰底行）
    const UnItem *src = &c->it[uSrc];
    fDrawMix(4, 40, src->nm, 0, 255);
    uidisp->draw_printf(150, 42, 12, 0, 255, src->sy);
    uidisp->draw_printf(206, 42, 12, 0, 255, "<->");
    uidisp->draw_line(0, 60, 255, 60, 200);
    // 结果 4 行（可视，行高 12 紧排）；当前源单位行灰底黑字（区别于输入行黑底白字）
    for (int r = 0; r < 4; r++) {
        int idx = uTop + r;
        if (idx >= c->n) break;
        int y = 62 + r * 12;
        int sel = (idx == uSrc);
        if (sel) uidisp->draw_box(0, y - 1, 255, y + 11, 255, 190); // 灰底（fill=190）
        const UnItem *it = &c->it[idx];
        uidisp->draw_printf(6, y, 12, 0, sel ? 190 : 255, "%s", it->sy);
        double v = unConv(uCat, uSrc, idx, eAct ? atof(ebuf) : uVal);
        char vb[24];
        unFmt(v, vb);
        uidisp->draw_printf(252 - (int)strlen(vb) * 8, y, 12, 0, sel ? 190 : 255, "%s", vb);
    }
    drawMenu(unMenus);
}

// 输入局部刷新：值行 + 结果区整行重画后按全宽横条 flush
// 输入局部刷新（值列子宽——UICore.h flushRect 已支持子宽连续化，2026-09-06）
static void unDrawValueCol(void) {
    const UnCat *c = &unCats[uCat];
    uidisp->draw_box(136, 18, 254, 35, 255, 0); // 值区清底（黑底常驻）
    {
        char vb[24];
        const char *txt;
        if (eAct) { txt = (elen > 12) ? ebuf + elen - 12 : ebuf; }
        else { unFmt(uVal, vb); txt = vb; }
        uidisp->draw_printf(252 - (int)strlen(txt) * 8, 20, 16, 255, 0, "%s", txt);
    }
    for (int r = 0; r < 4; r++) {
        int idx = uTop + r;
        if (idx >= c->n) break;
        int y = 62 + r * 12;
        int sel = (idx == uSrc);
        uidisp->draw_box(150, y - 1, 254, y + 11, 255, sel ? 190 : 255); // 值列清底（旧文本残留清除）
        double v = unConv(uCat, uSrc, idx, eAct ? atof(ebuf) : uVal);
        char vb[24];
        unFmt(v, vb);
        uidisp->draw_printf(252 - (int)strlen(vb) * 8, y, 12, 0, sel ? 190 : 255, "%s", vb);
    }
    uidisp->flushRect(136, 18, 254, 35);   // 值行值区
    uidisp->flushRect(150, 61, 254, 111);  // 结果值列
}

// 整行内容变化（←→ 源单位切换、↑↓ 滚动）：整行重画 + 全宽条 flush
static void unDrawRowsFull(int r0, int r1) {
    const UnCat *c = &unCats[uCat];
    for (int r = r0; r <= r1; r++) {
        int y;
        if (r == 0) {
            y = 20;
            uidisp->draw_box(0, 18, 255, 35, 255, 0);
            fDrawMix(4, y, "\xCA\xFD\xD6\xB5", 255, 0);
            uidisp->draw_printf(84, y, 12, 255, 0, "VALUE");
            char vb[24];
            const char *txt;
            if (eAct) { txt = (elen > 12) ? ebuf + elen - 12 : ebuf; }
            else { unFmt(uVal, vb); txt = vb; }
            uidisp->draw_printf(252 - (int)strlen(txt) * 8, y, 16, 255, 0, "%s", txt);
        } else if (r == 1) {
            y = 40;
            const UnItem *src = &c->it[uSrc];
            uidisp->draw_box(0, y - 1, 255, y + 15, 255, 255);
            fDrawMix(4, y, src->nm, 0, 255);
            uidisp->draw_printf(150, y + 2, 12, 0, 255, src->sy);
            uidisp->draw_printf(206, y + 2, 12, 0, 255, "<->");
        } else {
            int idx = uTop + (r - 2);
            if (idx >= c->n) break;
            y = 62 + (r - 2) * 12;
            int sel = (idx == uSrc);
            if (sel) uidisp->draw_box(0, y - 1, 255, y + 11, 255, 190);
            const UnItem *it = &c->it[idx];
            uidisp->draw_printf(6, y, 12, 0, sel ? 190 : 255, "%s", it->sy);
            double v = unConv(uCat, uSrc, idx, eAct ? atof(ebuf) : uVal);
            char vb[24];
            unFmt(v, vb);
            uidisp->draw_printf(252 - (int)strlen(vb) * 8, y, 12, 0, sel ? 190 : 255, "%s", vb);
        }
    }
}

static void unRedrawValue(void) {
    unDrawValueCol(); // 输入只变值列——子宽局部
}
static void unEdit(char c) {
    if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; }
    if (elen < 20) { ebuf[elen++] = c; ebuf[elen] = 0; }
    unRedrawValue();
}
static void unEditNeg(void) {
    if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; }
    if (ebuf[0] == '-') { memmove(ebuf, ebuf + 1, elen); elen--; }
    else { memmove(ebuf + 1, ebuf, elen + 1); ebuf[0] = '-'; elen++; }
    unRedrawValue();
}

static int fcUnitKey(int key) {
    int d = fcDigit(key);
    if (d >= 0) { unEdit((char)('0' + d)); return 0; }
    if (key == KEY_DOT) { unEdit('.'); return 0; }
    if (key == KEY_NEGATIVE) { unEditNeg(); return 0; }
    if (key == KEY_BACKSPACE) {
        if (!eAct) { eAct = 1; elen = 0; ebuf[0] = 0; }
        else if (elen > 0) { elen--; ebuf[elen] = 0; if (elen == 0) eAct = 0; }
        unRedrawValue();
        return 0;
    }
    const UnCat *c = &unCats[uCat];
    int moved = 0;
    if (key == KEY_LEFT) { uSrc = (uSrc + c->n - 1) % c->n; moved = 1; }
    if (key == KEY_RIGHT) { uSrc = (uSrc + 1) % c->n; moved = 1; }
    if (moved) {
        if (uSrc < uTop) uTop = uSrc;
        if (uSrc > uTop + 3) uTop = uSrc - 3;
        char rb[16];
        sprintf(rb, "%d/%d", uSrc + 1, c->n); // 标题右序号局部重绘
        uidisp->draw_box(150, 0, 255, 17, 255, 255);
        uidisp->draw_printf(254 - (int)strlen(rb) * 8, 2, 12, 0, 255, "%s", rb);
        uidisp->flushRect(150, 0, 255, 17);
        unDrawRowsFull(1, 1); // 单位行
        unDrawRowsFull(2, 5); // 结果行
        uidisp->flushRect(0, 39, 255, 56);
        uidisp->flushRect(0, 61, 255, 111);
        return 0;
    }
    if (key == KEY_UP) { if (uTop > 0) { uTop--; unDrawRowsFull(2, 5); uidisp->flushRect(0, 61, 255, 111); } return 0; }
    if (key == KEY_DOWN) { if (uTop + 4 < c->n) { uTop++; unDrawRowsFull(2, 5); uidisp->flushRect(0, 61, 255, 111); } return 0; }
    if (key == KEY_ENTER) { if (eAct) { uVal = atof(ebuf); eAct = 0; elen = 0; ebuf[0] = 0; return 1; } return 0; }
    if (key == KEY_F1) { eAct = 0; elen = 0; ebuf[0] = 0; uVal = 0; return 1; }
    if (key == KEY_ON || key == KEY_HOME || key == KEY_APPS) {
        if (eAct) { uVal = atof(ebuf); eAct = 0; elen = 0; ebuf[0] = 0; }
        fcLevel = (key == KEY_HOME) ? 0 : 1;
        fcMsg[0] = 0;
        if (fcLevel == 1) { fcTop = (uCat / 5) * 5; fcSel = uCat - fcTop; } // 回到当前类别行
        return 1;
    }
    if (key == KEY_VIEWS) return 0;
    return 0;
}
