// RPN39 - RPN 计算器（阶段 1：4 层栈 + 基础四则）
// 设计见 docs/RPN39-design.md（42S 基准，实现原创）
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdarg.h>
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
}

static double stX = 0, stY = 0, stZ = 0, stT = 0; // 4 层栈
static double lastX = 0;                          // LAST X
static int entering = 0;                          // 数字输入中
static char inbuf[40];
static int inlen = 0;
static int rpn39Running = 0;

// ---- 栈操作（HP RPN 语义）----
static void stackLift() { stT = stZ; stZ = stY; stY = stX; }
static void stackDrop() { stX = stY; stY = stZ; stZ = stT; }

// ---- 数字格式化（整数直显，否则 10 位有效数字）----
static const char *fmtNum(double v, char *buf) {
    if (v == (double)(long long)v && v > -1e15 && v < 1e15)
        sprintf(buf, "%lld", (long long)v);
    else
        sprintf(buf, "%.10g", v);
    return buf;
}

// 右侧对齐打印（fsize 16 -> 每字符 8px，8 -> 6px）
static void printR(int y, int fsize, const char *s) {
    int w = strlen(s) * (fsize == 16 ? 8 : 6);
    if (w > 255) w = 255;
    uidisp->draw_printf(255 - w, y, fsize, 0, 255, "%s", s);
}

// ---- 绘制 ----
static void draw(void) {
    char buf[48];
    uidisp->draw_box(0, 0, 255, 127, 255, 255); // 白底
    uidisp->draw_printf(0, 0, 8, 0, 255, "RPN39");
    printR(16, 8, fmtNum(stT, buf));
    printR(32, 8, fmtNum(stZ, buf));
    printR(48, 8, fmtNum(stY, buf));
    if (entering)
        printR(64, 16, inbuf);
    else
        printR(64, 16, fmtNum(stX, buf));
    // 菜单行
    uidisp->draw_printf(0, 112, 8, 255, 0, "x<>y |Rdn  |DROP |CLx  |     |");
    uidisp->flush();
}

// ---- 按键处理 ----
static void handleKey(int key) {
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
        if (!entering) { entering = 1; inlen = 0; inbuf[0] = 0; }
        if (inlen < 30) { inbuf[inlen++] = (char)('0' + d); inbuf[inlen] = 0; }
        return;
    }
    switch (key) {
        case KEY_DOT:
            if (!entering) { entering = 1; inlen = 0; inbuf[0] = 0; }
            if (inlen < 30 && !strchr(inbuf, '.')) { inbuf[inlen++] = '.'; inbuf[inlen] = 0; }
            break;
        case KEY_NEGATIVE:
            if (entering) {
                if (inlen > 0 && inbuf[0] == '-') { memmove(inbuf, inbuf + 1, inlen); inlen--; }
                else if (inlen < 30) { memmove(inbuf + 1, inbuf, inlen + 1); inbuf[0] = '-'; inlen++; }
            } else {
                stX = -stX;
            }
            break;
        case KEY_BACKSPACE:
            if (entering && inlen > 0) { inlen--; inbuf[inlen] = 0; }
            break;
        case KEY_ENTER:
            if (entering) {
                stackLift();
                stX = atof(inbuf);
                lastX = stX;
                entering = 0; inlen = 0;
            } else {
                stackLift(); // 复制 X 压栈（HP 惯例）
            }
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
            break;
        }
        case KEY_F1: { double t = stX; stX = stY; stY = t; break; } // x<>y
        case KEY_F2: { double t = stT; stT = stZ; stZ = stY; stY = stX; stX = t; break; } // R↓
        case KEY_F3: stackDrop(); break;                            // DROP
        case KEY_F4: stX = 0; entering = 0; inlen = 0; break;       // CLx
        case KEY_ON:
            if (entering) { entering = 0; inlen = 0; }
            else { rpn39Running = 0; }
            break;
        case KEY_HOME:
            rpn39Running = 0;
            break;
        default:
            break;
    }
}

// ---- 任务 ----
void rpn39Task(void *_) {
    SystemUISuspend();
    uidisp->emergencyBuffer(); // UI_Suspend 已释放 disp_buf（releaseBuffer），切到固定 RAM 缓冲
    rpn39Running = 1;
    draw();
    int lastKey = -1;
    while (rpn39Running) {
        uint32_t keys = ll_vm_check_key();
        uint32_t kp = keys >> 16;
        uint32_t key = keys & 0xFFFF;
        if (kp) {
            if (key != (uint32_t)lastKey) { // 按下沿（防重复）
                lastKey = key;
                handleKey((int)key);
                draw();
            }
        } else {
            lastKey = -1;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    uidisp->flush();
    SystemUIResume();
    vTaskDelete(NULL);
}

extern "C" void StartRPN39() {
    xTaskCreate(rpn39Task, "RPN39", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
}
