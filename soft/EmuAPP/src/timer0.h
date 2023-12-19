#ifndef TIMER0_H
#define TIMER0_H


#include "ets.h"

#define ETS_CCOMPARE0_INUM  6

#include "hardware/structs/systick.h"

// #define timer0_write( count) __asm__ __volatile__("wsr %0,ccompare0; esync"::"a" (count) : "memory")
static inline uint64_t getCycleCount() {
    uint64_t ccount;//systick_hw->csr;
   // __asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));
    return time_us_64();
}

// timer0 is a special CPU timer that has very high resolution but with
// limited control.
// it uses CCOUNT (ESP.GetCycleCount()) as the non-resetable timer counter
// it does not support divide, type, or reload flags
// it is auto-disabled when the compare value matches CCOUNT
// it is auto-enabled when the compare value changes

#endif
