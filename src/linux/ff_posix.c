/*
 * FatFs API over POSIX files (see ff.h).
 *
 * f_sync() starts write-back of the written range without waiting for it
 * (sync_file_range), because emulator storage code may call it after every
 * sector batch, where a blocking fdatasync() on an SD card would stall the
 * emulation for milliseconds.  f_close() performs the blocking fdatasync().
 */
#define _GNU_SOURCE
/* FatFs' DIR collides with POSIX <dirent.h>: name the FatFs one FF_DIR here. */
#define DIR FF_DIR
#include "ff.h"
#undef DIR

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static char ff_root[PATH_MAX] = ".";
static FATFS host_fs = { .fs_type = 4, .pdrv = 0, .id = 1, .csize = 8 };

void ff_set_root(const char *dir)
{
	snprintf(ff_root, sizeof(ff_root), "%s", (dir && *dir) ? dir : ".");
	size_t n = strlen(ff_root);
	while (n > 1 && ff_root[n - 1] == '/')
		ff_root[--n] = 0;
}

const char *ff_get_root(void)
{
	return ff_root;
}

static FRESULT errno_to_fresult(int e)
{
	switch (e) {
	case 0:            return FR_OK;
	case ENOENT:       return FR_NO_FILE;
	case ENOTDIR:      return FR_NO_PATH;
	case EACCES:
	case EPERM:
	case EISDIR:
	case ENOSPC:
	case ENOTEMPTY:
	case EBUSY:        return FR_DENIED;
	case EEXIST:       return FR_EXIST;
	case EROFS:        return FR_WRITE_PROTECTED;
	case EMFILE:
	case ENFILE:       return FR_TOO_MANY_OPEN_FILES;
	case ENAMETOOLONG:
	case EINVAL:       return FR_INVALID_NAME;
	case ENOMEM:       return FR_NOT_ENOUGH_CORE;
	default:           return FR_DISK_ERR;
	}
}

/* Find an entry of dir named name, ignoring case; exact match preferred.
 * On success the real name is copied to found. */
static bool lookup_nocase(const char *dir, const char *name, char *found, size_t n)
{
	char probe[PATH_MAX];
	struct stat st;
	if (snprintf(probe, sizeof(probe), "%s/%s", dir, name) < (int)sizeof(probe) &&
	    lstat(probe, &st) == 0) {
		snprintf(found, n, "%s", name);
		return true;
	}
	DIR *d = opendir(dir);
	if (!d)
		return false;
	bool ok = false;
	struct dirent *de;
	while ((de = readdir(d)) != NULL) {
		if (!strcasecmp(de->d_name, name)) {
			snprintf(found, n, "%s", de->d_name);
			ok = true;
			break;
		}
	}
	closedir(d);
	return ok;
}

/*
 * Map a FatFs path to a host path below the volume root.
 * Returns 0: object exists; 1: only the last component is missing (out holds
 * the name as given, for creation); -1: a directory on the way is missing.
 */
static int resolve(const TCHAR *path, char *out, size_t out_size)
{
	char rel[PATH_MAX];
	const char *p = path ? path : "";
	/* Drive prefix "N:" */
	if (p[0] && p[1] == ':')
		p += 2;
	size_t n = 0;
	for (; *p && n + 1 < sizeof(rel); ++p)
		rel[n++] = (*p == '\\') ? '/' : *p;
	rel[n] = 0;

	/* Split into components, folding "." and "..", never above the root */
	char *comps[PATH_MAX / 2];
	int ncomp = 0;
	char *save = NULL;
	for (char *tok = strtok_r(rel, "/", &save); tok; tok = strtok_r(NULL, "/", &save)) {
		if (!strcmp(tok, "."))
			continue;
		if (!strcmp(tok, "..")) {
			if (ncomp > 0)
				--ncomp;
			continue;
		}
		comps[ncomp++] = tok;
	}

	snprintf(out, out_size, "%s", ff_root);
	for (int i = 0; i < ncomp; ++i) {
		char real[NAME_MAX + 1];
		size_t len = strlen(out);
		if (lookup_nocase(out, comps[i], real, sizeof(real))) {
			snprintf(out + len, out_size - len, "/%s", real);
			continue;
		}
		snprintf(out + len, out_size - len, "/%s", comps[i]);
		return i == ncomp - 1 ? 1 : -1;
	}
	return 0;
}

int ff_host_path(const TCHAR *path, char *out, size_t out_size)
{
	return resolve(path, out, out_size) == 0 ? 0 : -1;
}

static void fill_fileinfo(FILINFO *fno, const char *name, const struct stat *st)
{
	memset(fno, 0, sizeof(*fno));
	fno->fsize = S_ISDIR(st->st_mode) ? 0 : (FSIZE_t)st->st_size;
	struct tm tm;
	time_t mt = st->st_mtime;
	if (localtime_r(&mt, &tm) && tm.tm_year >= 80) {
		fno->fdate = (WORD)(((tm.tm_year - 80) << 9) | ((tm.tm_mon + 1) << 5) | tm.tm_mday);
		fno->ftime = (WORD)((tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec / 2));
	}
	fno->fattrib = S_ISDIR(st->st_mode) ? AM_DIR : AM_ARC;
	if (!(st->st_mode & S_IWUSR))
		fno->fattrib |= AM_RDO;
	if (name[0] == '.')
		fno->fattrib |= AM_HID;
	snprintf(fno->fname, sizeof(fno->fname), "%s", name);

	/* Short name: the name itself when it is a valid upper-case 8.3 name */
	const char *dot = strrchr(name, '.');
	size_t base = dot ? (size_t)(dot - name) : strlen(name);
	size_t ext = dot ? strlen(dot + 1) : 0;
	if (base >= 1 && base <= 8 && ext <= 3 && !strpbrk(name, " +,;=[]")) {
		bool ok = true;
		for (const char *c = name; *c; ++c)
			if ((*c == '.' && c != dot) || (unsigned char)*c < 0x20)
				ok = false;
		if (ok) {
			size_t i = 0;
			for (const char *c = name; *c && i < FF_SFN_BUF; ++c)
				fno->altname[i++] = (char)toupper((unsigned char)*c);
			fno->altname[i] = 0;
		}
	}
}

FRESULT f_mount(FATFS *fs, const TCHAR *path, BYTE opt)
{
	(void)path;
	(void)opt;
	if (fs)
		*fs = host_fs;
	struct stat st;
	if (stat(ff_root, &st) != 0 || !S_ISDIR(st.st_mode))
		return FR_NOT_READY;
	return FR_OK;
}

FRESULT f_open(FIL *fp, const TCHAR *path, BYTE mode)
{
	if (!fp)
		return FR_INVALID_OBJECT;
	memset(fp, 0, sizeof(*fp));
	fp->fd = -1;

	char host[PATH_MAX];
	int r = resolve(path, host, sizeof(host));
	if (r < 0)
		return FR_NO_PATH;

	int flags = (mode & FA_WRITE) ? ((mode & FA_READ) ? O_RDWR : O_WRONLY) : O_RDONLY;
	if (mode & FA_CREATE_NEW)
		flags |= O_CREAT | O_EXCL;
	else if (mode & FA_CREATE_ALWAYS)
		flags |= O_CREAT | O_TRUNC;
	else if (mode & FA_OPEN_ALWAYS)
		flags |= O_CREAT;
	else if (r == 1)
		return FR_NO_FILE;

	int fd = open(host, flags | O_CLOEXEC, 0666);
	if (fd < 0)
		return errno_to_fresult(errno);
	struct stat st;
	if (fstat(fd, &st) != 0) {
		int e = errno;
		close(fd);
		return errno_to_fresult(e);
	}
	if (S_ISDIR(st.st_mode)) {
		close(fd);
		return FR_NO_FILE;
	}
	fp->fd = fd;
	fp->obj.fs = &host_fs;
	fp->obj.objsize = (FSIZE_t)st.st_size;
	fp->flag = mode & (FA_READ | FA_WRITE);
	fp->fptr = ((mode & FA_OPEN_APPEND) == FA_OPEN_APPEND) ? fp->obj.objsize : 0;
	return FR_OK;
}

FRESULT f_close(FIL *fp)
{
	if (!fp || !fp->obj.fs || fp->fd < 0)
		return FR_INVALID_OBJECT;
	FRESULT res = FR_OK;
	if (fp->written && fdatasync(fp->fd) != 0)
		res = FR_DISK_ERR;
	if (close(fp->fd) != 0 && res == FR_OK)
		res = FR_DISK_ERR;
	fp->fd = -1;
	fp->obj.fs = NULL;
	return res;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
	if (br) *br = 0;
	if (!fp || !fp->obj.fs || fp->fd < 0)
		return FR_INVALID_OBJECT;
	if (!(fp->flag & FA_READ))
		return FR_DENIED;
	UINT done = 0;
	while (done < btr) {
		ssize_t n = pread(fp->fd, (uint8_t *)buff + done, btr - done,
				  (off_t)(fp->fptr + done));
		if (n < 0) {
			if (errno == EINTR)
				continue;
			fp->fptr += done;
			if (br) *br = done;
			fp->err = FR_DISK_ERR;
			return FR_DISK_ERR;
		}
		if (n == 0)
			break;
		done += (UINT)n;
	}
	fp->fptr += done;
	if (br) *br = done;
	return FR_OK;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
	if (bw) *bw = 0;
	if (!fp || !fp->obj.fs || fp->fd < 0)
		return FR_INVALID_OBJECT;
	if (!(fp->flag & FA_WRITE))
		return FR_DENIED;
	UINT done = 0;
	FRESULT res = FR_OK;
	while (done < btw) {
		ssize_t n = pwrite(fp->fd, (const uint8_t *)buff + done, btw - done,
				   (off_t)(fp->fptr + done));
		if (n < 0) {
			if (errno == EINTR)
				continue;
			/* Disk full ends the write early with FR_OK, as in FatFs */
			if (errno != ENOSPC) {
				res = FR_DISK_ERR;
				fp->err = FR_DISK_ERR;
			}
			break;
		}
		if (n == 0)
			break;
		done += (UINT)n;
	}
	if (done)
		fp->written = 1;
	fp->fptr += done;
	if (fp->fptr > fp->obj.objsize)
		fp->obj.objsize = fp->fptr;
	if (bw) *bw = done;
	return res;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
	if (!fp || !fp->obj.fs || fp->fd < 0)
		return FR_INVALID_OBJECT;
	if (ofs > fp->obj.objsize) {
		if (!(fp->flag & FA_WRITE)) {
			ofs = fp->obj.objsize;
		} else {
			/* FatFs expands a writable file to the new pointer */
			if (ftruncate(fp->fd, (off_t)ofs) != 0)
				return errno_to_fresult(errno);
			fp->obj.objsize = ofs;
			fp->written = 1;
		}
	}
	fp->fptr = ofs;
	return FR_OK;
}

FRESULT f_truncate(FIL *fp)
{
	if (!fp || !fp->obj.fs || fp->fd < 0)
		return FR_INVALID_OBJECT;
	if (!(fp->flag & FA_WRITE))
		return FR_DENIED;
	if (fp->fptr < fp->obj.objsize) {
		if (ftruncate(fp->fd, (off_t)fp->fptr) != 0)
			return errno_to_fresult(errno);
		fp->obj.objsize = fp->fptr;
		fp->written = 1;
	}
	return FR_OK;
}

FRESULT f_sync(FIL *fp)
{
	if (!fp || !fp->obj.fs || fp->fd < 0)
		return FR_INVALID_OBJECT;
	if (fp->written)
		sync_file_range(fp->fd, 0, 0, SYNC_FILE_RANGE_WRITE);
	return FR_OK;
}

FRESULT f_expand(FIL *fp, FSIZE_t fsz, BYTE opt)
{
	if (!fp || !fp->obj.fs || fp->fd < 0)
		return FR_INVALID_OBJECT;
	if (!(fp->flag & FA_WRITE) || fsz == 0 || fp->obj.objsize != 0 || fp->fptr != 0)
		return FR_DENIED;
	if (!opt)
		return FR_OK;
	int e = posix_fallocate(fp->fd, 0, (off_t)fsz);
	if (e != 0 && ftruncate(fp->fd, (off_t)fsz) != 0)
		return errno_to_fresult(e ? e : errno);
	fp->obj.objsize = fsz;
	fp->written = 1;
	return FR_OK;
}

FRESULT f_opendir(FF_DIR *dp, const TCHAR *path)
{
	if (!dp)
		return FR_INVALID_OBJECT;
	memset(dp, 0, sizeof(*dp));
	char host[PATH_MAX];
	if (resolve(path, host, sizeof(host)) != 0)
		return FR_NO_PATH;
	struct stat st;
	if (stat(host, &st) != 0 || !S_ISDIR(st.st_mode))
		return FR_NO_PATH;
	DIR *d = opendir(host);
	if (!d)
		return errno_to_fresult(errno);
	dp->host = d;
	dp->path = strdup(host);
	if (!dp->path) {
		closedir(d);
		dp->host = NULL;
		return FR_NOT_ENOUGH_CORE;
	}
	dp->obj.fs = &host_fs;
	return FR_OK;
}

FRESULT f_closedir(FF_DIR *dp)
{
	if (!dp || !dp->obj.fs || !dp->host)
		return FR_INVALID_OBJECT;
	closedir((DIR *)dp->host);
	free(dp->path);
	dp->host = NULL;
	dp->path = NULL;
	dp->obj.fs = NULL;
	return FR_OK;
}

FRESULT f_readdir(FF_DIR *dp, FILINFO *fno)
{
	if (!dp || !dp->obj.fs || !dp->host)
		return FR_INVALID_OBJECT;
	if (!fno) {
		rewinddir((DIR *)dp->host);
		dp->dptr = 0;
		return FR_OK;
	}
	for (;;) {
		errno = 0;
		struct dirent *de = readdir((DIR *)dp->host);
		if (!de) {
			memset(fno, 0, sizeof(*fno));
			return errno ? FR_DISK_ERR : FR_OK;
		}
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		if (strlen(de->d_name) > FF_LFN_BUF)
			continue;
		char host[PATH_MAX];
		struct stat st;
		if (snprintf(host, sizeof(host), "%s/%s", dp->path, de->d_name) >= (int)sizeof(host) ||
		    stat(host, &st) != 0)
			continue;
		if (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode))
			continue;
		fill_fileinfo(fno, de->d_name, &st);
		dp->dptr++;
		return FR_OK;
	}
}

FRESULT f_stat(const TCHAR *path, FILINFO *fno)
{
	char host[PATH_MAX];
	int r = resolve(path, host, sizeof(host));
	if (r < 0)
		return FR_NO_PATH;
	if (r > 0)
		return FR_NO_FILE;
	struct stat st;
	if (stat(host, &st) != 0)
		return errno_to_fresult(errno);
	if (fno) {
		const char *name = strrchr(host, '/');
		fill_fileinfo(fno, name ? name + 1 : host, &st);
	}
	return FR_OK;
}

FRESULT f_mkdir(const TCHAR *path)
{
	char host[PATH_MAX];
	int r = resolve(path, host, sizeof(host));
	if (r < 0)
		return FR_NO_PATH;
	if (r == 0)
		return FR_EXIST;
	if (mkdir(host, 0777) != 0)
		return errno_to_fresult(errno);
	return FR_OK;
}

FRESULT f_unlink(const TCHAR *path)
{
	char host[PATH_MAX];
	int r = resolve(path, host, sizeof(host));
	if (r < 0)
		return FR_NO_PATH;
	if (r > 0)
		return FR_NO_FILE;
	struct stat st;
	if (lstat(host, &st) != 0)
		return errno_to_fresult(errno);
	if (S_ISDIR(st.st_mode) ? rmdir(host) : unlink(host))
		return errno_to_fresult(errno);
	return FR_OK;
}

FRESULT f_rename(const TCHAR *path_old, const TCHAR *path_new)
{
	char from[PATH_MAX], to[PATH_MAX];
	int r = resolve(path_old, from, sizeof(from));
	if (r < 0)
		return FR_NO_PATH;
	if (r > 0)
		return FR_NO_FILE;
	r = resolve(path_new, to, sizeof(to));
	if (r < 0)
		return FR_NO_PATH;
	if (r == 0 && strcmp(from, to))
		return FR_EXIST;
	if (rename(from, to) != 0)
		return errno_to_fresult(errno);
	return FR_OK;
}
