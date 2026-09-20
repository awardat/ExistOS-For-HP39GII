
# HP39GII MicroPython 端口（eoslib）
# 2026-09-20：M1——编译为 libmpy.a（由 System/CMakeLists 链接）
# 用法：cd Libs/src/micropython/ports/eoslib && make

include ../../py/mkenv.mk

CROSS_COMPILE ?= arm-none-eabi-

# qstr 定义（必须早于 py.mk）
QSTR_DEFS = qstrdefsport.h

# MicroPython 特性配置
MICROPY_ROM_TEXT_COMPRESSION ?= 0

# 包含 py 核心构建定义
include $(TOP)/py/py.mk

# extmod 模块源码（须在 OBJ 组装前 include；各文件按 mpconfig 内的开关自动裁剪）
include $(TOP)/extmod/extmod.mk

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)

CFLAGS += $(INC) -Wall -std=c99 -Wno-unused-function
CFLAGS += -mtune=arm926ej-s -mcpu=arm926ej-s -marm -msoft-float -mfloat-abi=soft
CSUPEROPT = -Os

# 优化等级（DEBUG=1 用 -Og 便于定位）
DEBUG = 0
ifeq ($(DEBUG), 1)
CFLAGS += -Og -g
else
CFLAGS += -Os -DNDEBUG -fdata-sections -ffunction-sections
endif

LIBS =

SRC_C = \
	mpy_port.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	shared/readline/readline.c

SRC_QSTR += mpy_port.c shared/runtime/pyexec.c shared/readline/readline.c

OBJ += $(PY_O)  # PY_O = py 核心 + extmod（py.mk 约定）
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))

LIBSDIR = ../../../..

all: $(LIBSDIR)/libmpy.a

$(LIBSDIR)/libmpy.a: $(BUILD)/libmpy.a
	$(ECHO) "CP $@"
	$(Q)cp $< $@

$(BUILD)/libmpy.a: $(OBJ)
	$(ECHO) "AR $@"
	$(Q)$(AR) -rcs $@ $^
	$(Q)$(SIZE) $@

# 包含核心 make 规则
include $(TOP)/py/mkrules.mk
