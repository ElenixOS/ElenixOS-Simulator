/**
 * @file eos_port_audio_decoder.h
 * @brief AudioToolbox-based audio decoder - macOS simulator port
 *
 * Register this decoder from eos_port_audio_init(). It is NOT part of
 * the core OS library — it only exists in the simulator port layer.
 */
#ifndef EOS_PORT_AUDIO_DECODER_H
#define EOS_PORT_AUDIO_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

void eos_port_audio_decoder_init(void);

#ifdef __cplusplus
}
#endif

#endif
