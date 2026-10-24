#ifndef ORANGE_BK_AUDIO_LINUX_H
#define ORANGE_BK_AUDIO_LINUX_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ALSA playback, the Linux counterpart of the RP2350 44.1 kHz audio timer.
 *
 * The emulation thread produces one stereo sample per 1/44100 s of emulated
 * time (audio_push()), so beeper edges keep their position in time however
 * the host schedules the thread; a playback thread writes the queued
 * samples to the PCM device.  The device is $AUDIODEV, else the ALSA card
 * "HDMI" (plughw:CARD=HDMI,DEV=0), else "default".
 */

/* Start playback at rate Hz; false (sound disabled) if no device opens. */
bool audio_init(unsigned rate);

/* Queue one stereo sample (emulation thread).  Dropped when the queue is
 * full, i.e. when emulation runs ahead of real time (turbo). */
void audio_push(int16_t left, int16_t right);

/* Stop the playback thread and close the PCM device. */
void audio_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ORANGE_BK_AUDIO_LINUX_H */
