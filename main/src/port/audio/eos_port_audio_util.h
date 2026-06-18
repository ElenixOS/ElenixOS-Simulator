/**
 * @file eos_port_audio_util.h
 * @brief Shared audio port utilities for macOS simulator
 */
#ifndef EOS_PORT_AUDIO_UTIL_H
#define EOS_PORT_AUDIO_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

/**
 * @brief Resolve an audio file path for AudioToolbox APIs.
 * Tries realpath, strips leading '/', then prepends "fs/".
 * @param path Input path (may be relative)
 * @return strdup'd absolute path, or NULL if not found. Caller must free().
 */
char *eos_port_audio_resolve_path(const char *path);

#ifdef __APPLE__
/**
 * @brief Create a CFURLRef from a C file path.
 * @param path Absolute file path
 * @return CFURLRef (caller must CFRelease), or NULL
 */
CFURLRef eos_port_audio_create_cfurl(const char *path);
#endif

#ifdef __cplusplus
}
#endif

#endif /* EOS_PORT_AUDIO_UTIL_H */
