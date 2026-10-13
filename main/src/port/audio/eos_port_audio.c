/**
 * @file eos_port_audio.c
 * @brief Audio port implementation for PC Simulator
 * @note Uses AudioToolbox (AudioQueue) for PCM output and recording
 *       on macOS. Registers speaker device ops (PCM sink only).
 *
 * Playback uses an ISR-safe pattern:
 *  - AudioQueue callback (system thread) enqueues free buffer pointers
 *    into eos_cqueue with critical section.
 *  - Main loop LVGL timer dequeues free buffers, fills with decoded PCM,
 *    and enqueues to AudioQueue.
 *  - All shared state protected by eos_critical_enter/leave.
 */

#include "eos_port_audio.h"
#include "eos_port_audio_decoder.h"
#include "eos_config.h"
#include "config/eos_simulator_config.h"

#ifdef __APPLE__

#include "eos_port_audio_util.h"
#include "eos_dev_speaker.h"
#include "eos_dev_microphone.h"
#include "mac_api.h"
#include "eos_cqueue.h"
#include "eos_port_critical.h"
#define EOS_LOG_TAG "PortAudio"
#include "eos_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

/* ---- Constants -------------------------------------------- */
#define RECORD_FORMAT_SAMPLE_RATE  44100.0
#define RECORD_FORMAT_CHANNELS     1
#define RECORD_FORMAT_BITS         16
#define NUM_PLAYBACK_BUFFERS       8
#define NUM_RECORD_BUFFERS         3
#define RECORD_BUFFER_SECONDS      0.5

/* ---- Helper: borrow/enqueue state ----------------------- */
static AudioQueueBufferRef _borrowed_buf;  /* last borrowed struct ptr */

/* ---- Playback state machine ------------------------------ */
typedef enum {
    PB_STATE_IDLE = 0,
    PB_STATE_READY,     /* queue + buffers allocated, not started   */
    PB_STATE_PLAYING,   /* AudioQueue running                       */
    PB_STATE_PAUSED,    /* AudioQueue paused                        */
    PB_STATE_STOPPING,  /* shutting down, ignore callbacks          */
} pb_state_t;

static struct {
    AudioQueueRef queue;
    AudioStreamBasicDescription format;
    AudioQueueBufferRef buffers[NUM_PLAYBACK_BUFFERS];
    eos_cqueue_t *free_queue;
    pb_state_t state;
} _playback = {0};

/* ---- Recording state -------------------------------------- */
static struct {
    AudioQueueRef queue;
    AudioStreamBasicDescription format;
    bool isRecording;
    uint8_t *ring_buf;
    uint32_t ring_buf_size;
    volatile uint32_t write_offset;
} _recording = {0};

/* ---- Forward declarations -------------------------------- */
static int _speaker_stop(void);
static int _mic_open(uint32_t sr, uint8_t ch, uint8_t bits);
static int _mic_close(void);
static int _mic_start(void);
static int _mic_stop(void);
static int _mic_set_buffer(uint8_t *buf, uint32_t size);
static uint32_t _mic_get_write_offset(void);
static int _mic_stop_recording(void);

/* ================================================================ */
/*  Playback Implementation (PCM AudioQueue Output)                  */
/* ================================================================ */

/**
 * @brief AudioQueue playback callback.
 * Runs on macOS audio system thread. Only enqueues the free
 * buffer pointer into the cqueue (ISR-safe, protected by critical section).
 */
static void _playback_callback(void *userData,
                                AudioQueueRef queue,
                                AudioQueueBufferRef buffer)
{
    (void)userData;
    (void)queue;

    if (_playback.state == PB_STATE_STOPPING)
        return;

    eos_critical_ctx_t ctx = eos_critical_enter();
    eos_cqueue_enqueue(_playback.free_queue, buffer);
    eos_critical_leave(ctx);
}

static int _speaker_init(void)
{
    return 0;
}

static int _speaker_open(uint32_t sample_rate, uint8_t channels, uint8_t bits_per_sample)
{
    if (_playback.queue)
    {
        _speaker_stop();
    }

    if (_playback.free_queue)
    {
        eos_cqueue_destroy(_playback.free_queue);
    }
    _playback.free_queue = eos_cqueue_create(NUM_PLAYBACK_BUFFERS + 2);
    if (!_playback.free_queue) return -1;

    memset(&_playback.format, 0, sizeof(_playback.format));
    _playback.format.mFormatID         = kAudioFormatLinearPCM;
    _playback.format.mSampleRate       = (Float64)sample_rate;
    _playback.format.mChannelsPerFrame = channels;
    _playback.format.mBitsPerChannel   = bits_per_sample;
    _playback.format.mBytesPerFrame    = (bits_per_sample / 8) * channels;
    _playback.format.mFramesPerPacket  = 1;
    _playback.format.mBytesPerPacket   = _playback.format.mBytesPerFrame;
    _playback.format.mFormatFlags      = kAudioFormatFlagIsSignedInteger |
                                         kAudioFormatFlagIsPacked;

    EOS_LOG_I("AQ fmt: %dHz %uch %ubits bpF=%u flags=0x%x",
           (int)_playback.format.mSampleRate,
           _playback.format.mChannelsPerFrame,
           _playback.format.mBitsPerChannel,
           (unsigned)_playback.format.mBytesPerFrame,
           (unsigned)_playback.format.mFormatFlags);

    OSStatus status = AudioQueueNewOutput(&_playback.format,
                                           _playback_callback,
                                           NULL,
                                           NULL,
                                           kCFRunLoopCommonModes,
                                           0,
                                           &_playback.queue);
    if (status != noErr)
    {
        printf("[PortAudio] Failed to create output queue (err=%d)\n", (int)status);
        eos_cqueue_destroy(_playback.free_queue);
        _playback.free_queue = NULL;
        return -1;
    }

    /* Allocate buffers and enqueue them all as "free" initially */
    UInt32 bufferByteSize = (UInt32)(sample_rate * 0.05 * _playback.format.mBytesPerFrame);
    if (bufferByteSize < 4096) bufferByteSize = 4096;

    for (int i = 0; i < NUM_PLAYBACK_BUFFERS; i++)
    {
        OSStatus s = AudioQueueAllocateBuffer(_playback.queue, bufferByteSize,
                                               &_playback.buffers[i]);
        if (s != noErr)
        {
            printf("[PortAudio] Failed to allocate buffer %d (err=%d)\n", i, (int)s);
            _playback.buffers[i] = NULL;
            continue;
        }
        _playback.buffers[i]->mAudioDataByteSize = 0;
        eos_cqueue_enqueue(_playback.free_queue, _playback.buffers[i]);
    }

    _playback.state = PB_STATE_READY;
    _borrowed_buf = NULL;

    EOS_LOG_I("PCM opened: %dHz %dch %dbits",
           sample_rate, channels, bits_per_sample);
    return 0;
}

/**
 * @brief Borrow a free AudioQueue buffer for PCM filling.
 * Returns mAudioData pointer for the player to write PCM into.
 * Port layer saves struct ptr internally for enqueue() matching.
 * Called from main loop. ISR-safe via critical section.
 * @return 0 on success, -1 if no free buffer available
 */
static int _speaker_borrow(void **p_buf, uint32_t *p_capacity)
{
    if (_playback.queue == NULL || _playback.free_queue == NULL) return -1;

    eos_critical_ctx_t ctx = eos_critical_enter();
    AudioQueueBufferRef buf = eos_cqueue_dequeue(_playback.free_queue);
    eos_critical_leave(ctx);

    if (buf == NULL) return -1;

    _borrowed_buf = buf;
    *p_buf = buf->mAudioData;
    *p_capacity = buf->mAudioDataBytesCapacity;
    return 0;
}

/**
 * @brief Enqueue the last borrowed buffer for playback.
 * @param data  must match mAudioData of last borrowed buffer
 * @param size  PCM bytes written; 0 = return buffer to pool
 */
static int _speaker_enqueue(void *data, uint32_t size)
{
    if (_playback.queue == NULL) return -1;

    AudioQueueBufferRef aq_buf = _borrowed_buf;
    _borrowed_buf = NULL;

    if (aq_buf == NULL || aq_buf->mAudioData != data) return -1;

    if (size == 0)
    {
        eos_critical_ctx_t ctx = eos_critical_enter();
        eos_cqueue_enqueue(_playback.free_queue, aq_buf);
        eos_critical_leave(ctx);
        return 0;
    }

    /* Guard: size must be multiple of mBytesPerFrame */
    UInt32 bpF = _playback.format.mBytesPerFrame;
    if (size % bpF != 0) {
        static int warn_cnt = 0;
        if (warn_cnt < 5) {
            EOS_LOG_W("misaligned enqueue size %u bpF=%u", size, bpF);
            warn_cnt++;
        }
        size -= size % bpF;
        if (size == 0) { return 0; }
    }

    aq_buf->mAudioDataByteSize = size;
    AudioQueueEnqueueBuffer(_playback.queue, aq_buf, 0, NULL);

    if (_playback.state == PB_STATE_READY)
    {
        AudioQueueStart(_playback.queue, NULL);
        _playback.state = PB_STATE_PLAYING;
    }
    return 0;
}

static int _speaker_stop(void)
{
    if (_playback.state == PB_STATE_IDLE) return 0;

    _playback.state = PB_STATE_STOPPING;

    _borrowed_buf = NULL;
    AudioQueueDispose(_playback.queue, true);
    _playback.queue = NULL;

    if (_playback.free_queue)
    {
        eos_cqueue_destroy(_playback.free_queue);
        _playback.free_queue = NULL;
    }

    for (int i = 0; i < NUM_PLAYBACK_BUFFERS; i++)
    {
        _playback.buffers[i] = NULL;
    }
    _playback.state = PB_STATE_IDLE;

    printf("[PortAudio] Playback stopped\n");
    return 0;
}

static int _speaker_pause(void)
{
    if (_playback.state == PB_STATE_PLAYING)
    {
        AudioQueuePause(_playback.queue);
        _playback.state = PB_STATE_PAUSED;
        printf("[PortAudio] Playback paused\n");
    }
    return 0;
}

static int _speaker_resume(void)
{
    if (_playback.state == PB_STATE_PAUSED)
    {
        AudioQueueStart(_playback.queue, NULL);
        _playback.state = PB_STATE_PLAYING;
        printf("[PortAudio] Playback resumed\n");
    }
    return 0;
}

static int _speaker_set_volume(uint8_t volume)
{
#if EOS_PORT_MACOS_VOLUME_CONTROL_ENABLE
    set_system_volume(volume * 0.01);
#else
    (void)volume;
#endif
    return 0;
}

static bool _speaker_is_available(void)
{
    return true;
}

/* ================================================================ */
/*  Recording Implementation (ring-buffer producer)                  */
/* ================================================================ */

static void _recording_callback(void *userData,
                                 AudioQueueRef queue,
                                 AudioQueueBufferRef buffer,
                                 const AudioTimeStamp *startTime,
                                 UInt32 numPackets,
                                 const AudioStreamPacketDescription *packetDescs)
{
    (void)userData;
    (void)startTime;
    (void)packetDescs;

    if (!_recording.isRecording)
        return;

    if (numPackets > 0 && _recording.ring_buf)
    {
        uint8_t *src = (uint8_t *)buffer->mAudioData;
        UInt32 size = buffer->mAudioDataByteSize;
        uint32_t pos = _recording.write_offset % _recording.ring_buf_size;
        uint32_t space = _recording.ring_buf_size - pos;

        if (size <= space) {
            memcpy(_recording.ring_buf + pos, src, size);
        } else {
            memcpy(_recording.ring_buf + pos, src, space);
            memcpy(_recording.ring_buf, src + space, size - space);
        }
        _recording.write_offset += size;
    }

    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

static int _mic_init(void)
{
    memset(&_recording.format, 0, sizeof(_recording.format));
    _recording.format.mFormatID         = kAudioFormatLinearPCM;
    _recording.format.mSampleRate       = RECORD_FORMAT_SAMPLE_RATE;
    _recording.format.mChannelsPerFrame = RECORD_FORMAT_CHANNELS;
    _recording.format.mBitsPerChannel   = RECORD_FORMAT_BITS;
    _recording.format.mBytesPerFrame    = (RECORD_FORMAT_BITS / 8) * RECORD_FORMAT_CHANNELS;
    _recording.format.mFramesPerPacket  = 1;
    _recording.format.mBytesPerPacket   = _recording.format.mBytesPerFrame;
    _recording.format.mFormatFlags      = kAudioFormatFlagIsSignedInteger |
                                           kAudioFormatFlagIsPacked;
    return 0;
}

static int _mic_stop_recording(void)
{
    if (!_recording.isRecording) return 0;
    _recording.isRecording = false;
    if (_recording.queue) {
        AudioQueueStop(_recording.queue, true);
        AudioQueueReset(_recording.queue);
        AudioQueueDispose(_recording.queue, true);
        _recording.queue = NULL;
    }
    printf("[PortAudio] Recording stopped\n");
    return 0;
}

static bool _mic_is_available(void)
{
    return true;
}

static int _mic_open(uint32_t sample_rate, uint8_t channels, uint8_t bits)
{
    _recording.format.mFormatID         = kAudioFormatLinearPCM;
    _recording.format.mSampleRate       = (Float64)sample_rate;
    _recording.format.mChannelsPerFrame = channels;
    _recording.format.mBitsPerChannel   = bits;
    _recording.format.mBytesPerFrame    = (bits / 8) * channels;
    _recording.format.mFramesPerPacket  = 1;
    _recording.format.mBytesPerPacket   = _recording.format.mBytesPerFrame;
    _recording.format.mFormatFlags      = kAudioFormatFlagIsSignedInteger |
                                           kAudioFormatFlagIsPacked;
    _recording.write_offset = 0;
    return 0;
}

static int _mic_close(void)
{
    _mic_stop_recording();
    return 0;
}

static int _mic_start(void)
{
    if (_recording.queue) {
        _mic_stop_recording();
    }

    _recording.write_offset = 0;

    OSStatus status = AudioQueueNewInput(&_recording.format,
                                          _recording_callback,
                                          NULL,
                                          NULL,      /* internal dispatch queue — avoids runloop deadlock */
                                          NULL,      /* unused when CFRunLoopRef is NULL */
                                          0,
                                          &_recording.queue);
    if (status != noErr) {
        printf("[PortAudio] Failed to create input queue (err=%d)\n", (int)status);
        return -1;
    }

    UInt32 bufferByteSize = (UInt32)(_recording.format.mSampleRate *
                                      RECORD_BUFFER_SECONDS *
                                      _recording.format.mBytesPerFrame);
    for (int i = 0; i < NUM_RECORD_BUFFERS; i++) {
        AudioQueueBufferRef buf = NULL;
        AudioQueueAllocateBuffer(_recording.queue, bufferByteSize, &buf);
        if (buf) AudioQueueEnqueueBuffer(_recording.queue, buf, 0, NULL);
    }

    _recording.isRecording = true;
    status = AudioQueueStart(_recording.queue, NULL);

    printf("[PortAudio] Recording started (rate=%.0f, ch=%d, bits=%d)\n",
           _recording.format.mSampleRate,
           (int)_recording.format.mChannelsPerFrame,
           (int)_recording.format.mBitsPerChannel);
    return (status == noErr) ? 0 : -1;
}

static int _mic_stop(void)
{
    _mic_stop_recording();
    return 0;
}

static int _mic_set_buffer(uint8_t *buf, uint32_t size)
{
    _recording.ring_buf = buf;
    _recording.ring_buf_size = size;
    _recording.write_offset = 0;
    return 0;
}

static uint32_t _mic_get_write_offset(void)
{
    return _recording.write_offset;
}

/* ---- Ops tables ------------------------------------------- */

static const eos_dev_speaker_ops_t _speaker_ops = {
    .init         = _speaker_init,
    .deinit       = NULL,
    .open         = _speaker_open,
    .borrow       = _speaker_borrow,
    .enqueue      = _speaker_enqueue,
    .stop         = _speaker_stop,
    .pause        = _speaker_pause,
    .resume       = _speaker_resume,
    .set_volume   = _speaker_set_volume,
    .is_available = _speaker_is_available,
};

static const eos_dev_microphone_ops_t _microphone_ops = {
    .init        = _mic_init,
    .deinit      = NULL,
    .open        = _mic_open,
    .close       = _mic_close,
    .start       = _mic_start,
    .stop        = _mic_stop,
    .set_gain    = NULL,
    .is_available = _mic_is_available,
    .set_buffer   = _mic_set_buffer,
    .get_write_offset = _mic_get_write_offset,
};

/* ---- Init ------------------------------------------------- */

void eos_port_audio_init(void)
{
    printf("[PortAudio] Audio port initializing...\n");
    eos_dev_speaker_register(&_speaker_ops);
    eos_dev_microphone_register(&_microphone_ops);
    _mic_init();
    eos_port_audio_decoder_init();
}

#else /* !__APPLE__ */

#include <stdio.h>

void eos_port_audio_init(void)
{
    printf("[PortAudio] Audio port stub (not macOS)\n");
}

#endif /* __APPLE__ */
