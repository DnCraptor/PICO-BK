#pragma once
#include <stdbool.h>
#include <inttypes.h>
#include <pico-vision.h>

#define VIDEORAM_SIZE (MAX_WIDTH * MAX_HEIGHT * BYTES_PER_CHAR)

// --- select only one of 'em
#ifdef SWAP_BLOCK_1k
#define RAM_PAGE_SIZE_KB 1ul
#define RAM_IN_PAGE_ADDR_MASK (0x000003FF)
#endif
// --- select only one of 'em
#ifdef SWAP_BLOCK_2k
#define RAM_PAGE_SIZE_KB 2ul
#define RAM_IN_PAGE_ADDR_MASK (0x000007FF)
#endif
// --- select only one of 'em
#ifdef SWAP_BLOCK_4k
#define RAM_PAGE_SIZE_KB 4ul
#define RAM_IN_PAGE_ADDR_MASK (0x00000FFF)
#endif
// --- select only one of 'em ^

#define RAM_PAGE_SIZE (RAM_PAGE_SIZE_KB * 1024)
#define RAM_SIZE (128ul << 10)

extern uint8_t RAM[RAM_SIZE];
// TODO: cleanup
#define RAM_BLOCKS (RAM_SIZE / RAM_PAGE_SIZE)
extern uint16_t RAM_PAGES[RAM_BLOCKS]; // lba (14-0); 15 - written

#define CONVENTIONAL_END 0xA0000ul

bool init_vram();

#if PICO_ON_DEVICE
uint8_t ram_page_read(uint32_t addr32);
uint16_t ram_page_read16(uint32_t addr32);

void ram_page_write(uint32_t addr32, uint8_t value);
void ram_page_write16(uint32_t addr32, uint16_t value);
#else
static uint8_t ram_page_read(uint32_t addr32);
static uint16_t ram_page_read16(uint32_t addr32);

static void ram_page_write(uint32_t addr32, uint8_t value);
static void ram_page_write16(uint32_t addr32, uint16_t value);

#endif
void read_vram_block(char* dst, uint32_t file_offset, uint32_t sz);
void flush_vram_block(const char* src, uint32_t file_offset, uint32_t sz);

