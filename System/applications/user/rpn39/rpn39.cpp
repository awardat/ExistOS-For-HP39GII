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
static int autoLift = 0;                           // 栈提升标志（42S：计算后输入数字自动压栈）
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

// 菜单项（F1-F6；无功能的显示占位）
static const char *menuItems[6] = {"x<>y", "Rdn", "DROP", "", "", ""};

// ---- 绘制（左对齐，寄存器/X 16px，菜单 12px 六段均分）----
static void draw(void) {
    char buf[48];
    int i;
    uidisp->draw_box(0, 0, 255, 127, 255, 255); // 白底
    uidisp->draw_printf(0, 0, 12, 0, 255, "RPN39");
    uidisp->draw_printf(0, 16, 16, 0, 255, "T: %s", fmtNum(stT, buf));
    uidisp->draw_printf(0, 32, 16, 0, 255, "Z: %s", fmtNum(stZ, buf));
    uidisp->draw_printf(0, 48, 16, 0, 255, "Y: %s", fmtNum(stY, buf));
    if (entering)
        uidisp->draw_printf(0, 64, 16, 0, 255, "X: %s", inbuf);
    else
        uidisp->draw_printf(0, 64, 16, 0, 255, "X: %s", fmtNum(stX, buf));
    // 菜单行：六段均分（每段 42px），空位显示占位
    for (i = 0; i < 6; i++) {
        const char *t = menuItems[i];
        if (t[0] == 0) t = "_";
        uidisp->draw_printf(i * 42 + 2, 112, 12, 255, 0, "%s", t);
    }
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
        if (!entering) {
            if (autoLift) { stackLift(); autoLift = 0; } // 42S：计算后输入自动压栈（X->Y）
            entering = 1; inlen = 0; inbuf[0] = 0;
        }
        if (inlen < 30) { inbuf[inlen++] = (char)('0' + d); inbuf[inlen] = 0; }
        return;
    }
    switch (key) {
        case KEY_DOT:
            if (!entering) {
                if (autoLift) { stackLift(); autoLift = 0; }
                entering = 1; inlen = 0; inbuf[0] = 0;
            }
            if (inlen < 30 && !strchr(inbuf, '.')) { inbuf[inlen++] = '.'; inbuf[inlen] = 0; }
            break;
        case KEY_NEGATIVE:
            if (entering) {
                if (inlen > 0 && inbuf[0] == '-') { memmove(inbuf, inbuf + 1, inlen); inlen--; }
                else if (inlen < 30) { memmove(inbuf + 1, inbuf, inlen + 1); inbuf[0] = '-'; inlen++; }
            } else {
                stX = -stX;
                autoLift = 0;
            }
            break;
        case KEY_BACKSPACE:
            if (entering && inlen > 0) { inlen--; inbuf[inlen] = 0; }
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
        case KEY_F1: { double t = stX; stX = stY; stY = t; autoLift = 0; break; } // x<>y
        case KEY_F2: { double t = stT; stT = stZ; stZ = stY; stY = stX; stX = t; autoLift = 0; break; } // R↓
        case KEY_F3: stackDrop(); autoLift = 0; break;              // DROP
        case KEY_F4: break; // 预留（CLx 移至 Shift+backspace）
        case KEY_ON:
            if (entering) { entering = 0; inlen = 0; autoLift = 0; }
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
    uidisp->restoreBuffer(); // UI_Suspend 已释放 disp_buf（releaseBuffer），重新分配堆缓冲（panic 尝试：避开 emergencyBuffer 固定区）
    rpn39Running = 1;
    draw();
    int lastKey = -1;
    int shiftHeld = 0;
    while (rpn39Running) {
        uint32_t keys = ll_vm_check_key();
        uint32_t kp = keys >> 16;
        uint32_t key = keys & 0xFFFF;
        if (kp) {
            if (key == KEY_SHIFT) { shiftHeld = 1; lastKey = key; }
            else if (key != (uint32_t)lastKey) { // 按下沿（防重复）
                lastKey = key;
                if (key == KEY_BACKSPACE && shiftHeld) {
                    stX = 0; entering = 0; inlen = 0; autoLift = 0; // CLx（Shift+backspace）
                } else {
                    handleKey((int)key);
                }
                draw();
            }
        } else {
            lastKey = -1;
            shiftHeld = 0;
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
