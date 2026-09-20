// Python app —— 独立 MicroPython（v1.29.0）终端式 REPL
// 2026-09-20 M1/M2：事件驱动 REPL + 键盘/显示 HAL + 4 行回看
// 方案见 docs/Python-app-plan.md（本地）
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
#include "../../graphics/UICore.h"
#include "../../drivers/keyboard_gii39.h"

extern UI_Display *uidisp;
extern "C" {
uint32_t ll_vm_check_key();
void SystemUISuspend();
void SystemUIResume();
void ll_disp_set_indicator(int indicateBit, int BatInt);
uint32_t ll_get_time_ms();
// MicroPython 端口 API（Libs/src/micropython/ports/eoslib/mpy_port.c）
void mpy_init(void *heap, size_t heap_size);
int mpy_repl_init(void);
int mpy_repl_feed_char(int c);
int mpy_exec_str(const char *src);
void mpy_gc_collect(void);
}

// ---- 终端缓冲（96 行回看，环形）----
#define TERM_COLS 31
#define TERM_ROWS 8
#define TERM_LINES 96
#define PY_HEAP_SIZE (256 * 1024)

static char term[TERM_LINES][TERM_COLS + 1];
static int termLines = 0;   // 已产生行数（总）
static int termCol = 0;     // 当前行光标列
static int termScroll = 0;  // 回看偏移（0=最新）
static volatile int termDirty = 1;

static int termCur() { return termLines ? (termLines - 1) % TERM_LINES : 0; }

static void termNewline() {
    termLines++;
    int idx = termLines % TERM_LINES;
    memset(term[idx], 0, sizeof(term[idx]));
    termCol = 0;
    if (termLines >= TERM_LINES) termScroll = 0;
}

static void termPutc(char c) {
    if (c == '\n') { termNewline(); return; }
    if (c == '\r') { termCol = 0; return; }
    if (c == 0x08) { // 退格
        char *l = term[termCur()];
        if (termCol > 0) { termCol--; l[termCol] = 0; }
        return;
    }
    if ((unsigned char)c < 0x20 && c != 0x09) return; // 控制字符忽略（除 TAB）
    char *l = term[termCur()];
    if (termCol >= TERM_COLS) termNewline();
    l = term[termCur()];
    l[termCol++] = c;
    l[termCol] = 0;
}

static void termPuts(const char *s) {
    while (*s) termPutc(*s++);
}

// ---- MicroPython HAL（强符号覆盖端口的弱定义）----
extern "C" uint32_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) termPutc(str[i]);
    termDirty = 1;
    return len;
}
extern "C" uint32_t mp_hal_ticks_ms(void) { return (uint32_t)ll_get_time_ms(); }
extern "C" uint32_t mp_hal_ticks_us(void) { return (uint32_t)ll_get_time_ms() * 1000; }
extern "C" void mp_hal_delay_us(uint32_t us) { (void)us; }
extern "C" void mp_hal_delay_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); } // time.sleep()
extern "C" uint32_t mp_hal_ticks_cpu(void) { return 0; }
extern "C" int mp_hal_stdin_rx_chr(void) { return 0; }
extern "C" void mp_hal_set_interrupt_char(int c) { (void)c; }

// ---- 键盘映射（HP39GII 无独立字母键；沿用 KhiCAS 键盘表）----
// alpha: 0=关 1=小写 2=大写
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
    if (shift) { // Shift 层：Python 常用符号
        switch (key) {
        case KEY_5: return '[';
        case KEY_6: return ']';
        case KEY_8: return '{';
        case KEY_9: return '}';
        case KEY_3: return '#'; // 注释（KhiCAS 该位为 π，此处按 Python 用途）
        case KEY_0: return '"';
        case KEY_DOT: return '=';
        case KEY_MULTIPLICATION: return '!';
        case KEY_NEGATIVE: return '_';
        default: break;
        }
    }
    switch (key) {
    case KEY_0: case KEY_1: case KEY_2: case KEY_3: case KEY_4:
    case KEY_5: case KEY_6: case KEY_7: case KEY_8: case KEY_9:
        return '0' + (int)(key - KEY_0);
    case KEY_DOT: return '.';
    case KEY_PLUS: return '+';
    case KEY_SUBTRACTION: return '-';
    case KEY_MULTIPLICATION: return '*';
    case KEY_DIVISION: return '/';
    case KEY_LEFTBRACKET: return '(';
    case KEY_RIGHTBRACKET: return ')';
    case KEY_COMMA: return ',';
    case KEY_NEGATIVE: return '-';
    default: break;
    }
    return 0;
}

// ---- 绘制 ----
static void draw() {
    uidisp->draw_box(0, 0, LCD_PIX_W - 1, LCD_PIX_H - 1, 255, 255);
    uidisp->draw_printf(2, 1, 12, 0, 255, "Python 3.4 / MicroPython 1.29");
    // 文本区（8 行 12px 字体，y=15 起，行距 13）
    int last = termLines - 1 - termScroll;
    for (int i = 0; i < TERM_ROWS; i++) {
        int ln = last - (TERM_ROWS - 1 - i);
        if (ln < 0) continue;
        uidisp->draw_printf(1, 15 + i * 13, 12, 0, 255, "%s", term[ln % TERM_LINES]);
    }
    uidisp->draw_printf(2, 119, 12, 0, 255, "ON exit  ALPHA a-z  UP/DN scroll");
    uidisp->flush();
}

// ---- 主任务 ----
static volatile int pyRunning = 0;
static void *pyHeap = NULL;
static int pyInited = 0;

static void pyTask(void *_) {
    SystemUISuspend();
    uidisp->restoreBuffer();
    pyRunning = 1;

    if (!pyHeap) pyHeap = malloc(PY_HEAP_SIZE);
    if (!pyHeap) {
        uidisp->draw_box(0, 0, 255, 127, 255, 255);
        uidisp->draw_printf(2, 40, 12, 0, 255, "Python: not enough memory!");
        uidisp->draw_printf(2, 60, 12, 0, 255, "need %d KB", PY_HEAP_SIZE / 1024);
        uidisp->flush();
        vTaskDelay(pdMS_TO_TICKS(3000));
        SystemUIResume();
        vTaskDelete(NULL);
        return;
    }
    if (!pyInited) { // 首次进入：初始化（会话在退出后保留，变量不丢）
        mpy_init(pyHeap, PY_HEAP_SIZE);
        mpy_repl_init();
        pyInited = 1;
    }
    termDirty = 1;
    draw();

    int lastKey = -1, shift = 0, alpha = 0;
    while (pyRunning) {
        uint32_t keys = ll_vm_check_key();
        uint32_t kp = keys >> 16, key = keys & 0xFFFF;
        if (kp) {
            if (key == KEY_SHIFT) { // Shift 按下沿
                lastKey = key;
                if (alpha) { // Shift+ALPHA：切换大小写
                    alpha = (alpha == 1) ? 2 : 1;
                    ll_disp_set_indicator(alpha == 1 ? INDICATE_A__Z : INDICATE_a__z, -1);
                } else {
                    shift = 1;
                }
            } else if (key != (uint32_t)lastKey) {
                lastKey = key;
                if (key == KEY_ON) {
                    pyRunning = 0; // 退出
                } else if (key == KEY_ALPHA) {
                    alpha = (alpha + 1) % 3; // 大写(A..Z)→小写(a..z)→关（一次有效）
                    ll_disp_set_indicator(alpha == 1 ? INDICATE_A__Z : (alpha == 2 ? INDICATE_a__z : 0), -1);
                } else if (key == KEY_ENTER) {
                    mpy_repl_feed_char('\r');
                } else if (key == KEY_BACKSPACE) {
                    mpy_repl_feed_char(0x08);
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
                        if (alpha) { // 一次性 alpha：用后关闭
                            alpha = 0;
                            ll_disp_set_indicator(0, -1);
                        }
                    }
                }
                if (key != KEY_SHIFT) shift = 0;
            }
        } else {
            lastKey = -1;
            shift = 0;
        }
        if (termDirty) { termDirty = 0; draw(); }
        vTaskDelay(pdMS_TO_TICKS(10));
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
