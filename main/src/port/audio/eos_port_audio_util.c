/**
 * @file eos_port_audio_util.c
 * @brief Shared audio port utilities for macOS simulator
 */
#include "eos_port_audio_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "eos_fs_port.h"

char *eos_port_audio_resolve_path(const char *path)
{
    if (!path) return NULL;

    char vpath[PATH_MAX];
    if (!eos_fs_realpath(path, vpath, sizeof(vpath)))
        return NULL;

    char resolved[PATH_MAX];
    if (realpath(vpath, resolved))
        return strdup(resolved);

    return NULL;
}

#ifdef __APPLE__
CFURLRef eos_port_audio_create_cfurl(const char *path)
{
    return CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        (const UInt8 *)path,
        strlen(path),
        false);
}
#endif
