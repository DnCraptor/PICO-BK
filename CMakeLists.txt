cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

#set (MURM2 ON)

SET(PROJECT_NAME p1-bk)
set(FLASH_SIZE 2048)

IF(MURM2)
    SET(BUILD_NAME "m2${PROJECT_NAME}")
ELSE()
    SET(BUILD_NAME "m1${PROJECT_NAME}")
ENDIF()

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

project(pico-bk C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(drivers/nespad)
add_subdirectory(drivers/ps2)
add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/psram)
add_subdirectory(drivers/vga-nextgen)

# INCLUDE FILES THAT SHOULD BE COMPILED:
file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c" "soft/EmuAPP/src/*.c" "soft/EmuAPP/src/Overlays/Init/*.c"
#        "drivers/usb/*.c"
)

message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_executable(${PROJECT_NAME} ${SRC})

family_configure_device_example(${PROJECT_NAME} noos)

# pull in common dependencies
target_link_libraries(pico-bk PRIVATE
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
#    stdc++
)

target_include_directories(${PROJECT_NAME} PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/src
#            ${CMAKE_CURRENT_SOURCE_DIR}/src/imgos
            ${CMAKE_CURRENT_SOURCE_DIR}/disks
            ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb
            ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src
            ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/Emu
            ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/Init
            ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/EmuFfs
            ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/ROM
#            ${CMAKE_CURRENT_SOURCE_DIR}/assets
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

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(pico-bk pico_cyw43_arch_none)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

# add url via pico_set_program_url
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)
target_compile_definitions(${PROJECT_NAME} PRIVATE FLASH_SIZE=${FLASH_SIZE})

option(OVERCLOCKING "Target Pico MHz" 366)
if(NOT OVERCLOCKING)
    SET(OVERCLOCKING 366)
endif ()

#if (PICO_BOARD == "pico")
    pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
    target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
    pico_set_boot_stage2(${PROJECT_NAME} slower_boot2)
#endif ()

if (OVERCLOCKING)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${OVERCLOCKING}MHz")
    target_compile_definitions(${PROJECT_NAME} PRIVATE OVERCLOCKING=${OVERCLOCKING})
    message(STATUS "==================================")
    message(STATUS "Overclocking ${OVERCLOCKING}MHz enabled")
    message(STATUS "==================================")
    message(STATUS "")
endif ()

target_sources(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb/usb.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb/msc_disk.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb/usb_descriptors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/Init/CPU_i.c
)

IF(MURM2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_HEAP_SIZE=0
        USE_ZONE_FOR_MALLOC=1

        PICO_DEFAULT_UART_RX_PIN=-1
        PICO_DEFAULT_UART_TX_PIN=2

        KBD_CLOCK_PIN=2
        KBD_DATA_PIN=3

        # SDCARD
        SDCARD_PIN_SPI0_CS=5
        SDCARD_PIN_SPI0_SCK=2
        SDCARD_PIN_SPI0_MOSI=3
        SDCARD_PIN_SPI0_MISO=4

        beginVGA_PIN=12

        # NES Gamepad
        USE_NESPAD
        NES_GPIO_CLK=20
        NES_GPIO_LAT=21
        NES_GPIO_DATA=26

        USE_WII
        WII_SDA_PIN=20
        WII_SCL_PIN=21

       # PSRAM
        # PSRAM_MUTEX=1
       # PSRAM_SPINLOCK=1
       # PSRAM_ASYNC=1

       # PSRAM_PIN_CS=18
       # PSRAM_PIN_SCK=19
       # PSRAM_PIN_MOSI=20
       # PSRAM_PIN_MISO=21

        LOAD_WAV_PIO=22
        LOAD_WAV_2_COVOX=1
        BEEPER_PIN=9
        PWM_PIN0=10
        PWM_PIN1=11

        BUILD_IN_FDD0
        BUILD_IN_FDD1

        BK_FDD_16K=1

        # supports only for pico with 4 or more MB flashes
#        PICO_FLASH_SIZE_BYTES=4194304

        #INVALID_DEBUG=1
        #BOOT_DEBUG=1
        #BOOT_DEBUG_ACC=0xA0000
        #KBD_DEBUG=1
        #MNGR_DEBUG=1
        #DSK_DEBUG=1

        EXT_DRIVES_MOUNT=1

        SOUND_SYSTEM
        COVOX
        AYSOUND

        # select onr of the folowing:
        #SWAP_BLOCK_1k
        SWAP_BLOCK_2k
        #SWAP_BLOCK_4k
    )
ELSE()
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_HEAP_SIZE=0
        USE_ZONE_FOR_MALLOC=1

        PICO_DEFAULT_UART_RX_PIN=-1
        PICO_DEFAULT_UART_TX_PIN=0

        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1

        # SDCARD
        SDCARD_PIN_SPI0_CS=5
        SDCARD_PIN_SPI0_SCK=2
        SDCARD_PIN_SPI0_MOSI=3
        SDCARD_PIN_SPI0_MISO=4

        beginVGA_PIN=6

        # NES Gamepad
        USE_NESPAD
        NES_GPIO_CLK=14
        NES_GPIO_DATA=16
        NES_GPIO_LAT=15

        USE_WII
        WII_SDA_PIN=14
        WII_SCL_PIN=15

       # PSRAM
        # PSRAM_MUTEX=1
       # PSRAM_SPINLOCK=1
       # PSRAM_ASYNC=1

       # PSRAM_PIN_CS=18
       # PSRAM_PIN_SCK=19
       # PSRAM_PIN_MOSI=20
       # PSRAM_PIN_MISO=21

        LOAD_WAV_PIO=22
        LOAD_WAV_2_COVOX=1
        PWM_PIN0=26
        PWM_PIN1=27
        BEEPER_PIN=28

        BUILD_IN_FDD0
       # BUILD_IN_FDD1

        BK_FDD_16K=1

        # supports only for pico with 4 or more MB flashes
#        PICO_FLASH_SIZE_BYTES=4194304

        #INVALID_DEBUG=1
        #BOOT_DEBUG=1
        #BOOT_DEBUG_ACC=0xA0000
        #KBD_DEBUG=1
        #MNGR_DEBUG=1
        #DSK_DEBUG=1

        EXT_DRIVES_MOUNT=1

        SOUND_SYSTEM
        COVOX
        AYSOUND

        # select onr of the folowing:
        #SWAP_BLOCK_1k
        SWAP_BLOCK_2k
        #SWAP_BLOCK_4k
    )
ENDIF()

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}-VGA-PWM-${OVERCLOCKING}MHz")
