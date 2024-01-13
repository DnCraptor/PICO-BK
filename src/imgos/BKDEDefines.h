#pragma once

#ifndef BKDEDEFINES_H
#define BKDEDEFINES_H

#define _MAX_PATH   260 // max. length of full pathname
#define _MAX_EXT    256 // max. length of extension component

constexpr auto DEFAULT_DPIX = 96;
constexpr auto DEFAULT_DPIY = 96;

constexpr auto LC_FNAME_ST_COL_WIDTH = 200;
constexpr auto LC_SIZE_ST_COL_WIDTH = 70;
constexpr auto LC_OSTYPE_ST_COL_WIDTH = 70;
constexpr auto LC_SYSTYPE_ST_COL_WIDTH = 76;


constexpr auto LC_FNAME_COL_WIDTH = 200;
constexpr auto LC_TYPE_COL_WIDTH = 70;
constexpr auto LC_BLKSZ_COL_WIDTH = 50;
constexpr auto LC_ADDRESS_COL_WIDTH = 60;
constexpr auto LC_SIZE_COL_WIDTH = 80;
constexpr auto LC_ATTR_COL_WIDTH = 70;
constexpr auto LC_SPECIFIC_COL_WIDTH = 108;

// биты-признаки флагов разрешения контролов
constexpr uint32_t ENABLE_BUTON_EXTRACT = 0x0001;
constexpr uint32_t ENABLE_BUTON_VIEW    = 0x0002;
constexpr uint32_t ENABLE_BUTON_VIEWSPR = 0x0004;
constexpr uint32_t ENABLE_BUTON_ADD     = 0x0008;
constexpr uint32_t ENABLE_BUTON_DEL     = 0x0010;
constexpr uint32_t ENABLE_BUTON_REN     = 0x0020;
// и действий
constexpr uint32_t ENABLE_CONTEXT_CHANGEADDR    = 0x0100;

#endif
