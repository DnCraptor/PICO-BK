#ifndef ORANGE_BK_VIDEO_DRM_H
#define ORANGE_BK_VIDEO_DRM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Legacy DRM/KMS output: first connected HDMI-A connector, an HDMI mode that
 * holds the source raster at the largest integer scale (centred, never
 * cropped or downscaled), two XRGB8888 dumb buffers and page flipping.
 */

bool video_drm_open(const char *device, uint32_t src_w, uint32_t src_h);

/* Scale src (src_w x src_h XRGB8888, tightly packed) into the back buffer
 * and queue it for the next vblank.  Called from the scanout thread only. */
void video_drm_present(const uint32_t *src, uint32_t src_w, uint32_t src_h);

/* Restore the original CRTC configuration and release DRM resources. */
void video_drm_close(void);

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_BK_VIDEO_DRM_H */
