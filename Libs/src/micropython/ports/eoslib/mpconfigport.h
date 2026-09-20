/*
 * HP39GII (STMP3770 / ARM926EJ-S) MicroPython 端口配置
 * 2026-09-20：独立 Python app 工程（MicroPython v1.29.0）
 * 功能范围以硬件为准：无网络/蓝牙/线程/GPIO 等无硬件支撑的模块
 */
#include <stdint.h>

// 功能等级：EXTRA（含 math/cmath/random/struct/array/collections/io/time）
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)

#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_ENABLE_GC (1)
#define MICROPY_HELPER_REPL (1)
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
#define MICROPY_PY_URE (0)
#define MICROPY_PY_UZLIB (0)
#define MICROPY_PY_UHASHLIB (0)
#define MICROPY_PY_UCRYPTOLIB (0)
#define MICROPY_PY_FRAMEBUF (0)

// sys 模块基础内容
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
