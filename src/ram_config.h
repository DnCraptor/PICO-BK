/**
 * ram_config.h — хранение конфигурации в нестираемой при ресете RAM
 *
 * ВАЖНО: s_ram_cfg определена ОДИН РАЗ в manager.c (не static!).
 *        Здесь только extern-объявление.
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include "config_em.h"
#include <pico/platform.h>

#define RAM_CFG_MAGIC   UINT32_C(0xB10CF60)
#define RAM_CFG_VERSION UINT16_C(3)

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t crc16;

    config_em_t conf;

    bool     is_swap_wins_enabled;
    bool     is_dendy_joystick;
    bool     is_kbd_joystick;
    uint8_t  _pad;

    uint8_t  kbdpad1_A, kbdpad1_B, kbdpad1_START, kbdpad1_SELECT;
    uint8_t  kbdpad1_UP, kbdpad1_DOWN, kbdpad1_LEFT, kbdpad1_RIGHT;
    uint8_t  kbdpad2_A, kbdpad2_B, kbdpad2_START, kbdpad2_SELECT;
    uint8_t  kbdpad2_UP, kbdpad2_DOWN, kbdpad2_LEFT, kbdpad2_RIGHT;
} ram_config_block_t;

/* единственный экземпляр определён в manager.c */
extern ram_config_block_t s_ram_cfg;

extern bool          is_swap_wins_enabled;
extern volatile bool is_dendy_joystick;
extern volatile bool is_kbd_joystick;

extern uint8_t kbdpad1_A, kbdpad1_B, kbdpad1_START, kbdpad1_SELECT;
extern uint8_t kbdpad1_UP, kbdpad1_DOWN, kbdpad1_LEFT, kbdpad1_RIGHT;
extern uint8_t kbdpad2_A, kbdpad2_B, kbdpad2_START, kbdpad2_SELECT;
extern uint8_t kbdpad2_UP, kbdpad2_DOWN, kbdpad2_LEFT, kbdpad2_RIGHT;

static inline uint16_t _ram_cfg_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}

static inline uint16_t _ram_cfg_calc_crc(const ram_config_block_t *b) {
    const uint8_t *start = (const uint8_t *)&b->conf;
    size_t         len   = sizeof(*b) - offsetof(ram_config_block_t, conf);
    return _ram_cfg_crc16(start, len);
}

static inline bool ram_config_valid(void) {
    if (s_ram_cfg.magic   != RAM_CFG_MAGIC)   return false;
    if (s_ram_cfg.version != RAM_CFG_VERSION) return false;
    if (s_ram_cfg.crc16   != _ram_cfg_calc_crc(&s_ram_cfg)) return false;
    return true;
}

static inline void ram_config_save(void) {
    s_ram_cfg.magic   = RAM_CFG_MAGIC;
    s_ram_cfg.version = RAM_CFG_VERSION;
    memcpy(&s_ram_cfg.conf, (const void*)&g_conf, sizeof(config_em_t));

    s_ram_cfg.is_swap_wins_enabled = is_swap_wins_enabled;
    s_ram_cfg.is_dendy_joystick    = (bool)is_dendy_joystick;
    s_ram_cfg.is_kbd_joystick      = (bool)is_kbd_joystick;

    s_ram_cfg.kbdpad1_A = kbdpad1_A; s_ram_cfg.kbdpad1_B = kbdpad1_B;
    s_ram_cfg.kbdpad1_START = kbdpad1_START; s_ram_cfg.kbdpad1_SELECT = kbdpad1_SELECT;
    s_ram_cfg.kbdpad1_UP = kbdpad1_UP; s_ram_cfg.kbdpad1_DOWN = kbdpad1_DOWN;
    s_ram_cfg.kbdpad1_LEFT = kbdpad1_LEFT; s_ram_cfg.kbdpad1_RIGHT = kbdpad1_RIGHT;

    s_ram_cfg.kbdpad2_A = kbdpad2_A; s_ram_cfg.kbdpad2_B = kbdpad2_B;
    s_ram_cfg.kbdpad2_START = kbdpad2_START; s_ram_cfg.kbdpad2_SELECT = kbdpad2_SELECT;
    s_ram_cfg.kbdpad2_UP = kbdpad2_UP; s_ram_cfg.kbdpad2_DOWN = kbdpad2_DOWN;
    s_ram_cfg.kbdpad2_LEFT = kbdpad2_LEFT; s_ram_cfg.kbdpad2_RIGHT = kbdpad2_RIGHT;

    s_ram_cfg.crc16 = _ram_cfg_calc_crc(&s_ram_cfg);
}

static inline bool ram_config_load(void) {
    if (!ram_config_valid()) return false;

    memcpy((void*)&g_conf, &s_ram_cfg.conf, sizeof(config_em_t));

    is_swap_wins_enabled = s_ram_cfg.is_swap_wins_enabled;
    is_dendy_joystick    = s_ram_cfg.is_dendy_joystick;
    is_kbd_joystick      = s_ram_cfg.is_kbd_joystick;

    kbdpad1_A = s_ram_cfg.kbdpad1_A; kbdpad1_B = s_ram_cfg.kbdpad1_B;
    kbdpad1_START = s_ram_cfg.kbdpad1_START; kbdpad1_SELECT = s_ram_cfg.kbdpad1_SELECT;
    kbdpad1_UP = s_ram_cfg.kbdpad1_UP; kbdpad1_DOWN = s_ram_cfg.kbdpad1_DOWN;
    kbdpad1_LEFT = s_ram_cfg.kbdpad1_LEFT; kbdpad1_RIGHT = s_ram_cfg.kbdpad1_RIGHT;

    kbdpad2_A = s_ram_cfg.kbdpad2_A; kbdpad2_B = s_ram_cfg.kbdpad2_B;
    kbdpad2_START = s_ram_cfg.kbdpad2_START; kbdpad2_SELECT = s_ram_cfg.kbdpad2_SELECT;
    kbdpad2_UP = s_ram_cfg.kbdpad2_UP; kbdpad2_DOWN = s_ram_cfg.kbdpad2_DOWN;
    kbdpad2_LEFT = s_ram_cfg.kbdpad2_LEFT; kbdpad2_RIGHT = s_ram_cfg.kbdpad2_RIGHT;

    return true;
}

static inline void ram_config_invalidate(void) {
    s_ram_cfg.magic = 0;
}
