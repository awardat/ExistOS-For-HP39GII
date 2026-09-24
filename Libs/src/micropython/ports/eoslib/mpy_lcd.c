// Python app 绘图桥（lcd 模块）——2026-09-23
//
// 端口侧只做参数检查与转发：实际绘制/等待按键由 System 侧（pyapp.cpp）通过下述
// 弱符号钩子实现，避免 MicroPython 库反向依赖 System 的显示代码。
//
// 用法（Python）：
//   import framebuf, lcd
//   fb = framebuf.FrameBuffer(bytearray(256*128//8), 256, 128, framebuf.MONO_HLSB)
//   fb.line(0, 0, 100, 50, 1)
//   lcd.blit(fb)          # 推到屏幕（含刷新）
//   lcd.text(4, 4, "hi")  # 文字（走系统字库，支持中文）
//   lcd.wait_key()        # 阻塞直到按 ON/F5（用完回到终端）
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/objstr.h"
#include "py/mphal.h"

// 与 extmod/modframebuf.c 的 mp_obj_framebuf_t 布局保持一致（该结构未导出头文件）
typedef struct {
    mp_obj_base_t base;
    mp_obj_t buf_obj;
    void *buf;
    uint16_t width, height, stride;
    uint8_t format;
} mpy_framebuf_t;

#define MPY_FB_MHLSB (3) // framebuf.MONO_HLSB

// ---------------- 弱符号钩子（System 侧实现） ----------------
__attribute__((weak)) void mpy_lcd_blit1(const uint8_t *buf, int w, int h, int x, int y) {
    (void)buf; (void)w; (void)h; (void)x; (void)y;
}
__attribute__((weak)) void mpy_lcd_text(int x, int y, const char *s, int size) {
    (void)x; (void)y; (void)s; (void)size;
}
__attribute__((weak)) void mpy_lcd_clear(int color) { (void)color; }
__attribute__((weak)) int mpy_lcd_wait_key(void) { return -1; }
__attribute__((weak)) int mpy_lcd_width(void) { return 256; }
__attribute__((weak)) int mpy_lcd_height(void) { return 127; }

// ---------------- lcd 模块 ----------------
static mp_obj_t lcd_width(void) {
    return mp_obj_new_int(mpy_lcd_width());
}
static MP_DEFINE_CONST_FUN_OBJ_0(lcd_width_obj, lcd_width);

static mp_obj_t lcd_height(void) {
    return mp_obj_new_int(mpy_lcd_height());
}
static MP_DEFINE_CONST_FUN_OBJ_0(lcd_height_obj, lcd_height);

static mp_obj_t lcd_clear(size_t n_args, const mp_obj_t *args) {
    int color = 255; // 默认白
    if (n_args > 0) {
        color = mp_obj_get_int(args[0]);
    }
    mpy_lcd_clear(color);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lcd_clear_obj, 0, 1, lcd_clear);

static mp_obj_t lcd_text(size_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    const char *s = mp_obj_str_get_str(args[2]);
    int size = (n_args > 3) ? mp_obj_get_int(args[3]) : 16;
    mpy_lcd_text(x, y, s, size);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lcd_text_obj, 3, 4, lcd_text);

static mp_obj_t lcd_blit(size_t n_args, const mp_obj_t *args) {
    if (mp_obj_get_type(args[0])->name != MP_QSTR_FrameBuffer) {
        mp_raise_TypeError(MP_ERROR_TEXT("expect framebuf"));
    }
    mpy_framebuf_t *fb = (mpy_framebuf_t *)MP_OBJ_TO_PTR(args[0]);
    if (fb->format != MPY_FB_MHLSB) {
        mp_raise_ValueError(MP_ERROR_TEXT("only MONO_HLSB"));
    }
    int x = (n_args > 1) ? mp_obj_get_int(args[1]) : 0;
    int y = (n_args > 2) ? mp_obj_get_int(args[2]) : 0;
    mpy_lcd_blit1((const uint8_t *)fb->buf, fb->width, fb->height, x, y);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lcd_blit_obj, 1, 3, lcd_blit);

static mp_obj_t lcd_wait_key(void) {
    int k = mpy_lcd_wait_key();
    return mp_obj_new_int(k);
}
static MP_DEFINE_CONST_FUN_OBJ_0(lcd_wait_key_obj, lcd_wait_key);

static const mp_rom_map_elem_t lcd_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lcd) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&lcd_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&lcd_height_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&lcd_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&lcd_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&lcd_blit_obj) },
    { MP_ROM_QSTR(MP_QSTR_wait_key), MP_ROM_PTR(&lcd_wait_key_obj) },
};
static MP_DEFINE_CONST_DICT(lcd_module_globals, lcd_module_globals_table);

const mp_obj_module_t mpy_lcd_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&lcd_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lcd, mpy_lcd_module);
