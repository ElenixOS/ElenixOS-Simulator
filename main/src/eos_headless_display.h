/**
 * @file eos_headless_display.h
 * @brief LVGL display backend without a native window.
 */

#ifndef EOS_HEADLESS_DISPLAY_H
#define EOS_HEADLESS_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

lv_display_t *eos_headless_display_create(int32_t width,
                                          int32_t height,
                                          const char *socket_path,
                                          uint16_t websocket_port);

void eos_headless_display_set_brightness(uint8_t brightness);
void eos_headless_display_set_power(bool on);

#ifdef __cplusplus
}
#endif

#endif /* EOS_HEADLESS_DISPLAY_H */
