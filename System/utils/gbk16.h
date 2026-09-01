#ifndef __GBK16_H__
#define __GBK16_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 返回 GB2312 字符 (b1=高字节, b2=低字节) 的 32 字节点阵指针（HZK16S 字库，flash XIP）
// 非法字符或越界返回 NULL（统一范围与字库边界检查，供 vGL/lvgl 两套渲染后端共用）
const unsigned char *gbk16_glyph(unsigned char b1, unsigned char b2);

#ifdef __cplusplus
}
#endif

#endif