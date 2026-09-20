/*
 * HP39GII (STMP3770 / ARM926EJ-S) MicroPython 端口配置
 * 2026-09-20：独立 Python app 工程（MicroPython v1.29.0）
 * 功能范围以硬件为准：无网络/蓝牙/线程/GPIO 等无硬件支撑的模块
 */
#include <stdint.h>

// 功能等级：EXTRA（含 math/cmath/random/struct/array/collections/io/time）
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)

#define MICROPY_USE_INTERNAL_PRINTF (0) // 用 newlib printf（MP 内置版不支持 %*s，会覆盖系统 UI 的 vsnprintf）
#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_ENABLE_GC (1)
#define MICROPY_HELPER_REPL (1)
#define MICROPY_REPL_EVENT_DRIVEN (1) // App 侧逐字符喂入（终端式 REPL）
#define MICROPY_ENABLE_EXTERNAL_IMPORT (0) // v1：外部 .py 导入后续由 App 层提供
#define MICROPY_ALLOC_PATH_MAX (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (16)

// 浮点：软件 double（硬件无 FPU）
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_DOUBLE)

// 端口无硬件支撑/不需要的模块一律关闭
#define MICROPY_PY_MACHINE (0)
#define MICROPY_PY_NETWORK (0)
#define MICROPY_PY_SOCKET (0)
#define MICROPY_PY_SSL (0)
#define MICROPY_PY_BLUETOOTH (0)
#define MICROPY_PY_THREAD (0)
#define MICROPY_PY_ASYNCIO (0)
#define MICROPY_PY_WEBREPL (0)
#define MICROPY_PY_UWEBSOCKET (0)
#define MICROPY_PY_ONEWIRE (0)
// 依赖 lib/ 子模块或非计划功能的模块（v1 关闭；后续按需启用）
#define MICROPY_PY_RE (0)
#define MICROPY_PY_BINASCII (0)
#define MICROPY_PY_DEFLATE (0)
#define MICROPY_PY_HASHLIB (0)
#define MICROPY_PY_UCTYPES (0)
#define MICROPY_PY_JSON (0)
#define MICROPY_PY_FRAMEBUF (0)

// sys 模块基础内容（无 VFS：关闭 stdio 文件对象，REPL 直接走 mp_hal_stdout）
#define MICROPY_PY_SYS_STDFILES (0)
#define MICROPY_PY_SYS_STDIO_BUFFER (0)
#define MICROPY_PY_SYS_MODULES (1)
#define MICROPY_PY_SYS_EXIT (1)
#define MICROPY_PY_SYS_PATH (1)
#define MICROPY_PY_SYS_ARGV (0)

// 类型定义
typedef long mp_off_t;

// 需要 alloca 声明
#include <alloca.h>

#define MICROPY_HW_BOARD_NAME "HP39GII"
#define MICROPY_HW_MCU_NAME "STMP3770 (ARM926EJ-S)"

#define MP_STATE_PORT MP_STATE_VM
