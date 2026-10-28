#include "emt36_bin.h"
#include "CPU.h"
#include "CPU_ef.h"
#include "emulator.h"
#include <stdio.h>
#include <string.h>

// Keep state independent of guest snapshots and disk image redirection.
bool no_catch_emt36_on_bin = false;
static bool bin_catch_active;
static char bin_directory[256];

void emt36_bin_disarm(void) {
    bin_catch_active = false;
    bin_directory[0] = 0;
}

void emt36_bin_arm(const char *bin_path) {
    emt36_bin_disarm();
    if (no_catch_emt36_on_bin || !bin_path) return;
    const char *slash = strrchr(bin_path, '\\');
    const char *alt = strrchr(bin_path, '/');
    if (!slash || (alt && alt > slash)) slash = alt;
    if (!slash) return;
    size_t length = (size_t)(slash - bin_path);
    if (length == 0) length = 1; // FAT root directory
    if (length >= sizeof(bin_directory)) return;
    memcpy(bin_directory, bin_path, length);
    bin_directory[length] = 0;
    bin_catch_active = true;
}

static bool guest_read_byte(uint16_t address, uint8_t *byte) {
    TCPU_Arg value = CPU_ReadMemB(address);
    if (CPU_IS_ARG_FAULT(value)) return false;
    *byte = (uint8_t)value;
    return true;
}

static bool guest_write_byte(uint16_t address, uint8_t byte) {
    return !CPU_IS_ARG_FAULT(CPU_WriteMemB(address, byte));
}

static bool guest_write_word(uint16_t address, uint16_t word) {
    return !CPU_IS_ARG_FAULT(CPU_WriteMemW(address, word));
}

// Never write overlay data to ROM or emulated I/O; validate all guest pages
// before reading file data (writes are performed through the memory mapper).
static bool guest_ram_range(uint32_t address, uint32_t length) {
    if (address + length > CPU_START_IO_ADR) return false;
    if (!length) return true;
    uint32_t last = address + length - 1;
    for (uint32_t page = address >> 12; page <= (last >> 12); ++page) {
        uintptr_t base = Device_Data.MemPages[page];
        if (base < (uintptr_t)RAM || base >= (uintptr_t)(RAM + RAM_SIZE))
            return false;
    }
    return true;
}

static bool cassette_name(uint16_t addr, char out[17]) {
    for (unsigned i = 0; i < 16; ++i) {
        uint8_t c;
        if (!guest_read_byte((uint16_t)(addr + i), &c)) return false;
        if (!c) {
            out[i] = 0;
            break;
        }
        if (c < 32 || c > 126 || c == '/' || c == '\\' || c == ':')
            return false;
        out[i] = (char)c;
        out[i + 1] = 0;
    }
    size_t n = strlen(out);
    while (n && out[n - 1] == ' ') out[--n] = 0;
    return n && strcmp(out, ".") && strcmp(out, "..");
}

static bool open_overlay(FIL *file, const char *name) {
    char full[280];
    const char *suffix[] = { "", ".OVL", ".BIN" };
    unsigned count = strchr(name, '.') ? 1 : 3;
    for (unsigned i = 0; i < count; ++i) {
        int n = snprintf(full, sizeof(full), "%s%s%s%s", bin_directory,
                         bin_directory[strlen(bin_directory) - 1] == '\\' ? "" : "\\",
                         name, suffix[i]);
        if (n < 0 || (size_t)n >= sizeof(full)) return false;
        if (f_open(file, full, FA_READ) == FR_OK) return true;
    }
    return false;
}

bool emt36_bin_handle(uint16_t param_addr) {
    if (!bin_catch_active || no_catch_emt36_on_bin || (param_addr & 1) ||
        param_addr > CPU_START_IO_ADR - 42) return false;

    uint8_t command;
    if (!guest_read_byte(param_addr, &command) || command != 3)
        return false; // stop/start/write/dummy-read: always the original ROM

    char name[17] = { 0 };
    if (!cassette_name((uint16_t)(param_addr + 6), name)) return false;

    FIL file;
    if (!open_overlay(&file, name)) return false; // native cassette fallback

    uint8_t header[4];
    UINT got = 0;
    FRESULT res = f_read(&file, header, sizeof(header), &got);
    if (res != FR_OK || got != sizeof(header)) {
        f_close(&file);
        return false;
    }
    uint16_t file_addr = (uint16_t)(header[0] | ((uint16_t)header[1] << 8));
    uint16_t length = (uint16_t)(header[2] | ((uint16_t)header[3] << 8));
    uint8_t lo, hi;
    if (!guest_read_byte((uint16_t)(param_addr + 2), &lo) ||
        !guest_read_byte((uint16_t)(param_addr + 3), &hi)) {
        f_close(&file);
        return false;
    }
    uint32_t destination = ((uint16_t)(lo | ((uint16_t)hi << 8)));
    if (!destination) destination = file_addr;
    if (f_size(&file) < (FSIZE_t)length + 4 ||
        !guest_ram_range(destination, length)) {
        f_close(&file);
        return false;
    }

    // No temporary BIN-sized allocation; guest pages can be banked.
    uint8_t buffer[256];
    uint32_t written = 0;
    while (written < length) {
        UINT amount = (UINT)(length - written);
        if (amount > sizeof(buffer)) amount = sizeof(buffer);
        res = f_read(&file, buffer, amount, &got);
        if (res != FR_OK || got != amount) break;
        for (UINT i = 0; i < amount; ++i) {
            if (!guest_write_byte((uint16_t)(destination + written + i), buffer[i])) {
                res = FR_DISK_ERR;
                break;
            }
        }
        if (res != FR_OK) break;
        written += amount;
    }
    f_close(&file);
    if (written != length) {
        // As on STOP/aborted cassette read: report failure, do not jump to ROM.
        guest_write_byte((uint16_t)(param_addr + 1), 4);
        return true;
    }

    // ROM cassette driver output: current tape block and last loaded range.
    guest_write_word((uint16_t)(param_addr + 22), file_addr);
    guest_write_word((uint16_t)(param_addr + 24), length);
    for (unsigned i = 0; i < 16; ++i) {
        char c = name[i];
        guest_write_byte((uint16_t)(param_addr + 26 + i), c ? (uint8_t)c : ' ');
        if (!c) {
            for (++i; i < 16; ++i)
                guest_write_byte((uint16_t)(param_addr + 26 + i), ' ');
            break;
        }
    }
    guest_write_word(0264, (uint16_t)destination);
    guest_write_word(0266, length);
    guest_write_byte((uint16_t)(param_addr + 1), 0);
    return true;
}
