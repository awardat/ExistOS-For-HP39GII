#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include "sys_llapi.h"

int _getentropy(void *buffer, size_t length) {

    if (buffer == NULL) {
        errno = EFAULT;
        return -1;
    }

    if (length > 256) {
        errno = EIO;
        return -1;
    }
    unsigned char *buf = (unsigned char *)buffer;

    // Use multiple entropy sources to improve randomness
    static unsigned int seed = 0;
    static int seeded = 0;

    if (!seeded) {
        // Mix time, heap address, and stack address as initial entropy
        seed = ll_rtc_get_sec();
        seed ^= ll_get_time_ms();
        seed ^= (unsigned int)&seed;  // stack address entropy
        if (seed == 0) seed = 0xDEADBEEF;
        seeded = 1;
    }

    for (size_t i = 0; i < length; i++) {
        // LCG with better constants (Numerical Recipes)
        seed = seed * 1664525u + 1013904223u;
        buf[i] = (unsigned char)((seed >> 16) & 0xFF);
    }

    return 0;
}

int _getentropy_r(struct _reent *reent, void *buffer, size_t length) {
    return _getentropy(buffer, length);
}
