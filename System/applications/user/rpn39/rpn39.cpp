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
static int shiftHeld = 0; // Shift 状态（全局，供绘制显示）

static double stX = 0, stY = 0, stZ = 0, stT = 0; // 4 层栈
static double lastX = 0;                          // LAST X
static int entering = 0;                          // 数字输入中
static int autoLift = 0;                           // 栈提升标志（42S：计算后输入数字自动压栈）
static char inbuf[40];
static int inlen = 0;
static int rpn39Running = 0;

// ---- 寄存器（A-Z，42S STO/RCL）----
static double regs[26] = {0};
static int rpnMode = 0;       // 0=正常 1=STO 等字母 2=RCL 等字母 3=寄存器列表
static int clearConfirm = 0;  // CLEAR ALL 确认态
static int regSel = 0;        // 列表高亮（0-25）
static int regTop = 0;        // 列表滚动顶

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

// 掉电持久化（/rpn39_sto.dat：26 寄存器 + 4 栈 = 240B；旧版 208B 兼容——栈默认 0）
static void saveRegs(void) {
    FIL f;
    if (f_open(&f, "/rpn39_sto.dat", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK) {
        UINT bw = 0;
        f_write(&f, regs, sizeof(regs), &bw);          // 26 regs
        double stack[4] = {stX, stY, stZ, stT};
        f_write(&f, stack, sizeof(stack), &bw);        // X Y Z T（HP：关机保留栈）
        f_close(&f);
    }
}
static void loadRegs(void) {
    FIL f;
    UINT br = 0;
    memset(regs, 0, sizeof(regs));
    if (f_open(&f, "/rpn39_sto.dat", FA_OPEN_EXISTING | FA_READ) == FR_OK) {
        f_read(&f, regs, sizeof(regs), &br); // regs 区（br 出参）
        if (br >= sizeof(regs)) {            // 有栈区（新版文件）
            double stack[4] = {0};
            UINT br2 = 0;
            f_read(&f, stack, sizeof(stack), &br2);
            if (br2 == sizeof(stack)) { stX = stack[0]; stY = stack[1]; stZ = stack[2]; stT = stack[3]; }
        }
        f_close(&f);
    }
}

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

// ---- 绘制（左对齐，寄存器/X 16px，菜单 12px 六段均分）----
static void draw(void) {
    char buf[48];
    int i;
    uidisp->draw_box(0, 0, 255, 127, 255, 255); // 白底
    if (rpnMode == 1)
        uidisp->draw_printf(0, 0, 12, 255, 0, "STO _");
    else if (rpnMode == 2)
        uidisp->draw_printf(0, 0, 12, 255, 0, "RCL _");
    else if (shiftHeld)
        uidisp->draw_printf(0, 0, 12, 255, 0, "RPN39 S");
    else
        uidisp->draw_printf(0, 0, 12, 0, 255, "RPN39");
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

// X 行：标签 X: + 数字（24px 等宽，最多 12 字符）
static void drawXLine(void) {
    char buf[48], buf2[48];
    const char *num = entering ? inbuf : fmtNum(stX, buf);
    if ((int)strlen(num) > 12) { // 12 字符 x 16px = 192px + 标签 32px
        sprintf(buf2, "%.6e", stX); // 超宽科学计数压缩
        num = buf2;
    }
    uidisp->draw_printf(0, 92, 24, 0, 255, "X: %s", num);
}

// X 行区域刷新（输入变化专用）
static void drawX(void) {
    uidisp->draw_box(0, 92, 255, 115, 255, 255);
    drawXLine();
    uidisp->flushRect(0, 92, 255, 115);
}

// ---- 按键处理（返回 1 = 仅 X 行变化（区域刷新），0 = 全屏刷新）----
static int handleKey(int key) {
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
        int idx = regKeyToIdx(key);
        rpnMode = 0; // 无论是否命中先退出等待态（非字母=取消）
        if (idx >= 0) {
            if (entering) { stX = atof(inbuf); entering = 0; inlen = 0; }
            if (mode == 1) {          // STO：存 X 到寄存器
                regs[idx] = stX;
                saveRegs();
            } else {                  // RCL：寄存器值压栈（42S stack lift）
                lastX = stX;
                stackLift();
                stX = regs[idx];
                autoLift = 0;
            }
        }
        return 0;
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
                if (inlen > 0 && inbuf[0] == '-') { memmove(inbuf, inbuf + 1, inlen); inlen--; }
                else if (inlen < 12) { memmove(inbuf + 1, inbuf, inlen + 1); inbuf[0] = '-'; inlen++; }
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
                    int r = handleKey((int)key);
                    if (shiftHeld) { // shift + 普通键：动作后自动退 shift
                        shiftHeld = 0;
                        ll_disp_set_indicator(0, -1);
                        draw();
                    } else {
                        if (rpnMode == 3) drawRegList(); // 列表模式全屏
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
    saveRegs(); // 退出保存（寄存器 + 栈——HP 关机保留语义）
    entering = 0; inlen = 0; inbuf[0] = 0; // 清输入态（避免重进残留）
    uidisp->draw_box(0, 0, 255, 127, 255, 255);
    uidisp->flush();
    SystemUIResume();
    vTaskDelete(NULL);
}

extern "C" void StartRPN39() {
    xTaskCreate(rpn39Task, "RPN39", 4096, NULL, configMAX_PRIORITIES - 3, NULL);
}
