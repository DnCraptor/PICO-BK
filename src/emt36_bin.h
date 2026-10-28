#pragma once
#include <stdbool.h>
#include <stdint.h>

// File manager's BIN-only cassette redirect.  No effect on normal ROM EMTs.
extern bool no_catch_emt36_on_bin; // persisted in /BK/bk.conf, default false
void emt36_bin_arm(const char *bin_path);
void emt36_bin_disarm(void);
bool emt36_bin_handle(uint16_t param_addr); // true: EMT serviced, skip ROM vector
