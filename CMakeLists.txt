# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.22)
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
include(pico_sdk_import.cmake)

project(p2-bk C CXX ASM)

option(AY_TYPE "AY-3-8910 Type: PWM / HWAY" HWAY)
option(MURM2 "Murmulator 2.0" OFF)
option(PICO_PC "RP2040-PICO-PC" OFF)
option(ZERO2 "RP2350-PiZero" OFF)

set(BUILD_VERSION "1.3.6")

set(USE_NESPAD ON)
#set(AY_TYPE HWAY)
#set (m1p2launcher ON)

set (MURM2 ON)
#set (PICO_PC ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

IF(MURM2)
    SET(BUILD_NAME "m2${PROJECT_NAME}")
ELSEIF(PICO_PC)
    SET(BUILD_NAME "PC${PROJECT_NAME}")
ELSEIF(ZERO)
    SET(BUILD_NAME "z1${PROJECT_NAME}")
ELSEIF(ZERO2)
    SET(BUILD_NAME "z2${PROJECT_NAME}")
ELSE()
    SET(BUILD_NAME "m1${PROJECT_NAME}")
ENDIF()

if(NOT AY_TYPE)
    SET(AY_TYPE PWM)
endif ()


set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -ffunction-sections -fdata-sections -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections -O2")

add_subdirectory(drivers/libdvi)
add_subdirectory(drivers/vga-nextgen)
add_subdirectory(drivers/ps2)
add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/psram)
add_subdirectory(drivers/nespad)

# INCLUDE FILES THAT SHOULD BE COMPILED:

file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c" "soft/EmuAPP/src/*.c" "soft/EmuAPP/src/Overlays/Init/*.c")

message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_executable(${PROJECT_NAME} ${SRC})

pico_set_program_name(${PROJECT_NAME} "PICO BK-0010/11 Emulator for Murmulator devboard by DnCraptor")
pico_set_program_version(${PROJECT_NAME} "${BUILD_VERSION}")

# Enable compiler and linker garbage collect unused code/data sections
add_link_options(-Wl,--gc-sections)
target_link_libraries(${PROJECT_NAME} PRIVATE
        libdvi
        vga-nextgen
        ps2
        psram
        sdcard
        fatfs
        nespad

        pico_runtime
        pico_stdlib
        pico_rand
        pico_multicore
        hardware_dma
        hardware_pio
        hardware_i2c
        hardware_interp
        hardware_timer
        hardware_clocks
        hardware_pwm
        hardware_flash
)

target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)

target_sources(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/Init/CPU_i.c
)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/disks
        ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src
        ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/Emu
        ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/Init
        ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/EmuFfs
        ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/ROM
)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PUBLIC
            -Wno-error=suggest-attribute=format
            -Wno-error=cast-qual
            -Wno-error=unused-parameter
            -Wno-error=conversion
            -Wno-error=format=
            -Wno-error=sign-compare
            -Wno-error=missing-field-initializers
            -Wno-error=switch
            -Wno-error=implicit-fallthrough=
            -Wno-error=stringop-truncation
            -Wno-error=restrict
            -w
    )
endif ()

if( AY_TYPE MATCHES "HWAY" )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HWAY)
endif()

if (USE_WII)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_WII)
endif()
if (USE_NESPAD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_NESPAD)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_HEAP_SIZE=0
        USE_ZONE_FOR_MALLOC=1

        PICO_DEFAULT_UART_RX_PIN=-1
        PICO_DEFAULT_UART_TX_PIN=0

        LOAD_WAV_2_COVOX=1

        BK_FDD_16K=1
        BUILD_IN_FDD_MKDOS
        BUILD_IN_FDD_GAMES

        EXT_DRIVES_MOUNT=1

        SOUND_SYSTEM=1
        COVOX=1
        AYSOUND=1

        # select one of the folowing:
        #SWAP_BLOCK_1k
        SWAP_BLOCK_2k
        #SWAP_BLOCK_4k
)

if (MURM2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        KBD_CLOCK_PIN=2
        KBD_DATA_PIN=3
        
        # SDCARD
        SDCARD_PIN_SPI0_MISO=4
        SDCARD_PIN_SPI0_CS=5
        SDCARD_PIN_SPI0_SCK=6
        SDCARD_PIN_SPI0_MOSI=7
        
        BEEPER_PIN=9
        PWM_PIN0=10
        PWM_PIN1=11

        beginVGA_PIN=12
        DVI_DEFAULT_SERIAL_CONFIG=murmulator2_cfg

        WII_SDA_PIN=20
        WII_SCL_PIN=21

        NES_GPIO_CLK=20
        NES_GPIO_LAT=21
        NES_GPIO_DATA=26

        LOAD_WAV_PIO=22
    )
elseif (PICO_PC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_PC=1

        # DBG1 1/2
        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1

        LOAD_WAV_PIO=5 # UXT1-10

        # SDCARD
        SDCARD_PIN_SPI0_MISO=4
        SDCARD_PIN_SPI0_SCK=6
        SDCARD_PIN_SPI0_MOSI=7
        SDCARD_PIN_SPI0_CS=22
        
        BEEPER_PIN=26
        PWM_PIN0=27
        PWM_PIN1=28

        beginVGA_PIN=12
        DVI_DEFAULT_SERIAL_CONFIG=olimex_rp2040_cfg

        WII_SDA_PIN=20
        WII_SCL_PIN=21

        NES_GPIO_CLK=8    # UXT1-6
        NES_GPIO_LAT=9    # UXT1-5
        NES_GPIO_DATA=20  # UXT1-3
        NES_GPIO_DATA2=21 # UXT1-4

        LOAD_WAV_PIO=22
    )
else() # m1
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1
        
        # SDCARD
        SDCARD_PIN_SPI0_CS=5
        SDCARD_PIN_SPI0_SCK=2
        SDCARD_PIN_SPI0_MOSI=3
        SDCARD_PIN_SPI0_MISO=4
        
        beginVGA_PIN=6
        DVI_DEFAULT_SERIAL_CONFIG=murmulator_cfg

        WII_SDA_PIN=14
        WII_SCL_PIN=15

        NES_GPIO_CLK=14
        NES_GPIO_LAT=15
        NES_GPIO_DATA=16

        LOAD_WAV_PIO=22

        PWM_PIN0=26
        PWM_PIN1=27
        BEEPER_PIN=28
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_CORE1_STACK_SIZE=0x200
)

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

SET(BUILD_NAME "${BUILD_NAME}-VGA-HDMI-400MHz-${AY_TYPE}-${BUILD_VERSION}")
IF(m1p2launcher)
    SET(BUILD_NAME "${BUILD_NAME}.m1p2")
    pico_set_linker_script(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/memmap.ld")
ENDIF()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")

pico_add_extra_outputs(${PROJECT_NAME})

if (m1p2launcher)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
        "${OUTPUT_DIR}/${BUILD_NAME}.uf2"
        "${OUTPUT_DIR}/${BUILD_NAME}"
    )
endif ()
