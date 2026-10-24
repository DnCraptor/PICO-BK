#ifndef ORANGE_BK_FF_H
#define ORANGE_BK_FF_H

/*
 * FatFs API (R0.15) implemented over POSIX files - see ff_posix.c
 * (shared with orange386).
 *
 * The emulator reaches the SD card exclusively through FatFs: ROM files,
 * settings, save states and replays.  On Linux the "volume" is a host
 * directory (the data root, by default the working directory), so those
 * sources keep their FatFs calls unchanged.  FatFs semantics kept here:
 *  - paths are relative to the volume root; "0:" drive prefixes,
 *    leading '/' or '\' and '\' separators are accepted;
 *  - name lookup is case-insensitive (exact host name preferred);
 *  - dot entries are never returned by f_readdir();
 *  - f_size()/f_tell()/f_eof() are macros over the FIL object.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int    UINT;
typedef unsigned char   BYTE;
typedef uint16_t        WORD;
typedef uint32_t        DWORD;
typedef uint64_t        QWORD;
typedef WORD            WCHAR;
typedef char            TCHAR;
typedef QWORD           FSIZE_t;   /* FF_FS_EXFAT = 1 */
typedef DWORD           LBA_t;     /* FF_LBA64 = 0 */

#define FF_USE_LFN      1
#define FF_MAX_LFN      255
#define FF_LFN_UNICODE  0
#define FF_LFN_BUF      255
#define FF_SFN_BUF      12
#define FF_MAX_SS       512
#define FF_MIN_SS       512
#define FF_FS_EXFAT     1
#define FF_FS_READONLY  0
#define FF_USE_EXPAND   1
#define FF_FS_RPATH     2
#define FF_VOLUMES      10
#define FF_CODE_PAGE    866

/* Filesystem object (one host directory acts as the mounted volume) */
typedef struct {
	BYTE fs_type;     /* 0: not mounted */
	BYTE pdrv;
	WORD id;
	DWORD csize;      /* "cluster" size in sectors (reported as 8) */
} FATFS;

/* Object ID and allocation information */
typedef struct {
	FATFS *fs;        /* Non-NULL while the object is open */
	WORD id;
	BYTE attr;
	BYTE stat;
	DWORD sclust;
	FSIZE_t objsize;  /* Object size (valid when sclust != 0 or file) */
} FFOBJID;

/* File object (named struct: disk.h forward-declares struct FIL_s) */
typedef struct FIL_s {
	FFOBJID obj;
	BYTE flag;        /* File status flags (FA_*) */
	BYTE err;
	FSIZE_t fptr;     /* File read/write pointer */
	int fd;           /* Host file descriptor */
	BYTE written;     /* Host: data written since last fdatasync */
} FIL;

/* Directory object */
typedef struct {
	FFOBJID obj;
	DWORD dptr;
	void *host;       /* Host DIR * */
	char *path;       /* Host path of the directory */
} DIR;

/* File information */
typedef struct {
	FSIZE_t fsize;
	WORD fdate;
	WORD ftime;
	BYTE fattrib;
	TCHAR altname[FF_SFN_BUF + 1];
	TCHAR fname[FF_LFN_BUF + 1];
} FILINFO;

/* File function return code */
typedef enum {
	FR_OK = 0,
	FR_DISK_ERR,
	FR_INT_ERR,
	FR_NOT_READY,
	FR_NO_FILE,
	FR_NO_PATH,
	FR_INVALID_NAME,
	FR_DENIED,
	FR_EXIST,
	FR_INVALID_OBJECT,
	FR_WRITE_PROTECTED,
	FR_INVALID_DRIVE,
	FR_NOT_ENABLED,
	FR_NO_FILESYSTEM,
	FR_MKFS_ABORTED,
	FR_TIMEOUT,
	FR_LOCKED,
	FR_NOT_ENOUGH_CORE,
	FR_TOO_MANY_OPEN_FILES,
	FR_INVALID_PARAMETER
} FRESULT;

FRESULT f_open(FIL *fp, const TCHAR *path, BYTE mode);
FRESULT f_close(FIL *fp);
FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br);
FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw);
FRESULT f_lseek(FIL *fp, FSIZE_t ofs);
FRESULT f_truncate(FIL *fp);
FRESULT f_sync(FIL *fp);
FRESULT f_opendir(DIR *dp, const TCHAR *path);
FRESULT f_closedir(DIR *dp);
FRESULT f_readdir(DIR *dp, FILINFO *fno);
FRESULT f_mkdir(const TCHAR *path);
FRESULT f_unlink(const TCHAR *path);
FRESULT f_rename(const TCHAR *path_old, const TCHAR *path_new);
FRESULT f_stat(const TCHAR *path, FILINFO *fno);
FRESULT f_mount(FATFS *fs, const TCHAR *path, BYTE opt);
FRESULT f_expand(FIL *fp, FSIZE_t fsz, BYTE opt);

#define f_eof(fp) ((int)((fp)->fptr == (fp)->obj.objsize))
#define f_error(fp) ((fp)->err)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->obj.objsize)
#define f_rewind(fp) f_lseek((fp), 0)
#define f_rewinddir(dp) f_readdir((dp), 0)
#define f_rmdir(path) f_unlink(path)
#define f_unmount(path) f_mount(0, path, 0)

/* Host extensions */
/* Set the host directory that acts as the volume root (default "."). */
void ff_set_root(const char *dir);
const char *ff_get_root(void);
/* Resolve a FatFs path to an existing host path (case-insensitive).
 * Returns 0 and fills out on success, -1 if no such object exists. */
int ff_host_path(const TCHAR *path, char *out, size_t out_size);

/* File access mode and open method flags (3rd argument of f_open) */
#define FA_READ           0x01
#define FA_WRITE          0x02
#define FA_OPEN_EXISTING  0x00
#define FA_CREATE_NEW     0x04
#define FA_CREATE_ALWAYS  0x08
#define FA_OPEN_ALWAYS    0x10
#define FA_OPEN_APPEND    0x30

/* File attribute bits for directory entry (FILINFO.fattrib) */
#define AM_RDO  0x01
#define AM_HID  0x02
#define AM_SYS  0x04
#define AM_DIR  0x10
#define AM_ARC  0x20

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_BK_FF_H */
