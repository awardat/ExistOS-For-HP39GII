// Python app —— 独立 MicroPython（v1.29.0）终端式 REPL
// 2026-09-20 v2：修复数字键映射（HP39GII 数字键码不连续）、ANSI 转义（退格/擦除）、
// 闪烁光标、底部出界、Shift 指示、Shift+ON 退出、Shift+ALPHA 小写锁定、堆自适应
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdarg.h>
#include <stdint.h>
#define LCD_PIX_W 256
#define LCD_PIX_H 127
#define EXCLUDE_UI_LANGUAGE
#include "FreeRTOS.h"
#include "task.h"
#include "../../../core/SystemConfig.h"
#include "../../../third_party/freertos/include/SysConf.h"
#if FS_TYPE == FS_FATFS
    #include "filesystem/fatfs/ff.h"
#else
    #include "filesystem/littlefs/lfs.h"
#endif
#include "../../graphics/UICore.h"
#include "../../drivers/keyboard_gii39.h"

extern UI_Display *uidisp;
extern "C" {
uint32_t ll_vm_check_key();
void SystemUISuspend();
void SystemUIResume();
void ll_disp_set_indicator(int indicateBit, int BatInt);
uint32_t ll_get_time_ms();
void mpy_init(void *heap, size_t heap_size);
int mpy_repl_init(void);
int mpy_repl_feed_char(int c);
int mpy_exec_str(const char *src);
void mpy_gc_collect(void);
void mpy_deinit(void);
}

// ---- 终端缓冲（96 行环形回看）----
#define TERM_COLS 31
#define TERM_ROWS 6
#define TERM_LINES 96
#define ROW_Y0 13
#define ROW_STEP 16
#define TITLE_Y 0
#define HINT_Y 112 // 已由底栏取代（保留常量）

static char term[TERM_LINES][TERM_COLS + 1];
static int termLines = 0; // 已产生行数
static int tCol = 0;      // 当前列
static int termScroll = 0;
static volatile int termDirty = 1;
static int rowDirty = -1; // >=0：仅刷新该可见行
static int cursorOn = 1;
static int lineLen = 0;   // 当前输入行长度（用于退格边界）
static int contMode = 0;  // REPL 续行态（尾部提示符为 "... "）
static int inPs2Tail = 0; // 正处 "... " 之后（用于统计 auto-indent 空格数）
static int ps2Spaces = 0; // PS2 后自动缩进的空格数（已写进 REPL 行缓冲）
static char outTail[8];
static int outTailLen = 0;

static void trackPrompt(void) { // 尾部匹配 ">>> " / "... "；并统计缩进空格
    if (outTailLen >= 1) {
        if (outTailLen >= 4 && memcmp(outTail + outTailLen - 4, "... ", 4) == 0) {
            contMode = 1;
            inPs2Tail = 1;
            ps2Spaces = 0;
            return;
        }
        if (outTailLen >= 4 && memcmp(outTail + outTailLen - 4, ">>> ", 4) == 0) {
            contMode = 0;
            inPs2Tail = 0;
            ps2Spaces = 0;
            return;
        }
        if (inPs2Tail) {
            if (outTail[outTailLen - 1] == ' ') ps2Spaces++;
            else inPs2Tail = 0;
        }
    }
}

static int termCur() { return termLines ? (termLines - 1) % TERM_LINES : 0; }

static void termClearLineFrom(int idx, int col) {
    char *l = term[idx % TERM_LINES];
    for (int i = col; i <= TERM_COLS; i++) l[i] = 0;
}

static void termNewline() {
    rowDirty = -1;
    lineLen = 0;
    termDirty = 1; // 换行使可见行整体上移 → 需整屏重绘（普通按键不触发）
    termLines++;
    memset(term[termLines % TERM_LINES], 0, TERM_COLS + 1);
    tCol = 0;
    termScroll = 0; // 有新输出时回到最新
}

// ANSI 转义（readline 会发 ESC[K / ESC[D 等）
static int escState = 0, escN = 0, escHasN = 0;
static void termEscFinal(char c) {
    int n = escHasN ? escN : 1;
    if (n < 1) n = 1;
    switch (c) {
    case 'K': termClearLineFrom(termCur(), tCol); rowDirty = TERM_ROWS - 1; break; // 擦除行（0K）
    case 'D': tCol -= n; if (tCol < 0) tCol = 0; break;  // 左移
    case 'C': tCol += n; if (tCol > TERM_COLS) tCol = TERM_COLS; break;
    case 'A': break;                                     // 上移（忽略：行结构由 \n 管理）
    case 'B': break;
    case 'J':                                            // 清屏/清到末尾
        if (escHasN && escN == 2) for (int i = 0; i < TERM_LINES; i++) term[i][0] = 0;
        else { termClearLineFrom(termCur(), tCol); for (int i = 1; i < TERM_LINES; i++) { int idx = (termLines - 1 + i) % TERM_LINES; term[idx][0] = 0; } }
        break;
    case 'H': break;                                     // 光标定位（忽略）
    default: break;
    }
}

static void termPutc(char c) {
    if (escState == 1) {
        if (c == '[') { escState = 2; escN = 0; escHasN = 0; }
        else escState = 0;
        return;
    }
    if (escState == 2) {
        if (c >= '0' && c <= '9') { escN = escN * 10 + (c - '0'); escHasN = 1; return; }
        termEscFinal(c);
        escState = 0;
        return;
    }
    if (c == 0x1B) { escState = 1; return; }
    if (c == '\n') { termNewline(); return; }
    if (c == '\r') { tCol = 0; return; }
    if (c == 0x08) { if (tCol > 0) { tCol--; term[termCur()][tCol] = 0; rowDirty = TERM_ROWS - 1; } return; }
    if ((unsigned char)c < 0x20 && c != 0x09) return;
    char *l = term[termCur()];
    if (tCol >= TERM_COLS) { termNewline(); l = term[termCur()]; }
    if (tCol < TERM_COLS) {
        l[tCol++] = c;
        l[tCol] = 0;
        rowDirty = TERM_ROWS - 1; // 当前行即最底可见行
        if (outTailLen < (int)sizeof(outTail)) outTail[outTailLen++] = c;
        else {
            memmove(outTail, outTail + 1, sizeof(outTail) - 1);
            outTail[sizeof(outTail) - 1] = c;
        }
        trackPrompt();
    }
}

static void termPuts(const char *s) { while (*s) termPutc(*s++); }

// ---- 错误信息中文化（显示时替换；终端缓冲保持原始内容）----
typedef struct { const char *en; const char *zh; } errRule;
static const errRule errRules[] = {
    {"Traceback (most recent call last):", "\xbb\xd8\xcb\xdd\xa3\xa8\xd7\xee\xbd\xfc\xd2\xbb\xb4\xce\xb5\xf7\xd3\xc3\xa3\xa9:"},
    {"  File \"<stdin>\", line ", "  \xce\xc4\xbc\xfe \"<stdin>\"\xa3\xac\xb5\xda "},
    {"NameError: name '", "\xc3\xfb\xb3\xc6\xb4\xed\xce\xf3\xa3\xba\xc3\xfb\xb3\xc6 '"},
    {"' isn't defined", "' \xce\xb4\xb6\xa8\xd2\xe5"},
    {"SyntaxError: invalid syntax", "\xd3\xef\xb7\xa8\xb4\xed\xce\xf3\xa3\xba\xce\xde\xd0\xa7\xd3\xef\xb7\xa8"},
    {"SyntaxError: ", "\xd3\xef\xb7\xa8\xb4\xed\xce\xf3\xa3\xba "},
    {"IndentationError: ", "\xcb\xf5\xbd\xf8\xb4\xed\xce\xf3\xa3\xba "},
    {"TypeError: ", "\xc0\xe0\xd0\xcd\xb4\xed\xce\xf3\xa3\xba "},
    {"ValueError: ", "\xca\xfd\xd6\xb5\xb4\xed\xce\xf3\xa3\xba "},
    {"ZeroDivisionError: division by zero", "\xb3\xfd\xc1\xe3\xb4\xed\xce\xf3\xa3\xba\xb3\xfd\xd2\xd4\xc1\xe3"},
    {"ZeroDivisionError: ", "\xb3\xfd\xc1\xe3\xb4\xed\xce\xf3\xa3\xba "},
    {"IndexError: list index out of range", "\xcb\xf7\xd2\xfd\xb4\xed\xce\xf3\xa3\xba\xc1\xd0\xb1\xed\xcf\xc2\xb1\xea\xd4\xbd\xbd\xe7"},
    {"IndexError: ", "\xcb\xf7\xd2\xfd\xb4\xed\xce\xf3\xa3\xba "},
    {"KeyError: ", "\xbc\xfc\xb4\xed\xce\xf3\xa3\xba "},
    {"AttributeError: ", "\xca\xf4\xd0\xd4\xb4\xed\xce\xf3\xa3\xba "},
    {"ImportError: no module named ", "\xb5\xbc\xc8\xeb\xb4\xed\xce\xf3\xa3\xba\xc3\xbb\xd3\xd0\xc4\xa3\xbf\xe9 "},
    {"ImportError: ", "\xb5\xbc\xc8\xeb\xb4\xed\xce\xf3\xa3\xba "},
    {"NotImplementedError: only slices with step=1 (aka None) are supported", "\xce\xb4\xca\xb5\xcf\xd6\xb4\xed\xce\xf3\xa3\xba\xbd\xf6\xd6\xa7\xb3\xd6\xb2\xbd\xb3\xa4\xce\xaa 1 \xb5\xc4\xc7\xd0\xc6\xac"},
    {"NotImplementedError: ", "\xce\xb4\xca\xb5\xcf\xd6\xb4\xed\xce\xf3\xa3\xba "},
    {"MemoryError: ", "\xc4\xda\xb4\xe6\xb4\xed\xce\xf3\xa3\xba "},
    {"OverflowError: ", "\xd2\xe7\xb3\xf6\xb4\xed\xce\xf3\xa3\xba "},
    {"RuntimeError: ", "\xd4\xcb\xd0\xd0\xca\xb1\xb4\xed\xce\xf3\xa3\xba "},
    {"OSError: ", "\xcf\xb5\xcd\xb3\xb4\xed\xce\xf3\xa3\xba "},
    {"KeyboardInterrupt", "\xbc\xfc\xc5\xcc\xd6\xd0\xb6\xcf"},
    {"SystemExit", "\xcf\xb5\xcd\xb3\xcd\xcb\xb3\xf6"},
};
static char xlateBuf[160];
static const char *xlateLine(const char *src) {
    strncpy(xlateBuf, src, sizeof(xlateBuf) - 1);
    xlateBuf[sizeof(xlateBuf) - 1] = 0;
    for (unsigned r = 0; r < sizeof(errRules) / sizeof(errRules[0]); r++) {
        char *pos = strstr(xlateBuf, errRules[r].en);
        if (!pos) continue;
        static char tmp[200];
        int head = (int)(pos - xlateBuf);
        snprintf(tmp, sizeof(tmp), "%.*s%s%s", head, xlateBuf, errRules[r].zh, pos + strlen(errRules[r].en));
        strncpy(xlateBuf, tmp, sizeof(xlateBuf) - 1);
        xlateBuf[sizeof(xlateBuf) - 1] = 0;
    }
    return xlateBuf;
}

// ---- MicroPython HAL ----
extern "C" uint32_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) termPutc(str[i]); // 细粒度标记：普通字符→行刷新，换行→整屏
    return len;
}
// ---- FatFs 钩子（MicroPython open()/import 后端；M3）----
static void fsPath(const char *in, char *out, int n) { // 相对路径按 /xcas/ 解析
    if (in[0] == '/') snprintf(out, n, "%s", in);
    else snprintf(out, n, "/xcas/%s", in);
}

extern "C" int mpy_fs_stat(const char *path, int *isdir) {
    char p[64];
    fsPath(path, p, sizeof(p));
    FILINFO fno;
    if (f_stat(p, &fno) != FR_OK) return 0;
    *isdir = (fno.fattrib & AM_DIR) ? 1 : 0;
    return 1;
}

extern "C" void *mpy_fs_fopen(const char *path, const char *mode) {
    char p[64];
    fsPath(path, p, sizeof(p));
    BYTE flags = FA_READ;
    bool plus = strchr(mode, '+') != 0;
    switch (mode[0]) {
    case 'r': flags = plus ? (FA_READ | FA_WRITE) : FA_READ; break;
    case 'w': flags = FA_CREATE_ALWAYS | FA_WRITE | (plus ? FA_READ : 0); break;
    case 'a': flags = FA_OPEN_APPEND | FA_WRITE | (plus ? FA_READ : 0); break;
    default: flags = FA_READ; break;
    }
    FIL *f = (FIL *)malloc(sizeof(FIL));
    if (!f) return NULL;
    if (f_open(f, p, flags) != FR_OK) { free(f); return NULL; }
    return f;
}
extern "C" size_t mpy_fs_fread(void *h, void *buf, size_t n) {
    UINT br = 0;
    if (f_read((FIL *)h, buf, (UINT)n, &br) != FR_OK) return 0;
    return br;
}
extern "C" size_t mpy_fs_fwrite(void *h, const void *buf, size_t n) {
    UINT bw = 0;
    if (f_write((FIL *)h, buf, (UINT)n, &bw) != FR_OK) return 0;
    return bw;
}
extern "C" int mpy_fs_fclose(void *h) {
    FIL *f = (FIL *)h;
    FRESULT r = f_close(f);
    free(f);
    return r == FR_OK ? 0 : -1;
}
extern "C" long mpy_fs_fseek(void *h, long off, int whence) {
    FIL *f = (FIL *)h;
    FSIZE_t pos;
    if (whence == 0) pos = (FSIZE_t)off;
    else if (whence == 1) pos = f_tell(f) + off;
    else pos = f_size(f) + off;
    if (f_lseek(f, pos) != FR_OK) return -1;
    return (long)f_tell(f);
}
extern "C" long mpy_fs_ftell(void *h) { return (long)f_tell((FIL *)h); }

// ---- 脚本列表与运行（/xcas/*.py）----
#define PY_MAX_FILES 64
static char pyFiles[PY_MAX_FILES][28];
static int pyFileCount = 0, runSel = 0, runTop = 0;

static void scanPyFiles(void) {
    pyFileCount = 0;
    DIR dir;
    FILINFO fno;
    if (f_opendir(&dir, "/xcas") != FR_OK) return;
    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        if (fno.fattrib & AM_DIR) continue;
        int n = strlen(fno.fname);
        if (n > 3 && (fno.fname[n - 3] == '.') && (fno.fname[n - 2] == 'p' || fno.fname[n - 2] == 'P') && (fno.fname[n - 1] == 'y' || fno.fname[n - 1] == 'Y')) {
            strncpy(pyFiles[pyFileCount], fno.fname, 27);
            pyFiles[pyFileCount][27] = 0;
            if (++pyFileCount >= PY_MAX_FILES) break;
        }
    }
    f_closedir(&dir);
}

// 运行列表整区重绘（滚动时只刷列表区：y=13..109，不重绘 console 行）
static void drawRunList(void) {
    uidisp->draw_box(0, 13, LCD_PIX_W - 1, 109, -1, 255);
    uidisp->draw_printf(2, 14, 16, 0, 255, "运行脚本 (%d)", pyFileCount);
    for (int i = 0; i < 6; i++) {
        int idx = runTop + i;
        if (idx >= pyFileCount) break;
        int y = 36 + i * 12;
        if (idx == runSel) uidisp->draw_box(2, y - 1, LCD_PIX_W - 3, y + 10, -1, 0);
        uidisp->draw_printf(4, y, 12, (idx == runSel) ? 255 : 0, (idx == runSel) ? 0 : 255, "%s", pyFiles[idx]);
    }
    uidisp->flushRect(0, 13, LCD_PIX_W - 1, 109);
}

// 运行列表单行重绘（选中态）——避免每次移动整屏刷新
static void drawRunRow(int idx) {
    if (idx < runTop || idx >= runTop + 6 || idx >= pyFileCount) return;
    int y = 36 + (idx - runTop) * 12;
    uidisp->draw_box(2, y - 1, LCD_PIX_W - 3, y + 10, -1, (idx == runSel) ? 0 : 255);
    uidisp->draw_printf(4, y, 12, (idx == runSel) ? 255 : 0, (idx == runSel) ? 0 : 255, "%s", pyFiles[idx]);
    uidisp->flushRect(2, y - 1, LCD_PIX_W - 3, y + 10);
}

static void runPyFile(int idx) {
    char path[48];
    snprintf(path, sizeof(path), "/xcas/%s", pyFiles[idx]);
    FIL f;
    if (f_open(&f, path, FA_READ) != FR_OK) {
        termPuts("[open failed]");
        termNewline();
        return;
    }
    FSIZE_t sz = f_size(&f);
    if (sz > 64 * 1024) sz = 64 * 1024;
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        f_close(&f);
        termPuts("[no memory]");
        termNewline();
        return;
    }
    UINT br = 0;
    f_read(&f, buf, (UINT)sz, &br);
    buf[br] = 0;
    f_close(&f);
    termPuts("\r\n");
    termNewline();
    mpy_exec_str(buf);
    free(buf);
    termNewline();
}

// 保存终端会话到 /xcas/session.txt
static void saveSession(void) {
    FIL f;
    if (f_open(&f, "/xcas/session.txt", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        termPuts("\xb1\xa3\xb4\xe6\xca\xa7\xb0\xdc: session.txt");
        termNewline();
        return;
    }
    int first = termLines - TERM_LINES;
    if (first < 0) first = 0;
    for (int ln = first; ln < termLines; ln++) {
        const char *t = term[ln % TERM_LINES];
        if (!t[0]) continue;
        UINT bw = 0;
        f_write(&f, t, strlen(t), &bw);
        f_write(&f, "\r\n", 2, &bw);
    }
    f_close(&f);
    termPuts("\xd2\xd1\xb1\xa3\xb4\xe6 /xcas/session.txt");
    termNewline();
}

extern "C" uint32_t mp_hal_ticks_ms(void) { return (uint32_t)ll_get_time_ms(); }
extern "C" uint32_t mp_hal_ticks_us(void) { return (uint32_t)ll_get_time_ms() * 1000; }
extern "C" void mp_hal_delay_us(uint32_t us) { (void)us; }
extern "C" void mp_hal_delay_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
extern "C" uint32_t mp_hal_ticks_cpu(void) { return 0; }
extern "C" int mp_hal_stdin_rx_chr(void) { return 0; }
extern "C" void mp_hal_set_interrupt_char(int c) { (void)c; }

// ---- 键盘映射（数字键码不连续：显式映射；字母沿用 KhiCAS 布局）----
static int keyDigit(uint32_t key) {
    switch (key) {
    case KEY_0: return '0';
    case KEY_1: return '1';
    case KEY_2: return '2';
    case KEY_3: return '3';
    case KEY_4: return '4';
    case KEY_5: return '5';
    case KEY_6: return '6';
    case KEY_7: return '7';
    case KEY_8: return '8';
    case KEY_9: return '9';
    default: return 0;
    }
}

// alpha: 0=关 1=大写 2=小写；lock: Shift+ALPHA 锁定小写
static int keyToChar(uint32_t key, int shift, int alpha) {
    if (alpha) {
        switch (key) {
        case KEY_VARS: return (alpha == 2) ? 'a' : 'A';
        case KEY_MATH: return (alpha == 2) ? 'b' : 'B';
        case KEY_ABC: return (alpha == 2) ? 'c' : 'C';
        case KEY_XTPHIN: return (alpha == 2) ? 'd' : 'D';
        case KEY_SIN: return (alpha == 2) ? 'e' : 'E';
        case KEY_COS: return (alpha == 2) ? 'f' : 'F';
        case KEY_TAN: return (alpha == 2) ? 'g' : 'G';
        case KEY_LN: return (alpha == 2) ? 'h' : 'H';
        case KEY_LOG: return (alpha == 2) ? 'i' : 'I';
        case KEY_X2: return (alpha == 2) ? 'j' : 'J';
        case KEY_XY: return (alpha == 2) ? 'k' : 'K';
        case KEY_LEFTBRACKET: return (alpha == 2) ? 'l' : 'L';
        case KEY_RIGHTBRACKET: return (alpha == 2) ? 'm' : 'M';
        case KEY_DIVISION: return (alpha == 2) ? 'n' : 'N';
        case KEY_COMMA: return (alpha == 2) ? 'o' : 'O';
        case KEY_7: return (alpha == 2) ? 'p' : 'P';
        case KEY_8: return (alpha == 2) ? 'q' : 'Q';
        case KEY_9: return (alpha == 2) ? 'r' : 'R';
        case KEY_MULTIPLICATION: return (alpha == 2) ? 's' : 'S';
        case KEY_4: return (alpha == 2) ? 't' : 'T';
        case KEY_5: return (alpha == 2) ? 'u' : 'U';
        case KEY_6: return (alpha == 2) ? 'v' : 'V';
        case KEY_SUBTRACTION: return (alpha == 2) ? 'w' : 'W';
        case KEY_1: return (alpha == 2) ? 'x' : 'X';
        case KEY_2: return (alpha == 2) ? 'y' : 'Y';
        case KEY_3: return (alpha == 2) ? 'z' : 'Z';
        case KEY_PLUS: return ' ';
        case KEY_DOT: return ':';
        case KEY_0: return '"';
        case KEY_NEGATIVE: return ';';
        default: return 0;
        }
    }
    if (shift) {
        switch (key) {
        case KEY_5: return '[';
        case KEY_6: return ']';
        case KEY_8: return '{';
        case KEY_9: return '}';
        case KEY_3: return '#';
        case KEY_0: return '"';
        case KEY_DOT: return '=';
        case KEY_MULTIPLICATION: return '!';
        case KEY_NEGATIVE: return '_';
        default: break;
        }
    }
    int d = keyDigit(key);
    if (d) return d;
    switch (key) {
    case KEY_DOT: return '.';
    case KEY_PLUS: return '+';
    case KEY_SUBTRACTION: return '-';
    case KEY_MULTIPLICATION: return '*';
    case KEY_DIVISION: return '/';
    case KEY_LEFTBRACKET: return '(';
    case KEY_RIGHTBRACKET: return ')';
    case KEY_COMMA: return ',';
    case KEY_NEGATIVE: return '-';
    default: return 0;
    }
}

// ---- 主任务状态（draw 需读取堆大小）----
static volatile int pyRunning = 0;
static void *pyHeap = NULL;
static size_t pyHeapSize = 0;
static int pyInited = 0;

// ---- 菜单系统（v4：F1 符号面板 / F5 帮助 / F6 文件）----
#define UI_REPL 0
#define UI_SYMB 1
#define UI_HELP 2
#define UI_FILE 3
#define UI_RUN 4
static int uiMode = UI_REPL;
static int symPage = 0, symSel = 0, helpPage = 0, fileSel = 0;

#define SYM_PAGES 4
static const char *symItems[SYM_PAGES][10] = {
    {":", ",", ".", "=", "<", ">", "_", "#", "\"", "'"},
    {"==", "!=", "<=", ">=", "+", "-", "*", "/", "%", "**"},
    {"(", ")", "[", "]", "{", "}", "//", "+=", "-=", "="},
    {"if ", "elif ", "else:", "for ", "while ", "def ", "return ", "print(", "import ", "in "},
};
static const char *symTitles[SYM_PAGES] = {"\xb7\xfb\xba\xc5", "\xd4\xcb\xcb\xe3\xb7\xfb", "\xc0\xa8\xba\xc5\xd3\xeb\xb8\xb3\xd6\xb5", "\xbd\xe1\xb9\xb9"};
static const char *helpText[4][6] = {
    {"\xb0\xef\xd6\xfa 1/4 \xbb\xf9\xb1\xbe\xb2\xd9\xd7\xf7", "\xc6\xd5\xcd\xa8\xbc\xfc\xa3\xba\xca\xfd\xd7\xd6\xd3\xeb\xd4\xcb\xcb\xe3\xb7\xfb", "ALPHA\xa3\xba\xd7\xd6\xc4\xb8\xa3\xa8\xd2\xbb\xb4\xce\xb4\xf3\xd0\xb4\xa3\xac\xc1\xbd\xb4\xce\xd0\xa1\xd0\xb4\xa3\xa9", "Shift+ALPHA\xa3\xba\xcb\xf8\xb6\xa8\xd0\xa1\xd0\xb4", "ENT\xa3\xba\xd6\xb4\xd0\xd0\xa3\xbb\xbf\xe9\xce\xb4\xbd\xe1\xca\xf8\xd7\xd4\xb6\xaf\xd0\xf8\xd0\xd0", "Shift+ON\xa3\xba\xcd\xcb\xb3\xf6"},
    {"\xb0\xef\xd6\xfa 2/4 \xb1\xe0\xbc\xad\xd3\xeb\xb9\xf6\xb6\xaf", "\xcd\xcb\xb8\xf1\xa3\xba\xc9\xbe\xb3\xfd\xd7\xd6\xb7\xfb", "Shift+\xcd\xcb\xb8\xf1\xa3\xba\xc8\xa1\xcf\xfb\xb5\xb1\xc7\xb0\xca\xe4\xc8\xeb", "UP/DOWN\xa3\xba\xd6\xf0\xd0\xd0\xb9\xf6\xb6\xaf", "Shift+UP/DOWN\xa3\xba\xb7\xad\xd2\xb3", "ON\xa3\xba\xc7\xe5\xc6\xc1"},
    {"\xb0\xef\xd6\xfa 3/4 \xb6\xe0\xd0\xd0\xd3\xef\xbe\xe4", "\xc0\xfd\xa3\xba""for i in range(3):", "\xcf\xc2\xd2\xbb\xd0\xd0\xd6\xb1\xbd\xd3\xca\xe4\xc8\xeb\xa3\xa8\xd7\xd4\xb6\xaf\xcb\xf5\xbd\xf8\xa3\xa9", "\xcc\xe5\xd0\xd0\xca\xe4\xc8\xeb\xcd\xea\xba\xf3\xb0\xb4 ENT", "\xd4\xd9\xb0\xb4\xd2\xbb\xb4\xce ENT\xa3\xa8\xbf\xd5\xd0\xd0\xa3\xa9\xbf\xaa\xca\xbc\xd6\xb4\xd0\xd0", "F1\xa3\xba\xb7\xfb\xba\xc5\xc3\xe6\xb0\xe5"},
    {"\xb0\xef\xd6\xfa 4/4 \xb9\xd8\xd3\xda", "MicroPython 1.29 \xb6\xc0\xc1\xa2\xd3\xa6\xd3\xc3", "\xcf\xd4\xca\xbe\xbf\xed\xb6\xc8 31 \xd7\xd6\xb7\xfb x 8 \xd0\xd0", "\xca\xe4\xb3\xf6\xb1\xa3\xc1\xf4\xd7\xee\xbd\xfc 96 \xd0\xd0", "\xbb\xe1\xbb\xb0\xb1\xe4\xc1\xbf\xd4\xda\xcd\xcb\xb3\xf6\xba\xf3\xb1\xa3\xc1\xf4", "\xb8\xb4\xce\xbb\xbd\xe2\xca\xcd\xc6\xf7\xa3\xba""F6 \xce\xc4\xbc\xfe\xb2\xcb\xb5\xa5"},
};
static const char *fileItems[6] = {"\xb4\xf2\xbf\xaa\xb2\xa2\xd4\xcb\xd0\xd0", "\xb1\xa3\xb4\xe6\xbb\xe1\xbb\xb0", "\xc7\xe5\xc6\xc1", "\xb8\xb4\xce\xbb\xbd\xe2\xca\xcd\xc6\xf7", "\xb9\xd8\xd3\xda", "\xcd\xcb\xb3\xf6"};
static const char *barLabels[6] = {"\xb7\xfb\xba\xc5", "\xc7\xe5\xc6\xc1", "\xc8\xa1\xcf\xfb", "\xd4\xcb\xd0\xd0", "\xb0\xef\xd6\xfa", "\xce\xc4\xbc\xfe"};
static const char *barLabelsSymb[6] = {"\xd1\xa1\xd4\xf1", "\xc8\xa1\xcf\xfb", "\xc9\xcf\xb7\xad", "\xcf\xc2\xb7\xad", "", ""};

static void feedStr(const char *str) {
    while (*str) {
        mpy_repl_feed_char((unsigned char)*str);
        lineLen++;
        str++;
    }
    inPs2Tail = 0;
}

// ---- 绘制 ----
// 光标：当前单元格下划线（8x2px，紧贴上一字符，无空档）
static void cursorCell(int *px, int *py) {
    *px = -1;
    if (termScroll != 0 || termLines < 1) return;
    *px = 1 + tCol * 8;
    *py = ROW_Y0 + (TERM_ROWS - 1) * ROW_STEP + 14;
}
static void cursorPaint(int on) {
    int x, y;
    cursorCell(&x, &y);
    if (x < 0) return;
    uidisp->draw_box(x, y, x + 7, y + 1, on ? 0 : 255, -1);
    uidisp->flushRect(x, y, x + 7, y + 1);
}


static void draw() {
    uidisp->draw_box(0, 0, LCD_PIX_W - 1, LCD_PIX_H - 1, 255, 255);
    uidisp->draw_printf(2, TITLE_Y, 12, 0, 255, "Python 3.4 / MicroPython 1.29  [%dK]", (int)(pyHeapSize / 1024));
    int last = termLines - 1 - termScroll;
    for (int i = 0; i < TERM_ROWS; i++) {
        int ln = last - (TERM_ROWS - 1 - i);
        if (ln < 0) continue;
        uidisp->draw_printf(1, ROW_Y0 + i * ROW_STEP, 16, 0, 255, "%s", xlateLine(term[ln % TERM_LINES]));
    }
    if (cursorOn) {
        int x, y;
        cursorCell(&x, &y);
        if (x >= 0) uidisp->draw_box(x, y, x + 7, y + 1, 0, -1);
    }
    // 菜单覆盖层
    if (uiMode == UI_SYMB) {
        uidisp->draw_box(0, 13, LCD_PIX_W - 1, 109, -1, 255);
        uidisp->draw_printf(2, 14, 16, 0, 255, "%s %d/%d", symTitles[symPage], symPage + 1, SYM_PAGES);
        for (int i = 0; i < 10; i++) {
            int col = i % 5, row = i / 5;
            int x = 6 + col * 50, y = 42 + row * 24;
            if (i == symSel) uidisp->draw_box(x - 4, y - 3, x + 42, y + 15, -1, 0);
            uidisp->draw_printf(x, y, 12, (i == symSel) ? 255 : 0, (i == symSel) ? 0 : 255, "%s", symItems[symPage][i]);
        }
    } else if (uiMode == UI_HELP) {
        uidisp->draw_box(0, 13, LCD_PIX_W - 1, 109, -1, 255);
        for (int i = 0; i < 6; i++)
            if (helpText[helpPage][i][0]) uidisp->draw_printf(2, 15 + i * 16, 16, 0, 255, "%s", helpText[helpPage][i]);
    } else if (uiMode == UI_RUN) {
        uidisp->draw_box(0, 13, LCD_PIX_W - 1, 109, -1, 255);
        uidisp->draw_printf(2, 14, 16, 0, 255, "\xd4\xcb\xd0\xd0\xbd\xc5\xb1\xbe (%d)", pyFileCount);
        for (int i = 0; i < 6; i++) {
            int idx = runTop + i;
            if (idx >= pyFileCount) break;
            int y = 36 + i * 12;
            if (idx == runSel) uidisp->draw_box(2, y - 1, LCD_PIX_W - 3, y + 10, -1, 0);
            uidisp->draw_printf(4, y, 12, (idx == runSel) ? 255 : 0, (idx == runSel) ? 0 : 255, "%s", pyFiles[idx]);
        }
    } else if (uiMode == UI_FILE) {
        uidisp->draw_box(0, 13, LCD_PIX_W - 1, 109, -1, 255);
        uidisp->draw_printf(2, 14, 16, 0, 255, "\xce\xc4\xbc\xfe\xb2\xcb\xb5\xa5");
        for (int i = 0; i < 6; i++) {
            int y = 22 + i * 14;
            if (i == fileSel) uidisp->draw_box(2, y - 2, LCD_PIX_W - 3, y + 14, -1, 0);
            uidisp->draw_printf(6, y, 16, (i == fileSel) ? 255 : 0, (i == fileSel) ? 0 : 255, "%s", fileItems[i]);
        }
    }
    // 底栏（6 段；符号面板内替换为 选择/取消/上翻/下翻）
    const char **bar = (uiMode == UI_SYMB) ? barLabelsSymb : barLabels;
    uidisp->draw_box(0, 110, LCD_PIX_W - 1, LCD_PIX_H - 1, -1, 0);
    for (int i = 0; i < 6; i++) {
        if (!bar[i][0]) continue;
        uint8_t barFg = (uiMode == i + 1) ? 0 : 255;
        uint8_t barBg = (uiMode == i + 1) ? 255 : 0;
        uidisp->draw_printf(i * 42 + 5, 111, 16, barFg, barBg, "%s", bar[i]);
    }
    uidisp->flush();
}

// ---- 主任务 ----
static void pyTask(void *_) {
    SystemUISuspend();
    uidisp->restoreBuffer();
    pyRunning = 1;

    if (!pyHeap) { // 自适应堆：优先片上，依次尝试
        static const size_t tries[] = {96 * 1024, 64 * 1024, 32 * 1024};
        for (unsigned i = 0; i < sizeof(tries) / sizeof(tries[0]); i++) {
            pyHeap = malloc(tries[i]);
            if (pyHeap) { pyHeapSize = tries[i]; break; }
        }
    }
    if (!pyHeap) {
        uidisp->draw_box(0, 0, 255, 127, 255, 255);
        uidisp->draw_printf(2, 40, 12, 0, 255, "Python: no memory (need 32KB+)");
        uidisp->draw_printf(2, 60, 12, 0, 255, "Enable MEM SWAP in settings");
        uidisp->flush();
        vTaskDelay(pdMS_TO_TICKS(3000));
        SystemUIResume();
        vTaskDelete(NULL);
        return;
    }
    if (!pyInited) {
        mpy_init(pyHeap, pyHeapSize);
        mpy_repl_init();
        pyInited = 1;
    }
    termDirty = 1;
    draw();

    int lastKey = -1, shift = 0, alpha = 0, alphaLock = 0, blinkDiv = 0;
    while (pyRunning) {
        uint32_t keys = ll_vm_check_key();
        uint32_t kp = keys >> 16, key = keys & 0xFFFF;
        if (kp) {
            if (key == KEY_SHIFT) {
                if (key != (uint32_t)lastKey) {
                    lastKey = key;
                    shift = 1;
                    ll_disp_set_indicator(INDICATE_LEFT, -1);
                }
            } else if (key != (uint32_t)lastKey) {
                lastKey = key;
                if (uiMode == UI_SYMB) {
                    if (key == KEY_F1 || key == KEY_ENTER) { feedStr(symItems[symPage][symSel]); uiMode = UI_REPL; } // 选择
                    else if (key == KEY_F2 || key == KEY_ON) uiMode = UI_REPL;                                        // 取消
                    else if (key == KEY_F3) { symPage = (symPage + SYM_PAGES - 1) % SYM_PAGES; symSel = 0; }                              // 上翻
                    else if (key == KEY_F4) { symPage = (symPage + 1) % SYM_PAGES; symSel = 0; }                              // 下翻
                    else if (key == KEY_UP) { if (symSel >= 5) symSel -= 5; }
                    else if (key == KEY_DOWN) { if (symSel < 5) symSel += 5; }
                    else if (key == KEY_LEFT) { if ((symSel % 5) > 0) symSel--; }
                    else if (key == KEY_RIGHT) { if ((symSel % 5) < 4) symSel++; }
                    termDirty = 1;
                } else if (uiMode == UI_HELP) {
                    if (key == KEY_F5 || key == KEY_ON || key == KEY_ENTER) uiMode = UI_REPL;
                    else if (key == KEY_LEFT) helpPage = (helpPage + 3) % 4;
                    else if (key == KEY_RIGHT) helpPage = (helpPage + 1) % 4;
                    termDirty = 1;
                } else if (uiMode == UI_FILE) {
                    if (key == KEY_F6 || key == KEY_ON) uiMode = UI_REPL;
                    else if (key == KEY_UP) { if (fileSel > 0) fileSel--; }
                    else if (key == KEY_DOWN) { if (fileSel < 5) fileSel++; }
                    else if (key == KEY_ENTER) {
                        if (fileSel == 0) { scanPyFiles(); runSel = 0; runTop = 0; uiMode = (pyFileCount > 0) ? UI_RUN : UI_REPL; } // 打开并运行
                        else if (fileSel == 1) { saveSession(); uiMode = UI_REPL; }                                                                                       // 保存会话
                        else if (fileSel == 2) { for (int i = 0; i < TERM_LINES; i++) term[i][0] = 0; termLines = 0; tCol = 0; termScroll = 0; lineLen = 0; uiMode = UI_REPL; } // 清屏
                        else if (fileSel == 3) { mpy_deinit(); mpy_init(pyHeap, pyHeapSize); mpy_repl_init(); contMode = 0; uiMode = UI_REPL; }                            // 复位解释器
                        else if (fileSel == 4) { uiMode = UI_HELP; helpPage = 3; }                                                                                        // 关于 → 帮助
                        else { pyRunning = 0; }                                                                                                                           // 退出
                    }
                    termDirty = 1;
                } else if (uiMode == UI_RUN) {
                    if (key == KEY_F4 || key == KEY_ON) uiMode = UI_REPL;
                    else if (key == KEY_UP) {
                        if (runSel > 0) {
                            int old = runSel;
                            runSel--;
                            if (runSel < runTop) { runTop = runSel; drawRunList(); } // 滚动只刷列表区
                            else { drawRunRow(old); drawRunRow(runSel); }
                        }
                    } else if (key == KEY_DOWN) {
                        if (runSel + 1 < pyFileCount) {
                            int old = runSel;
                            runSel++;
                            if (runSel >= runTop + 6) { runTop = runSel - 5; drawRunList(); }
                            else { drawRunRow(old); drawRunRow(runSel); }
                        }
                    } else if (key == KEY_ENTER) { runPyFile(runSel); uiMode = UI_REPL; termDirty = 1; }
                    else termDirty = 1;
                } else if (key == KEY_F1) { uiMode = UI_SYMB; symSel = 0; termDirty = 1;
                } else if (key == KEY_F4) { scanPyFiles(); runSel = 0; runTop = 0; if (pyFileCount > 0) uiMode = UI_RUN; termDirty = 1;
                } else if (key == KEY_F5) { uiMode = UI_HELP; helpPage = 0; termDirty = 1;
                } else if (key == KEY_F6) { uiMode = UI_FILE; fileSel = 0; termDirty = 1;
                } else if (key == KEY_F2) { // 清屏
                    for (int i = 0; i < TERM_LINES; i++) term[i][0] = 0;
                    termLines = 0; tCol = 0; termScroll = 0; lineLen = 0; termDirty = 1;
                } else if (key == KEY_F3) { // 取消输入
                    mpy_repl_feed_char(0x03); contMode = 0; lineLen = 0;
                } else if (shift && key == KEY_ON) { // Shift+ON 退出
                    pyRunning = 0;
                } else if (shift && key == KEY_ALPHA) { // Shift+ALPHA：锁定小写（再按解除）
                    shift = 0; // 先消费 shift，避免其后的"清理 shift"逻辑把锁定图标熄灭
                    if (alphaLock) { alphaLock = 0; alpha = 0; ll_disp_set_indicator(0, -1); }
                    else { alphaLock = 1; alpha = 2; ll_disp_set_indicator(INDICATE_a__z, -1); }
                } else if (key == KEY_ON) {
                    // 单独 ON：清屏（终端清屏，不退出）
                    for (int i = 0; i < TERM_LINES; i++) term[i][0] = 0;
                    termLines = 0;
                    tCol = 0;
                    termScroll = 0;
                    lineLen = 0;
                    termDirty = 1;
                } else if (key == KEY_ALPHA) {
                    if (alphaLock) { alphaLock = 0; alpha = 0; ll_disp_set_indicator(0, -1); }
                    else {
                        alpha = (alpha + 1) % 3; // 大写→小写→关
                        ll_disp_set_indicator(alpha == 1 ? INDICATE_A__Z : (alpha == 2 ? INDICATE_a__z : 0), -1);
                    }
                } else if (key == KEY_ENTER) {
                    if (lineLen == 0 && contMode) {
                        // 空行结束块：MP auto-indent 已把空格写进缓冲 → 先退格清掉，再回车（缓冲末尾成为 '\n' 才会执行）
                        for (int i = 0; i < ps2Spaces; i++) mpy_repl_feed_char(0x08);
                        ps2Spaces = 0;
                        mpy_repl_feed_char('\r');
                    } else {
                        mpy_repl_feed_char('\r');
                    }
                    lineLen = 0;
                } else if (shift && key == KEY_BACKSPACE) { // Shift+退格 = Ctrl-C：取消当前输入（续行卡住的逃生口）
                    mpy_repl_feed_char(0x03);
                    contMode = 0;
                    lineLen = 0;
                } else if (key == KEY_BACKSPACE) {
                    if (lineLen > 0) { // 行首忽略：MP readline 在空行退格会重打提示符
                        mpy_repl_feed_char(0x08);
                        lineLen--;
                    }
                } else if (shift && key == KEY_UP) { // Shift+UP = PageUp（整页回看）
                    termScroll += TERM_ROWS;
                    if (termScroll > termLines - TERM_ROWS) termScroll = termLines - TERM_ROWS;
                    if (termScroll < 0) termScroll = 0;
                    termDirty = 1;
                } else if (shift && key == KEY_DOWN) { // Shift+DOWN = PageDown
                    termScroll -= TERM_ROWS;
                    if (termScroll < 0) termScroll = 0;
                    termDirty = 1;
                } else if (key == KEY_LEFT) { // 光标左移（REPL 行内编辑）
                    if (lineLen > 0) {
                        mpy_repl_feed_char(0x1B);
                        mpy_repl_feed_char('[');
                        mpy_repl_feed_char('D');
                    }
                } else if (key == KEY_RIGHT) { // 光标右移
                    mpy_repl_feed_char(0x1B);
                    mpy_repl_feed_char('[');
                    mpy_repl_feed_char('C');
                } else if (key == KEY_UP) {
                    if (termLines > TERM_ROWS) termScroll++;
                    if (termScroll > termLines - TERM_ROWS) termScroll = termLines - TERM_ROWS;
                    termDirty = 1;
                } else if (key == KEY_DOWN) {
                    if (termScroll > 0) termScroll--;
                    termDirty = 1;
                } else {
                    int ch = keyToChar(key, shift, alpha);
                    if (ch) {
                        mpy_repl_feed_char(ch);
                        lineLen++;
                        inPs2Tail = 0;
                        if (alpha && !alphaLock) { // 一次性 alpha
                            alpha = 0;
                            ll_disp_set_indicator(0, -1);
                        }
                    }
                }
                if (shift && key != KEY_SHIFT && key != KEY_ALPHA) {
                    shift = 0;
                    ll_disp_set_indicator(0, -1);
                }
                cursorOn = 1; // 有输入时点亮光标；重绘交由输出（rowDirty）或换行（termDirty）
            }
        } else {
            lastKey = -1;
        }
        blinkDiv++;
        if (blinkDiv >= 25) { // ~500ms 光标闪烁（仅刷新光标 2px 竖条，不整屏重绘）
            blinkDiv = 0;
            cursorOn = !cursorOn;
            if (!termDirty && uiMode == UI_REPL) cursorPaint(cursorOn);
        }
        if (termDirty) {
            termDirty = 0;
            rowDirty = -1;
            draw();
        } else if (rowDirty >= 0) { // 仅重绘最底行（输入回显：避免整屏刷新）
            int ln = termLines - 1;
            int ly = ROW_Y0 + rowDirty * ROW_STEP;
            uidisp->draw_box(1, ly, LCD_PIX_W - 2, ly + 15, 255, 255);
            if (ln >= 0) uidisp->draw_printf(1, ly, 16, 0, 255, "%s", xlateLine(term[ln % TERM_LINES]));
            if (cursorOn) {
                int x, y;
                cursorCell(&x, &y);
                if (x >= 0) uidisp->draw_box(x, y, x + 7, y + 1, 0, -1);
            }
            uidisp->flushRect(0, ly, LCD_PIX_W - 1, ly + 15);
            rowDirty = -1;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ll_disp_set_indicator(0, -1);
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    uidisp->flush();
    SystemUIResume();
    vTaskDelete(NULL);
}

extern "C" void StartPython() {
    xTaskCreate(pyTask, "Python", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
}
