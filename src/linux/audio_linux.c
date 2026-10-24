/*
 * ALSA audio output (see audio_linux.h), after orange386's audio_linux.c.
 */
#include "audio_linux.h"

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUDIO_PERIOD      256        /* frames per write, ~5.8 ms at 44.1 kHz */
#define AUDIO_LATENCY_US  40000u
#define QUEUE_FRAMES      2048u      /* power of two, ~46 ms */

static snd_pcm_t *pcm;
static pthread_t audio_thread;
static atomic_bool audio_running;
static bool audio_started;

/* Single-producer (emulation thread) / single-consumer (playback thread)
 * queue of interleaved L/R frames. */
static uint32_t queue[QUEUE_FRAMES];
static atomic_uint queue_w;
static atomic_uint queue_r;

void audio_push(int16_t left, int16_t right)
{
	if (!audio_started)
		return;
	const unsigned w = atomic_load_explicit(&queue_w, memory_order_relaxed);
	const unsigned r = atomic_load_explicit(&queue_r, memory_order_acquire);
	if (w - r >= QUEUE_FRAMES)
		return;
	queue[w & (QUEUE_FRAMES - 1)] = (uint32_t)(uint16_t)left | ((uint32_t)(uint16_t)right << 16);
	atomic_store_explicit(&queue_w, w + 1, memory_order_release);
}

static void *audio_main(void *arg)
{
	(void)arg;
	int16_t buf[AUDIO_PERIOD * 2];
	uint32_t last = 0;
	while (atomic_load(&audio_running)) {
		unsigned r = atomic_load_explicit(&queue_r, memory_order_relaxed);
		const unsigned w = atomic_load_explicit(&queue_w, memory_order_acquire);
		for (int i = 0; i < AUDIO_PERIOD; ++i) {
			/* An empty queue (manager open, emulation late) repeats the
			 * last level instead of dropping to 0: no click. */
			if (r != w)
				last = queue[r++ & (QUEUE_FRAMES - 1)];
			buf[i * 2 + 0] = (int16_t)(last & 0xFFFF);
			buf[i * 2 + 1] = (int16_t)(last >> 16);
		}
		atomic_store_explicit(&queue_r, r, memory_order_release);
		snd_pcm_sframes_t n = snd_pcm_writei(pcm, buf, AUDIO_PERIOD);
		if (n < 0) {
			n = snd_pcm_recover(pcm, (int)n, 1);
			if (n < 0) {
				fprintf(stderr, "audio: %s\n", snd_strerror((int)n));
				break;
			}
		}
	}
	return NULL;
}

/*
 * Sound output device, first match:
 *   AUDIODEV in the environment (as SDL programs use it),
 *   the HDMI card, which carries the picture as well,
 *   ALSA "default".
 */
static const char *audio_device_name(void)
{
	const char *dev = getenv("AUDIODEV");
	if (dev && *dev)
		return dev;
	if (snd_card_get_index("HDMI") >= 0)
		return "plughw:CARD=HDMI,DEV=0";
	return "default";
}

bool audio_init(unsigned rate)
{
	if (audio_started)
		return true;
	const char *dev = audio_device_name();
	int err = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		fprintf(stderr, "audio: cannot open %s: %s (sound disabled)\n",
			dev, snd_strerror(err));
		pcm = NULL;
		return false;
	}
	err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED,
				 2, rate, 1, AUDIO_LATENCY_US);
	if (err < 0) {
		fprintf(stderr, "audio: %s: %s (sound disabled)\n", dev, snd_strerror(err));
		snd_pcm_close(pcm);
		pcm = NULL;
		return false;
	}
	atomic_store(&audio_running, true);
	audio_started = true;
	err = pthread_create(&audio_thread, NULL, audio_main, NULL);
	if (err) {
		fprintf(stderr, "audio: cannot start thread: %s\n", strerror(err));
		audio_started = false;
		atomic_store(&audio_running, false);
		snd_pcm_close(pcm);
		pcm = NULL;
		return false;
	}
	printf("audio: %s, %u Hz stereo\n", dev, rate);
	return true;
}

void audio_shutdown(void)
{
	if (!audio_started)
		return;
	audio_started = false;
	atomic_store(&audio_running, false);
	pthread_join(audio_thread, NULL);
	snd_pcm_drop(pcm);
	snd_pcm_close(pcm);
	pcm = NULL;
}
