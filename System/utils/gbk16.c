#include "gbk16.h"
#include <stddef.h>

extern const unsigned char fonts_hzk_start[];
extern const unsigned char fonts_hzk_end[];

const unsigned char *gbk16_glyph(unsigned char b1, unsigned char b2) {
    if (b1 < 0xA1 || b1 > 0xF7 || b2 < 0xA1 || b2 > 0xFE)
        return NULL;
    uint32_t offset = ((uint32_t)(b1 - 0xA1) * 94 + (b2 - 0xA1)) * 32;
    const unsigned char *p = fonts_hzk_start + offset;
    if (p + 32 > fonts_hzk_end)
        return NULL;
    return p;
}