cmake_minimum_required(VERSION 3.22)
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
include(pico_sdk_import.cmake)

project(pico-bk C CXX ASM)
option(OVERCLOCKING "Target Pico MHz" 366)
option(AY_TYPE "AY-3-8910 Type: PWM / HWAY" HWAY)
if(NOT OVERCLOCKING)
    SET(OVERCLOCKING 366)
endif ()
if(NOT AY_TYPE)
    SET(AY_TYPE HWAY)
endif ()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -ffunction-sections -fdata-sections -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections -O2")

SET(BUILD_NAME "m1p1-bk-VGA-${AY_TYPE}-${OVERCLOCKING}MHz-133")

#add_compile_definitions(${PROJECT_NAME} NO_GRAPHICS)
set(FAMILY rp2040)
set(BOARD pico_sdk)
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

pico_set_program_name(pico-bk "PICO BK-0010/11 Emulator for ZX Murmulator devboard by DnCraptor")
pico_set_program_version(pico-bk "test")

#set(PICO_DEOPTIMIZED_DEBUG 1)

    if (OVERCLOCKING)
        set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${OVERCLOCKING}MHz")

        target_compile_definitions(${PROJECT_NAME} PRIVATE OVERCLOCKING=${OVERCLOCKING})

        pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
        target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
        pico_set_boot_stage2(${PROJECT_NAME} slower_boot2)

        message(STATUS "==================================")
        message(STATUS "Overclocking ${OVERCLOCKING}MHz enabled")
        message(STATUS "==================================")
        message(STATUS "")
    endif ()
    # Enable compiler and linker garbage collect unused code/data sections
    add_link_options(-Wl,--gc-sections)
    target_link_libraries(${PROJECT_NAME} PRIVATE
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
#    target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--script=${CMAKE_CURRENT_LIST_DIR}/memmap.ld")
#    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
    target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)

    family_configure_device_example(${PROJECT_NAME} noos)

    target_sources(${PROJECT_NAME} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb/usb.c
            ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb/msc_disk.c
            ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb/usb_descriptors.c
            ${CMAKE_CURRENT_SOURCE_DIR}/soft/EmuAPP/src/Overlays/Init/CPU_i.c
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

if( AY_TYPE MATCHES "HWAY" )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HWAY)
endif()

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
        
        BK_FDD_16K=1
        BUILD_IN_FDD_MKDOS
        BUILD_IN_FDD_GAMES

        # supports only for pico with 4 or more MB flashes
#        PICO_FLASH_SIZE_BYTES=4194304

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


pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")

