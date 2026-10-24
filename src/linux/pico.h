#ifndef ORANGE_BK_PICO_H
#define ORANGE_BK_PICO_H

/*
 * Linux host replacement for the Pico SDK <pico.h>.
 *
 * The emulator sources annotate hot functions and data with RP2350 section
 * placement macros (SRAM, scratch banks, uninitialized RAM).  On the ARM64
 * host every function and object lives in ordinary process memory, so the
 * annotations reduce to the bare declaration.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define __not_in_flash(group)
#define __in_flash(group)
#define __not_in_flash_func(func_name) func_name
#define __no_inline_not_in_flash_func(func_name) __attribute__((noinline)) func_name
#define __time_critical_func(func_name) func_name
#define __scratch_x(group)
#define __scratch_y(group)
#define __uninitialized_ram(name) name
#define __isr

#ifndef __aligned
#define __aligned(n) __attribute__((aligned(n)))
#endif

#ifndef __unreachable
#define __unreachable() __builtin_unreachable()
#endif

#ifndef count_of
#define count_of(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MIN
#define MIN(a, b) ((b) < (a) ? (b) : (a))
#endif

typedef unsigned int uint;

#ifdef __cplusplus
extern "C" {
#endif
/* newlib (RP2350 toolchain) has the BSD strnstr(); glibc does not
 * (implemented in platform_linux.c). */
char *strnstr(const char *haystack, const char *needle, size_t len);
#ifdef __cplusplus
}
#endif

/* Busy-wait loop body (SDK: a NOP on the Cortex-M33). */
static inline void tight_loop_contents(void) {}

#endif /* ORANGE_BK_PICO_H */
