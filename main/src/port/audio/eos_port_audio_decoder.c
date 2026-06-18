/**
 * @file eos_port_audio_decoder.c
 * @brief AudioToolbox-based audio decoder for macOS simulator
 *
 * This is the MVP decoder for the simulator. It wraps AudioToolbox's
 * file-reading and format-conversion APIs to decode audio files into
 * raw LPCM for the speaker device.
 *
 * This file is part of the port layer, NOT the core OS library.
 */

#include "eos_port_audio_decoder.h"

#ifdef __APPLE__

#include "eos_port_audio_util.h"
#include "eos_audio_decoder.h"

#define EOS_LOG_TAG "AudioDecAT"
#include "eos_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#define AT_READ_BUFFER_PACKETS 512

typedef struct {
    AudioFileID audioFile;
    AudioStreamBasicDescription srcFormat;
    AudioConverterRef converter;
    AudioStreamPacketDescription *packetDescs;
    SInt64 totalPackets;
    UInt32 packetIndex;
    UInt32 maxPacketSize;
    uint8_t *readBuf;          /* compressed input from AudioFile  */
    UInt32 readBufSize;
    uint32_t bytesPerFrame;
    bool isLPCM;
} at_dec_data_t;

static bool _at_is_lpcm(const AudioStreamBasicDescription *fmt)
{
    return fmt->mFormatID == kAudioFormatLinearPCM;
}

static eos_result_t _at_probe(const void *src, eos_audio_src_type_t src_type,
                              eos_audio_format_t *format)
{
    if (src_type != EOS_AUDIO_SRC_FILE) return EOS_FAILED;

    char *resolved = eos_port_audio_resolve_path((const char *)src);
    if (!resolved) return EOS_FAILED;

    CFURLRef url = eos_port_audio_create_cfurl(resolved);
    free(resolved);
    if (!url) return EOS_FAILED;

    AudioFileID af = NULL;
    OSStatus status = AudioFileOpenURL(url, kAudioFileReadPermission, 0, &af);
    CFRelease(url);
    if (status != noErr) return EOS_FAILED;

    AudioStreamBasicDescription asbd;
    UInt32 propSize = sizeof(asbd);
    status = AudioFileGetProperty(af, kAudioFilePropertyDataFormat,
                                   &propSize, &asbd);
    if (status != noErr)
    {
        AudioFileClose(af);
        return EOS_FAILED;
    }

    format->sample_rate    = (uint32_t)asbd.mSampleRate;
    format->channels       = asbd.mChannelsPerFrame;
    format->bits_per_sample = asbd.mBitsPerChannel;
    if (format->bits_per_sample == 0) format->bits_per_sample = 16;

    SInt64 totalPackets = 0;
    propSize = sizeof(totalPackets);
    AudioFileGetProperty(af, kAudioFilePropertyAudioDataPacketCount,
                         &propSize, &totalPackets);

    if (_at_is_lpcm(&asbd) && totalPackets > 0)
    {
        format->total_samples = (uint32_t)totalPackets;
    }
    else
    {
        Float64 totalFrames = 0;
        if (asbd.mFramesPerPacket > 0 && totalPackets > 0)
        {
            totalFrames = (Float64)totalPackets * asbd.mFramesPerPacket;
        }
        else
        {
            propSize = sizeof(totalFrames);
            AudioFileGetProperty(af, kAudioFilePropertyEstimatedDuration,
                                 &propSize, &totalFrames);
            totalFrames *= asbd.mSampleRate;
        }
        format->total_samples = (uint32_t)totalFrames;
    }

    if (format->total_samples > 0 && format->sample_rate > 0)
        format->duration_ms = (uint32_t)((uint64_t)format->total_samples * 1000 / format->sample_rate);

    AudioFileClose(af);
    return EOS_OK;
}

static eos_result_t _at_open(eos_audio_decoder_dsc_t *dsc)
{
    at_dec_data_t *ad = calloc(1, sizeof(at_dec_data_t));
    if (!ad) return EOS_ERR_MEM;

    char *resolved = eos_port_audio_resolve_path((const char *)dsc->src);
    if (!resolved)
    {
        free(ad);
        return EOS_ERR_NOT_FOUND;
    }

    CFURLRef url = eos_port_audio_create_cfurl(resolved);
    free(resolved);
    if (!url)
    {
        free(ad);
        return EOS_ERR_FILE_ERROR;
    }

    OSStatus status = AudioFileOpenURL(url, kAudioFileReadPermission, 0, &ad->audioFile);
    CFRelease(url);
    if (status != noErr)
    {
        free(ad);
        return EOS_ERR_FILE_ERROR;
    }

    UInt32 propSize = sizeof(ad->srcFormat);
    AudioFileGetProperty(ad->audioFile, kAudioFilePropertyDataFormat,
                         &propSize, &ad->srcFormat);

    ad->isLPCM = _at_is_lpcm(&ad->srcFormat);

    AudioStreamBasicDescription outFmt = {0};
    outFmt.mFormatID         = kAudioFormatLinearPCM;
    outFmt.mSampleRate       = ad->srcFormat.mSampleRate;
    outFmt.mChannelsPerFrame = 1;   /* mono for wearable: single speaker */
    outFmt.mBitsPerChannel   = 16;
    outFmt.mBytesPerFrame    = 2;   /* 16-bit mono */
    outFmt.mFramesPerPacket  = 1;
    outFmt.mBytesPerPacket   = outFmt.mBytesPerFrame;
    outFmt.mFormatFlags      = kAudioFormatFlagIsSignedInteger |
                                kAudioFormatFlagIsPacked;

    ad->bytesPerFrame = outFmt.mBytesPerFrame;

    dsc->format.sample_rate    = (uint32_t)outFmt.mSampleRate;
    dsc->format.channels       = outFmt.mChannelsPerFrame;
    dsc->format.bits_per_sample = outFmt.mBitsPerChannel;

    propSize = sizeof(ad->totalPackets);
    AudioFileGetProperty(ad->audioFile, kAudioFilePropertyAudioDataPacketCount,
                         &propSize, &ad->totalPackets);

    propSize = sizeof(ad->maxPacketSize);
    AudioFileGetProperty(ad->audioFile, kAudioFilePropertyPacketSizeUpperBound,
                         &propSize, &ad->maxPacketSize);
    if (ad->maxPacketSize == 0) ad->maxPacketSize = 4096;

    bool needsConvert = !ad->isLPCM
        || ad->srcFormat.mChannelsPerFrame != outFmt.mChannelsPerFrame
        || ad->srcFormat.mSampleRate       != outFmt.mSampleRate
        || ad->srcFormat.mBitsPerChannel   != outFmt.mBitsPerChannel;

    if (needsConvert)
    {
        status = AudioConverterNew(&ad->srcFormat, &outFmt, &ad->converter);
        if (status != noErr)
        {
            AudioFileClose(ad->audioFile);
            free(ad);
            return EOS_ERR_DEV_ERROR;
        }

        UInt32 cookieSize = 0;
        UInt32 writable = 0;
        AudioFileGetPropertyInfo(ad->audioFile, kAudioFilePropertyMagicCookieData,
                                  &cookieSize, &writable);
        if (cookieSize > 0)
        {
            void *cookie = malloc(cookieSize);
            AudioFileGetProperty(ad->audioFile, kAudioFilePropertyMagicCookieData,
                                  &cookieSize, cookie);
            AudioConverterSetProperty(ad->converter,
                                       kAudioConverterDecompressionMagicCookie,
                                       cookieSize, cookie);
            free(cookie);
        }

        ad->readBufSize = ad->maxPacketSize * AT_READ_BUFFER_PACKETS;
        if (ad->readBufSize < 4096) ad->readBufSize = 4096;
        ad->readBuf = malloc(ad->readBufSize);
    }

    bool isVBR = (ad->srcFormat.mBytesPerPacket == 0 ||
                  ad->srcFormat.mFramesPerPacket == 0);
    if (isVBR || !ad->isLPCM)
    {
        ad->packetDescs = malloc(sizeof(AudioStreamPacketDescription) * AT_READ_BUFFER_PACKETS);
    }

    if (ad->isLPCM && ad->totalPackets > 0)
    {
        dsc->format.total_samples = (uint32_t)ad->totalPackets;
    }
    else
    {
        Float64 estDur = 0;
        propSize = sizeof(estDur);
        AudioFileGetProperty(ad->audioFile, kAudioFilePropertyEstimatedDuration,
                             &propSize, &estDur);
        dsc->format.total_samples = (uint32_t)(estDur * ad->srcFormat.mSampleRate);
    }
    if (dsc->format.total_samples > 0 && dsc->format.sample_rate > 0)
        dsc->format.duration_ms = (uint32_t)((uint64_t)dsc->format.total_samples * 1000 / dsc->format.sample_rate);

    dsc->user_data = ad;
    EOS_LOG_I("srcFmt: %dHz %uch %ubits ID=%u bpF=%u fpP=%d bpP=%u flags=0x%x",
           (int)ad->srcFormat.mSampleRate, ad->srcFormat.mChannelsPerFrame,
           ad->srcFormat.mBitsPerChannel, (unsigned)ad->srcFormat.mFormatID,
           (unsigned)ad->srcFormat.mBytesPerFrame, (int)ad->srcFormat.mFramesPerPacket,
           (unsigned)ad->srcFormat.mBytesPerPacket, (unsigned)ad->srcFormat.mFormatFlags);
    EOS_LOG_I("outFmt: %dHz %uch %ubits bpF=%u packed int-signed, convert=%d totalPkts=%lld maxPkt=%u",
           (int)outFmt.mSampleRate, outFmt.mChannelsPerFrame, outFmt.mBitsPerChannel,
           (unsigned)outFmt.mBytesPerFrame, needsConvert, ad->totalPackets, ad->maxPacketSize);
    return EOS_OK;
}

static OSStatus _at_converter_input_proc(AudioConverterRef inAudioConverter,
                                          UInt32 *ioNumberDataPackets,
                                          AudioBufferList *ioData,
                                          AudioStreamPacketDescription **outDataPacketDescription,
                                          void *inUserData)
{
    (void)inAudioConverter;
    at_dec_data_t *ad = (at_dec_data_t *)inUserData;

    if (ad->packetIndex >= ad->totalPackets)
    {
        *ioNumberDataPackets = 0;
        return noErr;
    }

    UInt32 numPackets = *ioNumberDataPackets;
    if (numPackets > AT_READ_BUFFER_PACKETS) numPackets = AT_READ_BUFFER_PACKETS;

    UInt32 numBytes = numPackets * ad->maxPacketSize;
    if (ad->readBufSize < numBytes)
    {
        ad->readBuf = realloc(ad->readBuf, numBytes);
        ad->readBufSize = numBytes;
    }

    OSStatus status = AudioFileReadPacketData(ad->audioFile, false,
                                               &numBytes,
                                               ad->packetDescs,
                                               ad->packetIndex,
                                               &numPackets,
                                               ad->readBuf);
    if (status != noErr || numPackets == 0)
    {
        *ioNumberDataPackets = 0;
        return (status == noErr) ? noErr : status;
    }

    ioData->mBuffers[0].mData = ad->readBuf;
    ioData->mBuffers[0].mDataByteSize = numBytes;
    ioData->mBuffers[0].mNumberChannels = ad->srcFormat.mChannelsPerFrame;

    if (outDataPacketDescription)
    {
        *outDataPacketDescription = ad->packetDescs;
    }

    ad->packetIndex += numPackets;
    *ioNumberDataPackets = numPackets;
    return noErr;
}

static eos_result_t _at_read_lpcm(eos_audio_decoder_dsc_t *dsc,
                                  void *buf, uint32_t buf_size, uint32_t *bytes_read)
{
    at_dec_data_t *ad = (at_dec_data_t *)dsc->user_data;

    if (ad->packetIndex >= ad->totalPackets)
    {
        *bytes_read = 0;
        return EOS_OK;
    }

    SInt64 remaining = ad->totalPackets - ad->packetIndex;
    UInt32 wantFrames = buf_size / ad->bytesPerFrame;
    UInt32 wantPackets = (UInt32)((remaining < (SInt64)wantFrames) ? remaining : wantFrames);

    if (wantPackets == 0)
    {
        *bytes_read = 0;
        return EOS_OK;
    }

    UInt32 numBytes = wantPackets * ad->srcFormat.mBytesPerPacket;
    OSStatus status = AudioFileReadPacketData(ad->audioFile, false,
                                               &numBytes,
                                               ad->packetDescs,
                                               ad->packetIndex,
                                               &wantPackets,
                                               buf);
    if (status != noErr || wantPackets == 0)
    {
        *bytes_read = 0;
        return EOS_OK;
    }

    ad->packetIndex += wantPackets;
    *bytes_read = numBytes;

    if (ad->bytesPerFrame > 0) {
        uint32_t misalign = *bytes_read % ad->bytesPerFrame;
        if (misalign != 0) {
            static int lpcm_warn = 0;
            if (lpcm_warn < 5) {
                EOS_LOG_W("lpcm misalign: %u bytes (%u leftover)", *bytes_read, misalign);
                lpcm_warn++;
            }
            *bytes_read -= misalign;
        }
    }

    dsc->current_sample += wantPackets;
    return EOS_OK;
}

static eos_result_t _at_read_convert(eos_audio_decoder_dsc_t *dsc,
                                     void *buf, uint32_t buf_size, uint32_t *bytes_read)
{
    at_dec_data_t *ad = (at_dec_data_t *)dsc->user_data;

    if (ad->packetIndex >= ad->totalPackets)
    {
        *bytes_read = 0;
        return EOS_OK;
    }

    UInt32 ioPackets = AT_READ_BUFFER_PACKETS;

    AudioBufferList outBufList;
    outBufList.mNumberBuffers = 1;
    outBufList.mBuffers[0].mNumberChannels = 1;
    outBufList.mBuffers[0].mDataByteSize = buf_size;
    outBufList.mBuffers[0].mData = buf;

    AudioStreamPacketDescription outPktDescs[AT_READ_BUFFER_PACKETS];

    OSStatus status = AudioConverterFillComplexBuffer(
        ad->converter,
        _at_converter_input_proc,
        ad,
        &ioPackets,
        &outBufList,
        outPktDescs);

    if (status != noErr && status != kAudioConverterErr_RequiresPacketDescriptionsError)
    {
        *bytes_read = 0;
        return EOS_OK;
    }

    *bytes_read = outBufList.mBuffers[0].mDataByteSize;

    /* Clamp to frame boundary: AudioConverter may produce trailing partial bytes */
    if (ad->bytesPerFrame > 0) {
        uint32_t misalign = *bytes_read % ad->bytesPerFrame;
        if (misalign != 0) {
            static int warn_count = 0;
            if (warn_count < 5) {
                EOS_LOG_W("convert misalign: %u bytes (%u leftover), clamping",
                          *bytes_read, misalign);
                warn_count++;
            }
            *bytes_read -= misalign;
        }
    }
    if (*bytes_read == 0) return EOS_OK;

    dsc->current_sample += *bytes_read / ad->bytesPerFrame;
    return EOS_OK;
}

static eos_result_t _at_read(eos_audio_decoder_dsc_t *dsc,
                             void *buf, uint32_t buf_size, uint32_t *bytes_read)
{
    at_dec_data_t *ad = (at_dec_data_t *)dsc->user_data;
    if (!ad) return EOS_FAILED;

    static int _at_read_call = 0;
    eos_result_t r;
    if (ad->converter)
        r = _at_read_convert(dsc, buf, buf_size, bytes_read);
    else
        r = _at_read_lpcm(dsc, buf, buf_size, bytes_read);

    if (_at_read_call < 5 || (_at_read_call < 10 && *bytes_read == 0))
        EOS_LOG_I("read#%d: pkt=%u/%lld conv=%d buf=%u rd=%u",
               _at_read_call, ad->packetIndex, ad->totalPackets,
               ad->converter ? 1 : 0, buf_size, *bytes_read);
    _at_read_call++;
    return r;
}

static void _at_close(eos_audio_decoder_dsc_t *dsc)
{
    at_dec_data_t *ad = (at_dec_data_t *)dsc->user_data;
    if (!ad) return;

    if (ad->converter)
    {
        AudioConverterDispose(ad->converter);
        ad->converter = NULL;
    }
    if (ad->audioFile)
    {
        AudioFileClose(ad->audioFile);
        ad->audioFile = NULL;
    }
    free(ad->packetDescs);
    free(ad->readBuf);
    free(ad);
    dsc->user_data = NULL;
}

void eos_port_audio_decoder_init(void)
{
    eos_audio_decoder_t *dec = eos_audio_decoder_create();
    if (dec == NULL)
    {
        EOS_LOG_E("Failed to create AT decoder");
        return;
    }
    dec->name = "AT";
    dec->probe_cb = _at_probe;
    dec->open_cb  = _at_open;
    dec->read_cb  = _at_read;
    dec->close_cb = _at_close;
    dec->seek_cb  = NULL;
    EOS_LOG_I("AT decoder registered");
}

#else /* !__APPLE__ */

void eos_port_audio_decoder_init(void)
{
}

#endif /* __APPLE__ */
