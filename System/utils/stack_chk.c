#include <stdint.h>

// GCC -fstack-protector-strong 支持符号（裸机无 libssp）
uintptr_t __stack_chk_guard = 0x0badc0de;

void __attribute__((noreturn)) __stack_chk_fail(void) {
    while (1) {}
}