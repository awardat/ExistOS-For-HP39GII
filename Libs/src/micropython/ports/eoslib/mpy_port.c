/*
 * HP39GII MicroPython 端口入口（供 System 的 Python app 调用）
 * 2026-09-20：M1——REPL 事件驱动 + 直接执行字符串
 *
 * 对外 API（后续在 System 侧声明并调用）：
 *   void mpy_init(void *heap, size_t heap_size);  // 初始化（堆由 App 提供，malloc 大堆含 swap）
 *   void mpy_deinit(void);                        // 反初始化（mp_deinit）
 *   int  mpy_repl_init(void);                     // 初始化事件 REPL
 *   int  mpy_repl_feed_char(int c);               // 喂入一个字符；返回非 0 = 本次输入已执行完
 *   int  mpy_exec_str(const char *src);           // 直接执行源码（文件运行）；返回 0 成功
 *   int  mpy_run_cell(const char *src);           // 执行单元格（单语句回显值 / 多行 exec）；返回 0 成功
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
#include "py/stream.h"
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

// 执行一个"单元格"：单语句/表达式用 single 模式（回显结果值），多行代码用 exec 模式
int mpy_run_cell(const char *src) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        bool done = false;
        {   // 先试 single 模式（解析失败则静默回退到 exec）
            nlr_buf_t nlr2;
            if (nlr_push(&nlr2) == 0) {
                mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
                if (lex != NULL) {
                    qstr sn = lex->source_name;
                    mp_parse_tree_t pt = mp_parse(lex, MP_PARSE_SINGLE_INPUT);
                    mp_obj_t f = mp_compile(&pt, sn, true);
                    nlr_pop();
                    mp_call_function_0(f); // 运行异常由外层 nlr 捕获并打印
                    done = true;
                } else {
                    nlr_pop();
                }
            } else {
                MP_STATE_THREAD(mp_pending_exception) = MP_OBJ_NULL; // 清除解析异常，改用 exec
            }
        }
        if (!done) {
            mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
            if (lex == NULL) {
                nlr_pop();
                return -1;
            }
            qstr sn = lex->source_name;
            mp_parse_tree_t pt = mp_parse(lex, MP_PARSE_FILE_INPUT);
            mp_obj_t f = mp_compile(&pt, sn, false);
            mp_call_function_0(f);
        }
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

/* ---- 文件系统接口（App 侧实现 mpy_fs_* 钩子）---- */
MP_WEAK void *mpy_fs_fopen(const char *path, const char *mode);
MP_WEAK size_t mpy_fs_fread(void *h, void *buf, size_t n);
MP_WEAK size_t mpy_fs_fwrite(void *h, const void *buf, size_t n);
MP_WEAK int mpy_fs_fclose(void *h);
MP_WEAK long mpy_fs_fseek(void *h, long off, int whence);
MP_WEAK long mpy_fs_ftell(void *h);
MP_WEAK int mpy_fs_stat(const char *path, int *isdir);

MP_WEAK int mpy_fs_stat(const char *path, int *isdir) {
    (void)path;
    (void)isdir;
    return 0;
}

mp_import_stat_t mp_import_stat(const char *path) {
    int isdir = 0;
    if (mpy_fs_stat(path, &isdir)) {
        return isdir ? MP_IMPORT_STAT_DIR : MP_IMPORT_STAT_FILE;
    }
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    const char *path = qstr_str(filename);
    void *h = mpy_fs_fopen(path, "r");
    if (h == NULL) {
        mp_raise_OSError(MP_ENOENT);
    }
    long sz = mpy_fs_fseek(h, 0, 2); // 文件大小
    if (sz < 0) {
        sz = 0;
    }
    mpy_fs_fseek(h, 0, 0);
    char *buf = m_new(char, (size_t)sz + 1);
    size_t n = mpy_fs_fread(h, buf, (size_t)sz);
    mpy_fs_fclose(h);
    buf[n] = 0;
    return mp_lexer_new_from_str_len(filename, buf, n, (size_t)sz + 1);
}

/* ---- 文件对象（M3：FatFs 垫片，App 侧实现下列钩子）---- */
MP_WEAK void *mpy_fs_fopen(const char *path, const char *mode) {
    (void)path;
    (void)mode;
    return NULL;
}
MP_WEAK size_t mpy_fs_fread(void *h, void *buf, size_t n) {
    (void)h;
    (void)buf;
    (void)n;
    return 0;
}
MP_WEAK size_t mpy_fs_fwrite(void *h, const void *buf, size_t n) {
    (void)h;
    (void)buf;
    (void)n;
    return 0;
}
MP_WEAK int mpy_fs_fclose(void *h) {
    (void)h;
    return -1;
}
MP_WEAK long mpy_fs_fseek(void *h, long off, int whence) {
    (void)h;
    (void)off;
    (void)whence;
    return -1;
}
MP_WEAK long mpy_fs_ftell(void *h) {
    (void)h;
    return -1;
}

typedef struct _mp_obj_mpyfile_t {
    mp_obj_base_t base;
    void *h;
} mp_obj_mpyfile_t;

static mp_uint_t mpyfile_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    mp_obj_mpyfile_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->h == NULL) {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }
    return mpy_fs_fread(self->h, buf, size);
}

static mp_uint_t mpyfile_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    mp_obj_mpyfile_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->h == NULL) {
        *errcode = MP_EBADF;
        return MP_STREAM_ERROR;
    }
    return mpy_fs_fwrite(self->h, buf, size);
}

static mp_uint_t mpyfile_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mp_obj_mpyfile_t *self = MP_OBJ_TO_PTR(self_in);
    switch (request) {
    case MP_STREAM_SEEK: {
        struct mp_stream_seek_t *s = (struct mp_stream_seek_t *)arg;
        long r = mpy_fs_fseek(self->h, (long)s->offset, (int)s->whence);
        if (r < 0) {
            *errcode = MP_EINVAL;
            return MP_STREAM_ERROR;
        }
        s->offset = r;
        return 0;
    }
    case MP_STREAM_FLUSH:
        return 0;
    case MP_STREAM_CLOSE: {
        int r = mpy_fs_fclose(self->h);
        self->h = NULL;
        return r == 0 ? 0 : MP_STREAM_ERROR;
    }
    default:
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
    }
}

static void mpyfile_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_mpyfile_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<file %p>", self->h);
}

static const mp_rom_map_elem_t mpyfile_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_stream___exit___obj) },
};
static MP_DEFINE_CONST_DICT(mpyfile_locals_dict, mpyfile_locals_dict_table);

static const mp_stream_p_t mpyfile_stream_p = {
    .read = mpyfile_read,
    .write = mpyfile_write,
    .ioctl = mpyfile_ioctl,
    .is_text = true,
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_mpyfile,
    MP_QSTR_mpyfile,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    print, mpyfile_print,
    protocol, &mpyfile_stream_p,
    locals_dict, &mpyfile_locals_dict
    );

static mp_obj_t mpy_open(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_encoding, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
    const char *path = mp_obj_str_get_str(args[0]);
    const char *mode = "r";
    if (vals[0].u_obj != mp_const_none) {
        mode = mp_obj_str_get_str(vals[0].u_obj);
    }
    void *h = mpy_fs_fopen(path, mode);
    if (h == NULL) {
        mp_raise_OSError(MP_ENOENT);
    }
    mp_obj_mpyfile_t *o = mp_obj_malloc(mp_obj_mpyfile_t, &mp_type_mpyfile);
    o->h = h;
    return MP_OBJ_FROM_PTR(o);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mpy_open);

void nlr_jump_fail(void *val) {
    (void)val;
    for (;;) {
    }
}
