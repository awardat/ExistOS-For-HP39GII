/*
 * HP39GII MicroPython 端口入口（供 System 的 Python app 调用）
 * 2026-09-20：M1——REPL 事件驱动 + 直接执行字符串
 *
 * 对外 API（后续在 System 侧声明并调用）：
 *   void mpy_init(void *heap, size_t heap_size);  // 初始化（堆由 App 提供，malloc 大堆含 swap）
 *   void mpy_deinit(void);                        // 反初始化（mp_deinit）
 *   int  mpy_repl_init(void);                     // 初始化事件 REPL
 *   int  mpy_repl_feed_char(int c);               // 喂入一个字符；返回非 0 = 本次输入已执行完
 *   int  mpy_exec_str(const char *src);           // 直接执行源码（文件运行/F5）；返回 0 成功
 *   void mpy_gc_collect(void);                    // 主动 GC
 *
 * HAL（弱符号，System 侧可覆盖）：
 *   mp_hal_stdout_tx_strn  输出（App 收集到显示缓冲）
 *   mp_hal_stdin_rx_chr    输入（事件 REPL 用不到，保留接口）
 *   mp_hal_ticks_ms/us     time 模块计时
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "py/obj.h"
#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/gc.h"
#include "py/repl.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "shared/runtime/pyexec.h"
#include "mpconfigport.h"

static char *mpy_stack_top = NULL;
static void *mpy_heap_ptr = NULL;

#if MICROPY_ENABLE_GC
static void mpy_gc_collect_internal(void) {
    void *dummy;
    gc_collect_start();
    if (mpy_stack_top)
        gc_collect_root(&dummy, ((mp_uint_t)mpy_stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
}
void gc_collect(void) __attribute__((weak, alias("mpy_gc_collect_internal")));
#endif

void mpy_init(void *heap, size_t heap_size) {
    int stack_dummy;
    mpy_stack_top = (char *)&stack_dummy;
    mpy_heap_ptr = heap;
    mp_stack_ctrl_init();
    mp_stack_set_limit(6 * 1024); // Python 递归上限（任务栈 16KB 内，留足 C 调用余量）
    gc_init(heap, (uint8_t *)heap + heap_size);
    mp_init();
}

void mpy_deinit(void) {
    mp_deinit();
}

int mpy_repl_init(void) {
    pyexec_event_repl_init();
    return 0;
}

int mpy_repl_feed_char(int c) {
    return pyexec_event_repl_process_char(c);
}

int mpy_exec_str(const char *src) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        if (lex == NULL) {
            nlr_pop();
            return -1;
        }
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, false);
        mp_call_function_0(module_fun);
        nlr_pop();
        return 0;
    }
    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    return -1;
}

void mpy_gc_collect(void) {
#if MICROPY_ENABLE_GC
    gc_collect();
#endif
}

/* ---- HAL 弱实现（System 侧覆盖）---- */
MP_WEAK mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    (void)str;
    return len;
}

MP_WEAK int mp_hal_stdin_rx_chr(void) {
    return 0;
}

MP_WEAK mp_uint_t mp_hal_ticks_ms(void) {
    return 0;
}

MP_WEAK mp_uint_t mp_hal_ticks_us(void) {
    return 0;
}

MP_WEAK void mp_hal_delay_us(mp_uint_t us) {
    (void)us;
}

MP_WEAK void mp_hal_delay_ms(mp_uint_t ms) {
    (void)ms;
}

MP_WEAK mp_uint_t mp_hal_ticks_cpu(void) {
    return 0;
}

MP_WEAK void mp_hal_set_interrupt_char(int c) {
    (void)c;
}

/* ---- 文件系统接口（v1 关闭外部导入；文件 IO 在 M2 接入 FatFs）---- */
mp_import_stat_t mp_import_stat(const char *path) {
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    (void)filename;
    mp_raise_OSError(MP_ENOENT);
}

// open()：M2 接入 FatFs 前先抛错（modio/modbuiltins 要求端口提供）
mp_obj_t mp_builtin_open_obj_stub(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    (void)n_args;
    (void)args;
    (void)kwargs;
    mp_raise_OSError(MP_ENOENT);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open_obj_stub);

void nlr_jump_fail(void *val) {
    (void)val;
    for (;;) {
    }
}
