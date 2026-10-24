#include "input_linux.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define MAX_DEVICES   16
#define RESCAN_US     1000000u

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NLONGS(x)     (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define TEST_BIT(bit, array) \
	((array[(bit) / BITS_PER_LONG] >> ((bit) % BITS_PER_LONG)) & 1UL)

struct axis {
	int32_t lo, hi;   /* thresholds: below lo = negative, above hi = positive */
	bool valid;
};

struct input_dev {
	int fd;
	char path[32];
	bool keyboard;
	bool gamepad;
	/* game controller state, as INPUT_PAD_* bits */
	uint32_t buttons;
	uint32_t hat;
	uint32_t stick;
	struct axis ax, ay;
};

static struct input_dev devs[MAX_DEVICES];
static int ndevs;
static uint64_t last_scan_us;
static bool initialized;

static uint64_t now_us(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

static bool is_open(const char *path)
{
	for (int i = 0; i < ndevs; ++i)
		if (!strcmp(devs[i].path, path))
			return true;
	return false;
}

/* Stick deflection beyond half of the travel from the centre counts as a
 * direction, like a digital pad. */
static void read_axis(int fd, int code, struct axis *a)
{
	struct input_absinfo info;
	a->valid = false;
	if (ioctl(fd, EVIOCGABS(code), &info) < 0 || info.maximum <= info.minimum)
		return;
	const int32_t center = info.minimum + (info.maximum - info.minimum) / 2;
	const int32_t quarter = (info.maximum - info.minimum) / 4;
	a->lo = center - quarter;
	a->hi = center + quarter;
	a->valid = true;
}

static void probe(const char *path)
{
	if (ndevs >= MAX_DEVICES || is_open(path))
		return;
	int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0)
		return;

	unsigned long ev[NLONGS(EV_CNT)] = {0};
	unsigned long keys[NLONGS(KEY_CNT)] = {0};
	unsigned long rel[NLONGS(REL_CNT)] = {0};
	unsigned long abs[NLONGS(ABS_CNT)] = {0};
	if (ioctl(fd, EVIOCGBIT(0, sizeof(ev)), ev) < 0) {
		close(fd);
		return;
	}
	if (TEST_BIT(EV_KEY, ev))
		ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keys)), keys);
	if (TEST_BIT(EV_REL, ev))
		ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel)), rel);
	if (TEST_BIT(EV_ABS, ev))
		ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs)), abs);

	const bool keyboard = TEST_BIT(EV_KEY, ev) && TEST_BIT(KEY_A, keys) &&
			      TEST_BIT(KEY_Z, keys) && TEST_BIT(KEY_SPACE, keys);
	const bool mouse = TEST_BIT(EV_REL, ev) && TEST_BIT(REL_X, rel);
	const bool gamepad = !keyboard && !mouse && TEST_BIT(EV_KEY, ev) &&
			     (TEST_BIT(BTN_GAMEPAD, keys) || TEST_BIT(BTN_JOYSTICK, keys) ||
			      TEST_BIT(BTN_DPAD_UP, keys));
	if (!keyboard && !gamepad) {
		close(fd);
		return;
	}
	/* Exclusive access: the console must not see the emulator's keys. */
	if (keyboard && ioctl(fd, EVIOCGRAB, 1) < 0)
		fprintf(stderr, "input: cannot grab %s: %s\n", path, strerror(errno));

	char name[64] = "?";
	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("input: %s \"%s\"%s\n", path, name, keyboard ? " keyboard" : " gamepad");

	struct input_dev *d = &devs[ndevs++];
	memset(d, 0, sizeof(*d));
	d->fd = fd;
	snprintf(d->path, sizeof(d->path), "%s", path);
	d->keyboard = keyboard;
	d->gamepad = gamepad;
	if (gamepad && TEST_BIT(EV_ABS, ev)) {
		if (TEST_BIT(ABS_X, abs))
			read_axis(fd, ABS_X, &d->ax);
		if (TEST_BIT(ABS_Y, abs))
			read_axis(fd, ABS_Y, &d->ay);
	}
}

static void rescan(void)
{
	DIR *dir = opendir("/dev/input");
	if (!dir)
		return;
	struct dirent *de;
	while ((de = readdir(dir)) != NULL) {
		if (strncmp(de->d_name, "event", 5))
			continue;
		char path[32];
		snprintf(path, sizeof(path), "/dev/input/%.20s", de->d_name);
		probe(path);
	}
	closedir(dir);
}

static void drop(int i)
{
	printf("input: %s removed\n", devs[i].path);
	close(devs[i].fd);
	devs[i] = devs[--ndevs];
}

/* Controller button -> INPUT_PAD_* bit.  Standard (BTN_GAMEPAD) layout first,
 * then generic USB joysticks (BTN_JOYSTICK layout). */
static uint32_t button_bit(unsigned code)
{
	switch (code) {
	case BTN_SOUTH:      return INPUT_PAD_A;
	case BTN_EAST:       return INPUT_PAD_B;
	case BTN_SELECT:     return INPUT_PAD_SELECT;
	case BTN_START:      return INPUT_PAD_START;
	case BTN_DPAD_UP:    return INPUT_PAD_UP;
	case BTN_DPAD_DOWN:  return INPUT_PAD_DOWN;
	case BTN_DPAD_LEFT:  return INPUT_PAD_LEFT;
	case BTN_DPAD_RIGHT: return INPUT_PAD_RIGHT;
	case BTN_TRIGGER:    return INPUT_PAD_A;
	case BTN_THUMB:      return INPUT_PAD_B;
	case BTN_BASE3:      return INPUT_PAD_SELECT;
	case BTN_BASE4:      return INPUT_PAD_START;
	default:             return 0;
	}
}

static uint32_t axis_bits(int32_t v, const struct axis *a, uint32_t neg, uint32_t pos)
{
	if (!a->valid)
		return 0;
	return v < a->lo ? neg : v > a->hi ? pos : 0;
}

static void handle(struct input_dev *d, const struct input_event *e,
		   void (*on_key)(int code, int value))
{
	if (d->keyboard) {
		if (e->type == EV_KEY && e->code < BTN_MISC && on_key)
			on_key(e->code, e->value);
		return;
	}
	if (e->type == EV_KEY) {
		const uint32_t bit = button_bit(e->code);
		d->buttons = e->value ? (d->buttons | bit) : (d->buttons & ~bit);
	} else if (e->type == EV_ABS) {
		switch (e->code) {
		case ABS_HAT0X:
			d->hat = (d->hat & ~(INPUT_PAD_LEFT | INPUT_PAD_RIGHT)) |
				 (e->value < 0 ? INPUT_PAD_LEFT : e->value > 0 ? INPUT_PAD_RIGHT : 0);
			break;
		case ABS_HAT0Y:
			d->hat = (d->hat & ~(INPUT_PAD_UP | INPUT_PAD_DOWN)) |
				 (e->value < 0 ? INPUT_PAD_UP : e->value > 0 ? INPUT_PAD_DOWN : 0);
			break;
		case ABS_X:
			d->stick = (d->stick & ~(INPUT_PAD_LEFT | INPUT_PAD_RIGHT)) |
				   axis_bits(e->value, &d->ax, INPUT_PAD_LEFT, INPUT_PAD_RIGHT);
			break;
		case ABS_Y:
			d->stick = (d->stick & ~(INPUT_PAD_UP | INPUT_PAD_DOWN)) |
				   axis_bits(e->value, &d->ay, INPUT_PAD_UP, INPUT_PAD_DOWN);
			break;
		default:
			break;
		}
	}
}

void input_init(void)
{
	if (initialized)
		return;
	initialized = true;
	rescan();
	last_scan_us = now_us();
}

void input_poll(void (*on_key)(int code, int value))
{
	if (!initialized)
		return;
	uint64_t now = now_us();
	if (now - last_scan_us >= RESCAN_US) {
		last_scan_us = now;
		rescan();
	}
	for (int i = 0; i < ndevs; ) {
		struct input_event ev[64];
		ssize_t n = read(devs[i].fd, ev, sizeof(ev));
		if (n < 0) {
			if (errno == EAGAIN || errno == EINTR) {
				++i;
				continue;
			}
			drop(i);   /* ENODEV: unplugged */
			continue;
		}
		for (ssize_t k = 0; k < n / (ssize_t)sizeof(ev[0]); ++k)
			handle(&devs[i], &ev[k], on_key);
		if (n < (ssize_t)sizeof(ev))
			++i;
	}
}

uint32_t input_gamepad_state(int n)
{
	for (int i = 0; i < ndevs; ++i) {
		if (!devs[i].gamepad)
			continue;
		if (n-- == 0)
			return devs[i].buttons | devs[i].hat | devs[i].stick;
	}
	return 0;
}

void input_shutdown(void)
{
	for (int i = 0; i < ndevs; ++i) {
		if (devs[i].keyboard)
			ioctl(devs[i].fd, EVIOCGRAB, 0);
		close(devs[i].fd);
	}
	ndevs = 0;
	initialized = false;
}
