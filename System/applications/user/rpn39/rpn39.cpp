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
static int shiftHeld = 0; // Shift 状态（全局，供绘制显示）

static double stX = 0, stY = 0, stZ = 0, stT = 0; // 4 层栈
static double lastX = 0;                          // LAST X
static int entering = 0;                          // 数字输入中
static int autoLift = 0;                           // 栈提升标志（42S：计算后输入数字自动压栈）
static char inbuf[40];
static int inlen = 0;
static int rpn39Running = 0;

// ---- 栈操作（HP RPN 语义）----
static void stackLift() { stT = stZ; stZ = stY; stY = stX; }
static void stackDrop() { stX = stY; stY = stZ; stZ = stT; stT = 0; } // DROP：X 丢弃，栈上移，T 清空

// ---- 数字格式化（整数直显，否则 10 位有效数字）----
static const char *fmtNum(double v, char *buf) {
    if (v > -1e9 && v < 1e9 && v == (double)(long long)v) // 范围先判（防 2^63 UB），整数 <=9 位直显
        sprintf(buf, "%lld", (long long)v);
    else
        sprintf(buf, "%.12g", v); // YZT 行宽裕（14px x 15 字符=210px）；X 行超长由 drawXLine 压缩
    return buf;
}

// 菜单项（F1-F6；无功能的显示占位）
static const char *menuItems[6] = {"x<>y", "Rdn", "DROP", "", "", ""};

static void drawXLine(void); // 前置声明（draw 内调用）

// ---- 绘制（左对齐，寄存器/X 16px，菜单 12px 六段均分）----
static void draw(void) {
    char buf[48];
    int i;
    uidisp->draw_box(0, 0, 255, 127, 255, 255); // 白底
    if (shiftHeld)
        uidisp->draw_printf(0, 0, 12, 255, 0, "RPN39 S");
    else
        uidisp->draw_printf(0, 0, 12, 0, 255, "RPN39");
    uidisp->draw_printf(0, 16, 20, 0, 255, "T: %s", fmtNum(stT, buf));
    uidisp->draw_printf(0, 36, 20, 0, 255, "Z: %s", fmtNum(stZ, buf));
    uidisp->draw_printf(0, 56, 20, 0, 255, "Y: %s", fmtNum(stY, buf));
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

// X 行：左侧标签 X:（20px，与其他行标一致）+ 数字右对齐（32px Fira）
static void drawXLine(void) {
    char buf[48], buf2[48];
    const char *num = entering ? inbuf : fmtNum(stX, buf);
    if ((int)strlen(num) > 10) { // X 行数字上限 10 字符（32px 21px/字符 + X: 标签）
        sprintf(buf2, "%.4e", stX); // 超宽科学计数压缩
        num = buf2;
    }
    int len = (int)strlen(num);
    int x = 256 - len * 21 - 1; // 32px 字符宽 21，右对齐
    if (x < 30) x = 30;         // 不遮标签
    uidisp->draw_printf(0, 76, 20, 0, 255, "X:");
    uidisp->draw_printf(x, 76, 32, 0, 255, "%s", num);
}

// X 行区域刷新（输入变化专用）
static void drawX(void) {
    uidisp->draw_box(0, 76, 255, 107, 255, 255);
    drawXLine();
    uidisp->flushRect(0, 76, 255, 107);
}

// ---- 按键处理（返回 1 = 仅 X 行变化（区域刷新），0 = 全屏刷新）----
static int handleKey(int key) {
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
        if (inlen < 13) { inbuf[inlen++] = (char)('0' + d); inbuf[inlen] = 0; }
        return 1;
    }
    switch (key) {
        case KEY_DOT:
            if (!entering) {
                if (autoLift) { stackLift(); autoLift = 0; }
                entering = 1; inlen = 0; inbuf[0] = 0;
            }
            if (inlen < 13 && !strchr(inbuf, '.')) { inbuf[inlen++] = '.'; inbuf[inlen] = 0; }
            return 1;
        case KEY_NEGATIVE:
            if (entering) {
                if (inlen > 0 && inbuf[0] == '-') { memmove(inbuf, inbuf + 1, inlen); inlen--; }
                else if (inlen < 13) { memmove(inbuf + 1, inbuf, inlen + 1); inbuf[0] = '-'; inlen++; }
            } else {
                stX = -stX;
                autoLift = 0;
            }
            return 1;
        case KEY_BACKSPACE:
            if (entering && inlen > 0) { inlen--; inbuf[inlen] = 0; return 1; }
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
        case KEY_F4: break; // 预留（CLx 移至 Shift+backspace）
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
                    draw();
                }
                lastKey = key;
            } else if (key != (uint32_t)lastKey) { // 按下沿（防重复）
                lastKey = key;
                if (key == KEY_BACKSPACE && shiftHeld) {
                    stX = 0; entering = 0; inlen = 0; autoLift = 0; // CLx（Shift+backspace）
                    shiftHeld = 0; ll_disp_set_indicator(0, -1);   // 动作完成自动退 shift
                    draw();
                } else if (key == KEY_ON && shiftHeld) {
                    rpn39Running = 0; // Shift+ON 退出（不触发系统关机）
                } else {
                    int r = handleKey((int)key);
                    if (shiftHeld) { // shift + 普通键：动作后自动退 shift
                        shiftHeld = 0;
                        ll_disp_set_indicator(0, -1);
                        draw();
                    } else {
                        if (r) drawX();  // 输入变化：X 行区域刷新
                        else draw();     // 栈/菜单变化：全屏刷新
                    }
                }
            }
        } else {
            lastKey = -1; // 松开：仅复位边沿检测（shift 保持到动作完成）
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (shiftHeld) { shiftHeld = 0; ll_disp_set_indicator(0, -1); }
    entering = 0; inlen = 0; inbuf[0] = 0; // 清输入态（避免重进残留）
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    uidisp->flush();
    SystemUIResume();
    vTaskDelete(NULL);
}

extern "C" void StartRPN39() {
    xTaskCreate(rpn39Task, "RPN39", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
}
