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
}

// ---- 终端缓冲（96 行环形回看）----
#define TERM_COLS 31
#define TERM_ROWS 8
#define TERM_LINES 96
#define ROW_Y0 13
#define ROW_STEP 12
#define TITLE_Y 0
#define HINT_Y 112

static char term[TERM_LINES][TERM_COLS + 1];
static int termLines = 0; // 已产生行数
static int tCol = 0;      // 当前列
static int termScroll = 0;
static volatile int termDirty = 1;
static int rowDirty = -1; // >=0：仅刷新该可见行
static int cursorOn = 1;
static int lineLen = 0;   // 当前输入行长度（用于退格边界）
static int contMode = 0;  // REPL 续行态（尾部提示符为 "... "）
static char outTail[8];
static int outTailLen = 0;

static void trackPrompt(void) { // 尾部匹配 ">>> " / "... "
    if (outTailLen >= 4) {
        if (memcmp(outTail + outTailLen - 4, "... ", 4) == 0) contMode = 1;
        else if (memcmp(outTail + outTailLen - 4, ">>> ", 4) == 0) contMode = 0;
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

// ---- MicroPython HAL ----
extern "C" uint32_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) termPutc(str[i]);
    termDirty = 1;
    return len;
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

// ---- 绘制 ----
// 光标：当前单元格下划线（8x2px，紧贴上一字符，无空档）
static void cursorCell(int *px, int *py) {
    *px = -1;
    if (termScroll != 0 || termLines < 1) return;
    *px = 1 + tCol * 8;
    *py = ROW_Y0 + (TERM_ROWS - 1) * ROW_STEP + 10;
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
        uidisp->draw_printf(1, ROW_Y0 + i * ROW_STEP, 12, 0, 255, "%s", term[ln % TERM_LINES]);
    }
    if (cursorOn) {
        int x, y;
        cursorCell(&x, &y);
        if (x >= 0) uidisp->draw_box(x, y, x + 7, y + 1, 0, -1);
    }
    uidisp->draw_printf(2, HINT_Y, 12, 0, 255, "ON:cls shON:exit AC:^C UP/DN");
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
                if (shift && key == KEY_ON) { // Shift+ON 退出
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
                    if (lineLen == 0 && contMode) mpy_repl_feed_char(0x04); // 空行：Ctrl-D 结束块（否则 auto-indent 会反复续行）
                    else mpy_repl_feed_char('\r');
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
                cursorOn = 1;
                termDirty = 1;
            }
        } else {
            lastKey = -1;
        }
        blinkDiv++;
        if (blinkDiv >= 25) { // ~500ms 光标闪烁（仅刷新光标 2px 竖条，不整屏重绘）
            blinkDiv = 0;
            cursorOn = !cursorOn;
            if (!termDirty) cursorPaint(cursorOn);
        }
        if (termDirty) {
            termDirty = 0;
            rowDirty = -1;
            draw();
        } else if (rowDirty >= 0) { // 仅重绘最底行（输入回显：避免整屏刷新）
            int ln = termLines - 1;
            int ly = ROW_Y0 + rowDirty * ROW_STEP;
            uidisp->draw_box(1, ly, LCD_PIX_W - 2, ly + 11, 255, 255);
            if (ln >= 0) uidisp->draw_printf(1, ly, 12, 0, 255, "%s", term[ln % TERM_LINES]);
            if (cursorOn) {
                int x, y;
                cursorCell(&x, &y);
                if (x >= 0) uidisp->draw_box(x, y, x + 7, y + 1, 0, -1);
            }
            uidisp->flushRect(0, ly, LCD_PIX_W - 1, ly + 11);
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
