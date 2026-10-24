cmake_minimum_required(VERSION 3.16)
# orange branch: Linux ARM64 port (Orange Pi Zero 2W) - DRM/KMS video,
# ALSA sound, evdev keyboards and game controllers.  The RP2350 firmware
# is built from the master branch.
project(bk VERSION 1.5.3 LANGUAGES C CXX)

set(BUILD_VERSION "1.5.3")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
find_package(Threads REQUIRED)

add_executable(bk
    # Emulator (BK-0010/11M core, disks, sound, manager)
    src/main.cpp
    src/manager.c
    src/pico-vision.c
    src/aySoundSoft.c
    src/disk.c
    src/fdd.c
    src/disk_img.cpp
    src/mkdos.cpp
    soft/EmuAPP/src/AnyMem.c
    soft/EmuAPP/src/CPU.c
    soft/EmuAPP/src/Key.c
    soft/EmuAPP/src/reboot.c
    soft/EmuAPP/src/Overlays/Emu/CPU_e.c
    soft/EmuAPP/src/Overlays/Emu/emu_e.c
    soft/EmuAPP/src/Overlays/EmuFfs/CPU_ef.c
    soft/EmuAPP/src/Overlays/EmuUi/Key_eu.c
    soft/EmuAPP/src/Overlays/Init/CPU_i.c
    soft/EmuAPP/src/Overlays/Init/crc8_i.c
    soft/EmuAPP/src/Overlays/Init/main_i.c

    # Linux host (counterparts of the RP2350 drivers)
    src/linux/vga_linux.c       # VGA scanline renderer (drivers/vga-nextgen)
    src/linux/video_drm.c       # DRM/KMS output
    src/linux/audio_linux.c     # ALSA output (audio timer, PWM/I2S)
    src/linux/input_linux.c     # evdev keyboards and gamepads
    src/linux/kbd_linux.cpp     # keys -> scancodes (drivers/ps2kbd)
    src/linux/nespad_linux.c    # NES (Dendy) pads (drivers/nespad)
    src/linux/ff_posix.c        # FatFs API over the data directory (SD card)
    src/linux/platform_linux.c  # timers, reboot / quit
)

# src/linux first: its pico.h, pico/*.h, hardware/*.h, ff.h and f_util.h
# replace the Pico SDK and SD card driver headers.
target_include_directories(bk PRIVATE
    src/linux
    src
    disks
    drivers/vga-nextgen
    drivers/ps2kbd
    drivers/nespad
    soft/EmuAPP/src
    soft/EmuAPP/src/Overlays/Emu
    soft/EmuAPP/src/Overlays/Init
    soft/EmuAPP/src/Overlays/EmuFfs
    soft/EmuAPP/ROM
)

target_compile_definitions(bk PRIVATE
    _POSIX_C_SOURCE=200809L
    _DEFAULT_SOURCE=1
    ORANGE_BK=1
    PICO_ON_DEVICE=1
    PICO_RP2350=1

    LOAD_WAV_2_COVOX=1

    BK_FDD_16K=1
    BUILD_IN_FDD_MKDOS
    BUILD_IN_FDD_GAMES

    EXT_DRIVES_MOUNT=1

    SOUND_SYSTEM=1
    COVOX=1
    AYSOUND=1

    SWAP_BLOCK_2k

    USE_NESPAD
    BEEPER_PIN=9
    PICO_DEFAULT_LED_PIN=25
)

target_compile_options(bk PRIVATE
    -Wall
    -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

target_link_libraries(bk PRIVATE PkgConfig::LIBDRM PkgConfig::ALSA Threads::Threads)
install(TARGETS bk RUNTIME DESTINATION bin)

# Debian binary package. Dependencies are derived from the actual linked ELF
# on the build machine (e.g. libdrm2 and the distribution's ALSA library).
install(FILES README.md LICENSE DESTINATION share/doc/pico-bk)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "pico-bk")
set(CPACK_PACKAGE_VENDOR "DnCraptor")
set(CPACK_PACKAGE_CONTACT "DnCraptor")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "BK-0010/11M emulator using Linux DRM/KMS and ALSA")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/DnCraptor/PICO-BK")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_STRIP_FILES ON)
include(CPack)
