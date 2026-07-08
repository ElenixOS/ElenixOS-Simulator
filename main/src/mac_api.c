/**
 * @file mac_api.c
 * @brief MacOS API for macOS Tahoe 26
 */

#include "mac_api.h"

// Includes
#include <stdio.h>
#include <stdlib.h>
#if defined(__APPLE__)
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#endif

// Macros and Definitions

// Variables

// Function Implementations

#define VOLUME_ERROR -1.0f

#if defined(__APPLE__)

// Get the default output audio device ID
static AudioDeviceID get_default_output_device(void)
{
    AudioDeviceID deviceID = 0;
    UInt32 size = sizeof(deviceID);
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &size, &deviceID);
    return deviceID;
}

// Get the volume for a specific channel (1 for left, 2 for right)
static Float32 get_channel_volume(AudioDeviceID deviceID, UInt32 channel)
{
    Float32 volume = 0.0;
    UInt32 size = sizeof(volume);
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyVolumeScalar,
        kAudioDevicePropertyScopeOutput,
        channel
    };
    OSStatus result = AudioObjectGetPropertyData(deviceID, &propertyAddress, 0, NULL, &size, &volume);
    if (result != noErr)
        return -1.0f;
    return volume;
}

// Set volume for specified channel
static void set_channel_volume(AudioDeviceID deviceID, UInt32 channel, Float32 volume)
{
    UInt32 size = sizeof(volume);
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyVolumeScalar,
        kAudioDevicePropertyScopeOutput,
        channel
    };
    AudioObjectSetPropertyData(deviceID, &propertyAddress, 0, NULL, size, &volume);
}

// Get the system volume (0.0 ~ 1.0)
float get_system_volume(void)
{
    AudioDeviceID deviceID = get_default_output_device();
    if (deviceID == 0) return VOLUME_ERROR;

    // Method 1: Try main channel volume
    Float32 volume = 0.0;
    UInt32 size = sizeof(volume);
    AudioObjectPropertyAddress mainAddr = {
        kAudioDevicePropertyVolumeScalar,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };
    OSStatus result = AudioObjectGetPropertyData(deviceID, &mainAddr, 0, NULL, &size, &volume);
    if (result == noErr) return volume;

    // Method 2: Average left and right channels
    Float32 left = get_channel_volume(deviceID, 1);
    Float32 right = get_channel_volume(deviceID, 2);
    if (left < 0 && right < 0) {
        fprintf(stderr, "Error: No volume property available on this device.\n");
        return VOLUME_ERROR;
    }
    if (left < 0) return right;
    if (right < 0) return left;
    return (left + right) / 2.0f;
}

// Set the system volume (0.0 ~ 1.0)
void set_system_volume(float volume)
{
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    AudioDeviceID deviceID = get_default_output_device();
    if (deviceID == 0) {
        fprintf(stderr, "Error: No default output device.\n");
        return;
    }

    // Method 1: Try main channel volume
    AudioObjectPropertyAddress mainAddr = {
        kAudioDevicePropertyVolumeScalar,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };
    OSStatus result = AudioObjectSetPropertyData(deviceID, &mainAddr, 0, NULL, sizeof(volume), &volume);
    if (result == noErr) return; // Success

    // Method 2: Set left and right channels
    set_channel_volume(deviceID, 1, volume);
    set_channel_volume(deviceID, 2, volume);
}

float get_system_battery_level(void)
{
    CFTypeRef blob = IOPSCopyPowerSourcesInfo();
    if (!blob) return -1.0f;

    CFArrayRef sources = IOPSCopyPowerSourcesList(blob);
    if (!sources) {
        CFRelease(blob);
        return -1.0f;
    }

    float level = -1.0f;

    CFIndex count = CFArrayGetCount(sources);
    for (CFIndex i = 0; i < count; i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(sources, i);
        CFDictionaryRef dict = IOPSGetPowerSourceDescription(blob, ps);
        if (!dict) continue;

        CFNumberRef curCapacity = CFDictionaryGetValue(dict, CFSTR(kIOPSCurrentCapacityKey));
        CFNumberRef maxCapacity = CFDictionaryGetValue(dict, CFSTR(kIOPSMaxCapacityKey));
        if (curCapacity && maxCapacity) {
            int cur = 0, max = 0;
            CFNumberGetValue(curCapacity, kCFNumberIntType, &cur);
            CFNumberGetValue(maxCapacity, kCFNumberIntType, &max);
            if (max > 0) {
                level = (float)cur / (float)max;
                break; // Default to the first battery
            }
        }
    }

    CFRelease(sources);
    CFRelease(blob);
    return level;
}

int get_system_charging(void)
{
    CFTypeRef blob = IOPSCopyPowerSourcesInfo();
    if (!blob) return -1;

    CFArrayRef sources = IOPSCopyPowerSourcesList(blob);
    if (!sources) {
        CFRelease(blob);
        return -1;
    }

    int ac_connected = -1;
    CFIndex count = CFArrayGetCount(sources);
    for (CFIndex i = 0; i < count; i++) {
        CFTypeRef ps = CFArrayGetValueAtIndex(sources, i);
        CFDictionaryRef dict = IOPSGetPowerSourceDescription(blob, ps);
        if (!dict) continue;

        CFStringRef state = CFDictionaryGetValue(dict, CFSTR(kIOPSPowerSourceStateKey));
        if (state) {
            if (CFStringCompare(state, CFSTR(kIOPSACPowerValue), 0) == kCFCompareEqualTo) {
                ac_connected = 1;
            } else if (CFStringCompare(state, CFSTR(kIOPSBatteryPowerValue), 0) == kCFCompareEqualTo) {
                ac_connected = 0;
            } else {
                ac_connected = -1; // Unknown state
            }
            break; // Default to the first battery
        }
    }

    CFRelease(sources);
    CFRelease(blob);
    return ac_connected;
}

#else

float get_system_volume(void)
{
    return VOLUME_ERROR;
}

void set_system_volume(float volume)
{
    (void)volume;
}

float get_system_battery_level(void)
{
    return -1.0f;
}

int get_system_charging(void)
{
    return -1;
}

#endif
