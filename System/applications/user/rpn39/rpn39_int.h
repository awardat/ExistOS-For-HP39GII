// RPN39 内部共享接口（rpn39.cpp 主模块 ↔ rpn39_ext.cpp 扩展模块：复数/矩阵/统计）
#ifndef __RPN39_INT_H__
#define __RPN39_INT_H__

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

extern int shiftHeld;
extern double stX, stY, stZ, stT;
extern double lastX;
extern int entering;
extern int autoLift;
extern char inbuf[40];
extern int inlen;
extern int rpn39Running;
extern double regs[26];
extern int rpnMode;      // 0=正常 1=STO 2=RCL 3=VARS 4=MATH 5=CPLX 6=MATX 7=STAT
extern int stoOp;
extern int angMode;      // 0=DEG 1=RAD 2=GRAD
extern int fracMode;
extern int mathPage;

void saveRegs(void);
void loadRegs(void);
void stackLift(void);
void stackDrop(void);
const char *fmtNum(double v, char *buf);
double toRad(double v);
double fromRad(double v);
int valid(double r);
void unaryOp(double (*f)(double), int angIn, int angOut);
void binaryOp(double (*g)(double, double));
void pushConst(double v);
void drawXLine(void);
int drawTextMix(int x, int y, const char *s, uint8_t fg, int16_t bg);

// 扩展模块（rpn39_ext.cpp）
int rpn39ExtKey(int mode, int key, int shift);
void rpn39ExtDraw(int mode);
void rpn39ExtExit(int mode);   // 退出扩展态：最后持久化
void rpn39ExtEnter(int mode);  // 进入扩展态：初始化
void rpn39StatAccum(int add);  // 主界面 F4/F5：Σ+/Σ− 收集
void rpn39StatSessionReset(void); // RPN39 启动：统计新会话从空开始（旧数据经 Shift+7 页加载查看）

#endif
