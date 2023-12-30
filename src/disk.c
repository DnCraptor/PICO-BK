//
// Created by xrip on 23.10.2023.
//
#include "emulator.h"

uint8_t bootdrive, hdcount, fdcount;

#define _FILE FIL
_FILE fileA;
_FILE fileB;
_FILE fileC;
_FILE fileD;
_FILE * getFileA() { return &fileA; }
_FILE * getFileB() { return &fileB; }
_FILE * getFileC() { return &fileC; }
_FILE * getFileD() { return &fileD; }
size_t getFileA_sz() { return fileA.obj.fs ? f_size(&fileA) : 0; }
size_t getFileB_sz() { return fileB.obj.fs ? f_size(&fileB) : 0; }
size_t getFileC_sz() { return fileC.obj.fs ? f_size(&fileC) : 0; }
size_t getFileD_sz() { return fileD.obj.fs ? f_size(&fileD) : 0; }
#if BOOT_DEBUG || KBD_DEBUG || MNGR_DEBUG || DSK_DEBUG
_FILE fileD; // for debug output
#endif

size_t size_of_drive(uint8_t drive) {
    switch (drive)
    {
    case 0:
        return getFileA_sz();
    case 1:
        return getFileB_sz();
    case 2:
        return getFileC_sz();
    case 3:
        return getFileD_sz();
    }
    return 0;
}

uint16_t word_of_drive(uint8_t drive, size_t pos) {
    _FILE * f;
    switch (drive)
    {
    case 0:
        f = getFileA();
        break;
    case 1:
        f = getFileB();
        break;
    case 2:
        f = getFileC();
        break;
    case 3:
        f = getFileD();
        break;
    default:
        return 0;
    }
    f_lseek(f, pos); // TODO: FRES
    uint16_t res;
    UINT br;
    f_read(f, &res, 2, &br); // TODO: FRES
    return res;
}

struct struct_drive {
    _FILE *diskfile;
    size_t filesize;
    uint16_t cyls;
    uint16_t sects;
    uint16_t heads;
    uint8_t inserted;
    uint8_t readonly;
    char *data;
};

struct struct_drive disk[4];

static uint8_t sectorbuffer[512];

void ejectdisk(uint8_t drivenum) {
    if (disk[drivenum].inserted) {
        disk[drivenum].inserted = 0;
        if (drivenum >= 2)
            hdcount--;
        else
            fdcount--;
    }
}

static _FILE* actualDrive(uint8_t drivenum) {
    switch (drivenum)
    {
    case 0:
        return getFileA();
    case 1:
        return getFileB();
    case 2:
        return getFileC();
    case 3:
        return getFileD();
    }
    return 0;
}

static _FILE* tryFlushROM(uint8_t drivenum, size_t size, char *ROM, char *path) {
    _FILE *pFile = actualDrive(drivenum);
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    FRESULT result = f_open(pFile, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        return NULL;
    }
    UINT bw;
    result = f_write(pFile, ROM, size, &bw);
    if (result != FR_OK) {
        return NULL;
    }
    f_close(pFile);
    sleep_ms(33); // TODO: ensure
    result = f_open(pFile, path, FA_READ | FA_WRITE);
    if (FR_OK != result) {
        return NULL;
    }
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return pFile;
}

static _FILE* tryDefaultDrive(uint8_t drivenum, size_t size, char *path) {
    _FILE *pFile = actualDrive(drivenum);
    FRESULT result = f_open(pFile, path, FA_WRITE | FA_CREATE_ALWAYS);
    if (result != FR_OK) {
        return NULL;
    }
    result = f_lseek(pFile, size);
    if (result != FR_OK) {
        return NULL;
    }
    f_close(pFile);
    sleep_ms(33); // TODO: ensure
    result = f_open(pFile, path, FA_READ | FA_WRITE);
    if (FR_OK != result) {
        return NULL;
    }
    UINT bw;
    f_write(pFile, fdd0_rom(), fdd0_sz(), &bw);     
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    return pFile;
}

// manager.h
void notify_image_insert_action(uint8_t drivenum, char *pathname);

uint8_t insertdisk(uint8_t drivenum, size_t size, char *ROM, char *pathname) {
    _FILE *pFile = NULL;
    if (pathname != NULL) {
        pFile = actualDrive(drivenum);
        FRESULT result = f_open(pFile, pathname, FA_READ | FA_WRITE);
        if (FR_OK != result) {
            if (size != 0 && ROM != NULL) {
                pFile = tryFlushROM(drivenum, size, ROM, pathname);
                if (!pFile) {
                    pathname = NULL;
                }
            } else {
                pFile = tryDefaultDrive(drivenum, 819200, pathname);
                if (!pFile) {
                    return 1;
                }
            }
        } else {
            size = f_size(pFile);
        }
        notify_image_insert_action(drivenum, pathname);
    }
    const char *err = "?";
    if (size < 360 * 1024) {
        err = "Disk image is too small!";
        goto error;
    }
    if (size > 0x1f782000UL) {
        err = "Disk image is too large!";
        goto error;
    }
    if ((size & 511)) {
        err = "Disk image size is not multiple of 512 bytes!";
        goto error;
    }
    uint16_t cyls, heads, sects;
    if (drivenum >= 2) { //it's a hard disk image
        sects = 63;
        heads = 16;
        cyls = size / (sects * heads * 512);
    } else {   // it's a floppy image (1.44 МВ)
        cyls = 80;
        sects = 18;
        heads = 2;
        if (size <= 1228800) { // 1.2 MB
            sects = 15;
        }
        if (size <= 819200) { // 800 KB
            sects = 10;
        }
        if (size <= 737280) { // 720 KB
            sects = 9;
        }
        if (size <= 368640) { // 360 KB
            cyls = 40;
            sects = 9;
        }
        if (size <= 163840) { // 160 KB
            cyls = 40;
            sects = 8;
            heads = 1;
        }
    }
    if (cyls > 1023 || cyls * heads * sects * 512 != size) {
        err = "Cannot find some CHS geometry for this disk image file!";
        //goto error;
        // FIXME!!!!
    }
    // Seems to be OK. Let's validate (store params) and print message.
    ejectdisk(drivenum);    // close previous disk image for this drive if there is any
    disk[drivenum].diskfile = pFile;
    disk[drivenum].data = pFile == NULL ? ROM : NULL;
    disk[drivenum].filesize = size;
    disk[drivenum].inserted = true;
    disk[drivenum].readonly = disk[drivenum].data ? true : false;
    disk[drivenum].cyls = cyls;
    disk[drivenum].heads = heads;
    disk[drivenum].sects = sects;
    if (drivenum >= 2)
        hdcount++;
    else
        fdcount++;
    return 0;
error:
    fprintf(stderr, "DISK: ERROR: cannot insert disk 0%02Xh as %s because: %s\r\n", drivenum, "ROM", err);
    return 1;
}

// Call this ONLY if all parameters are valid! There is no check here!
static size_t chs2ofs(int drivenum, int cyl, int head, int sect) {
    return (
        ((size_t)cyl * (size_t)disk[drivenum].heads + (size_t)head) * (size_t)disk[drivenum].sects + (size_t) sect - 1
    ) * 512UL;
}

bool img_disk_read_sec(int drv, BYTE * buffer, LBA_t lba) {
    _FILE * pFile = actualDrive(drv);
    if(FR_OK != f_lseek(pFile, lba * 512)) {
        return false;
    }
    UINT br;
    if(FR_OK != f_read(pFile, buffer, 512, &br)) {
        return false;
    }
    return true;
}

bool img_disk_write_sec(int drv, BYTE * buffer, LBA_t lba) {
    _FILE * pFile = actualDrive(drv);
    if(FR_OK != f_lseek(pFile, lba * 512)) {
        return false;
    }
    UINT bw;
    if(FR_OK != f_write(pFile, buffer, 512, &bw)) {
        return false;
    }
    return true;
}

#if BOOT_DEBUG || KBD_DEBUG || MNGR_DEBUG || DSK_DEBUG
void logFile(char* msg) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    f_open(&fileD, "\\BK\\boot.log", FA_WRITE | FA_OPEN_APPEND);
    UINT bw;
    f_write(&fileD, msg, strlen(msg), &bw);
    f_close(&fileD);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
}
#endif
