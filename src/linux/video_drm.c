#include "video_drm.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct buffer {
    uint32_t handle, pitch, size, fb;
    void *pixels;
};

/*
 * Physical HDMI mode for an unscaled source raster.
 * The raster is always enlarged by an integer factor and centered;
 * unused pixels are black.  Never crop or downscale.
 */
struct video_placement {
    drmModeModeInfo mode;
    uint32_t scale, x, y, width, height;
};

static int drm_fd = -1;
static drmModeConnector *conn;
static drmModeCrtc *saved_crtc;
static uint32_t crtc_id;
static bool crtc_changed;
static struct buffer buffers[2];
static int front;
static bool flip_supported = true;
static volatile bool flip_pending;
static struct video_placement placement;
static uint32_t *scale_line;

static void destroy_buffer(int fd, struct buffer *b)
{
    if (b->pixels && b->pixels != MAP_FAILED) munmap(b->pixels, b->size);
    if (b->fb) drmModeRmFB(fd, b->fb);
    if (b->handle) {
        struct drm_mode_destroy_dumb d = {.handle = b->handle};
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &d);
    }
    memset(b, 0, sizeof(*b));
}

static int create_buffer(int fd, uint32_t width, uint32_t height, struct buffer *b)
{
    struct drm_mode_create_dumb c = {.width = width, .height = height, .bpp = 32};
    if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &c) < 0) return -1;
    b->handle = c.handle;
    b->pitch = c.pitch;
    b->size = (uint32_t)c.size;
    if (drmModeAddFB(fd, width, height, 24, 32, b->pitch, b->handle, &b->fb)) return -1;
    struct drm_mode_map_dumb m = {.handle = b->handle};
    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &m) < 0) return -1;
    b->pixels = mmap(NULL, b->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, m.offset);
    if (b->pixels == MAP_FAILED) { b->pixels = NULL; return -1; }
    memset(b->pixels, 0, b->size);
    return 0;
}

static uint32_t find_crtc(int fd, drmModeRes *res, drmModeConnector *c)
{
    if (c->encoder_id) {
        drmModeEncoder *e = drmModeGetEncoder(fd, c->encoder_id);
        if (e) {
            uint32_t id = e->crtc_id;
            drmModeFreeEncoder(e);
            if (id) return id;
        }
    }
    for (int i = 0; i < c->count_encoders; ++i) {
        drmModeEncoder *e = drmModeGetEncoder(fd, c->encoders[i]);
        if (!e) continue;
        for (int j = 0; j < res->count_crtcs; ++j) {
            if (e->possible_crtcs & (1u << j)) {
                uint32_t id = res->crtcs[j];
                drmModeFreeEncoder(e);
                return id;
            }
        }
        drmModeFreeEncoder(e);
    }
    return 0;
}

static bool select_video_mode(const drmModeConnector *c,
                              uint32_t src_w, uint32_t src_h,
                              struct video_placement *out)
{
    if (!src_w || !src_h) return false;
    bool found = false;
    uint64_t best_waste = UINT64_MAX;
    uint32_t best_scale = 0, best_refresh_error = UINT32_MAX;
    for (int i = 0; i < c->count_modes; ++i) {
        const drmModeModeInfo *m = &c->modes[i];
        if (m->flags & DRM_MODE_FLAG_INTERLACE) continue;
        uint32_t scale_w = m->hdisplay / src_w;
        uint32_t scale_h = m->vdisplay / src_h;
        uint32_t scale = scale_w < scale_h ? scale_w : scale_h;
        if (!scale) continue;

        uint32_t w = src_w * scale, h = src_h * scale;
        uint64_t area = (uint64_t)m->hdisplay * m->vdisplay;
        uint64_t waste = area - (uint64_t)w * h;
        /*
         * Compare unused-area fractions, not absolute pixel counts.
         * An exact match wins; among equally filled modes, prefer the
         * larger integer scale.  Prefer refresh close to 60 Hz last.
         */
        uint32_t refresh = m->vrefresh;
        uint32_t refresh_error = refresh > 60 ? refresh - 60 : 60 - refresh;
        bool better = !found ||
            waste * (uint64_t)out->mode.hdisplay * out->mode.vdisplay <
                best_waste * area ||
            (waste * (uint64_t)out->mode.hdisplay * out->mode.vdisplay ==
                best_waste * area &&
             (scale > best_scale ||
              (scale == best_scale && refresh_error < best_refresh_error)));
        if (!better) continue;
        out->mode = *m;
        out->scale = scale;
        out->width = w;
        out->height = h;
        out->x = (m->hdisplay - w) / 2;
        out->y = (m->vdisplay - h) / 2;
        best_waste = waste;
        best_scale = scale;
        best_refresh_error = refresh_error;
        found = true;
    }
    return found;
}

bool video_drm_open(const char *device, uint32_t src_w, uint32_t src_h)
{
    drm_fd = open(device, O_RDWR | O_CLOEXEC);
    if (drm_fd < 0) { perror(device); return false; }

    drmModeRes *res = drmModeGetResources(drm_fd);
    if (!res) {
        fprintf(stderr, "drmModeGetResources: %s\n", strerror(errno));
        goto fail;
    }
    for (int i = 0; i < res->count_connectors; ++i) {
        drmModeConnector *candidate = drmModeGetConnector(drm_fd, res->connectors[i]);
        if (!candidate) continue;
        if (candidate->connection == DRM_MODE_CONNECTED && candidate->count_modes > 0 &&
            candidate->connector_type == DRM_MODE_CONNECTOR_HDMIA) {
            conn = candidate;
            break;
        }
        drmModeFreeConnector(candidate);
    }
    if (!conn) {
        fprintf(stderr, "No connected HDMI-A connector with modes found\n");
        drmModeFreeResources(res);
        goto fail;
    }

    printf("Available HDMI modes (%d):\n", conn->count_modes);
    for (int i = 0; i < conn->count_modes; ++i) {
        const drmModeModeInfo *m = &conn->modes[i];
        printf("  %4d x %-4d @ %6.2f Hz  %-16s%s\n",
            m->hdisplay, m->vdisplay,
            (double)m->clock * 1000.0 / (m->htotal * m->vtotal),
            m->name,
            (m->type & DRM_MODE_TYPE_PREFERRED) ? " [preferred]" : "");
    }

    crtc_id = find_crtc(drm_fd, res, conn);
    drmModeFreeResources(res);
    if (!crtc_id) { fprintf(stderr, "No usable CRTC found\n"); goto fail; }
    saved_crtc = drmModeGetCrtc(drm_fd, crtc_id);
    if (!saved_crtc) { fprintf(stderr, "Cannot read original CRTC\n"); goto fail; }

    if (!select_video_mode(conn, src_w, src_h, &placement)) {
        fprintf(stderr, "No HDMI mode fits the %ux%u raster\n", src_w, src_h);
        goto fail;
    }
    drmModeModeInfo *mode = &placement.mode;
    printf("Raster placement: %ux%u -> %ux%u, scale %ux, offset (%u,%u)\n",
           src_w, src_h, placement.width, placement.height, placement.scale,
           placement.x, placement.y);

    for (int i = 0; i < 2; ++i) {
        if (create_buffer(drm_fd, mode->hdisplay, mode->vdisplay, &buffers[i])) {
            fprintf(stderr, "Cannot create dumb framebuffer: %s\n", strerror(errno));
            goto fail;
        }
    }
    scale_line = calloc(placement.width, sizeof(uint32_t));
    if (!scale_line) goto fail;

    front = 0;
    if (drmModeSetCrtc(drm_fd, crtc_id, buffers[front].fb, 0, 0,
                       &conn->connector_id, 1, mode)) {
        fprintf(stderr, "drmModeSetCrtc: %s\n", strerror(errno));
        goto fail;
    }
    crtc_changed = true;
    printf("DRM: %s, HDMI connector %u, %ux%u @ %u Hz\n", device,
           conn->connector_id, mode->hdisplay, mode->vdisplay, mode->vrefresh);
    return true;

fail:
    video_drm_close();
    return false;
}

static void page_flip_handler(int fd, unsigned int sequence, unsigned int tv_sec,
                              unsigned int tv_usec, void *user_data)
{
    (void)fd; (void)sequence; (void)tv_sec; (void)tv_usec; (void)user_data;
    flip_pending = false;
}

/* Wait (up to ~100 ms) for the previously queued flip to complete. */
static void wait_flip(void)
{
    drmEventContext ev = {
        .version = 2,
        .page_flip_handler = page_flip_handler,
    };
    int budget = 100;
    while (flip_pending && budget > 0) {
        struct pollfd pfd = { .fd = drm_fd, .events = POLLIN };
        int r = poll(&pfd, 1, 10);
        if (r > 0)
            drmHandleEvent(drm_fd, &ev);
        else if (r < 0 && errno != EINTR)
            break;
        budget -= 10;
    }
    flip_pending = false;
}

static void blit_scaled(const struct buffer *dst, const uint32_t *src,
                        uint32_t src_w, uint32_t src_h)
{
    const uint32_t scale = placement.scale;
    const size_t row_bytes = (size_t)src_w * scale * sizeof(uint32_t);
    uint8_t *base = (uint8_t *)dst->pixels + (size_t)placement.y * dst->pitch +
                    (size_t)placement.x * sizeof(uint32_t);
    for (uint32_t y = 0; y < src_h; ++y) {
        const uint32_t *line = src + (size_t)y * src_w;
        if (scale > 1) {
            uint32_t *o = scale_line;
            for (uint32_t x = 0; x < src_w; ++x)
                for (uint32_t k = 0; k < scale; ++k)
                    *o++ = line[x];
            line = scale_line;
        }
        for (uint32_t k = 0; k < scale; ++k) {
            memcpy(base, line, row_bytes);
            base += dst->pitch;
        }
    }
}

void video_drm_present(const uint32_t *src, uint32_t src_w, uint32_t src_h)
{
    if (drm_fd < 0 || !crtc_changed)
        return;
    if (src_w * placement.scale > placement.width ||
        src_h * placement.scale > placement.height)
        return;

    if (!flip_supported) {
        /* Single-buffered fallback: draw straight into the scanout buffer. */
        blit_scaled(&buffers[front], src, src_w, src_h);
        return;
    }

    wait_flip();
    int back = front ^ 1;
    blit_scaled(&buffers[back], src, src_w, src_h);
    if (drmModePageFlip(drm_fd, crtc_id, buffers[back].fb,
                        DRM_MODE_PAGE_FLIP_EVENT, NULL) == 0) {
        flip_pending = true;
        front = back;
    } else {
        fprintf(stderr, "drmModePageFlip: %s; using single buffering\n",
                strerror(errno));
        flip_supported = false;
        blit_scaled(&buffers[front], src, src_w, src_h);
    }
}

void video_drm_close(void)
{
    if (drm_fd >= 0 && flip_pending)
        wait_flip();
    if (crtc_changed && saved_crtc) {
        if (saved_crtc->mode_valid) {
            if (drmModeSetCrtc(drm_fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                               saved_crtc->x, saved_crtc->y,
                               &conn->connector_id, 1, &saved_crtc->mode))
                fprintf(stderr, "Warning: failed to restore CRTC: %s\n", strerror(errno));
        } else {
            if (drmModeSetCrtc(drm_fd, saved_crtc->crtc_id, 0, 0, 0, NULL, 0, NULL))
                fprintf(stderr, "Warning: failed to disable CRTC: %s\n", strerror(errno));
        }
    }
    crtc_changed = false;
    if (drm_fd >= 0) {
        destroy_buffer(drm_fd, &buffers[0]);
        destroy_buffer(drm_fd, &buffers[1]);
    }
    free(scale_line);
    scale_line = NULL;
    if (saved_crtc) drmModeFreeCrtc(saved_crtc);
    saved_crtc = NULL;
    if (conn) drmModeFreeConnector(conn);
    conn = NULL;
    if (drm_fd >= 0) close(drm_fd);
    drm_fd = -1;
}
