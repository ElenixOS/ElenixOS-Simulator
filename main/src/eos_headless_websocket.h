/**
 * @file eos_headless_websocket.h
 * @brief Direct loopback WebSocket transport for the headless Simulator.
 */

#ifndef EOS_HEADLESS_WEBSOCKET_H
#define EOS_HEADLESS_WEBSOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    EOS_HEADLESS_WEBSOCKET_INPUT_DOWN = 1,
    EOS_HEADLESS_WEBSOCKET_INPUT_MOVE = 2,
    EOS_HEADLESS_WEBSOCKET_INPUT_UP = 3,
    EOS_HEADLESS_WEBSOCKET_BUTTON_CROWN_CLICK = 4,
    EOS_HEADLESS_WEBSOCKET_BUTTON_CROWN_LONG_PRESS = 5,
    EOS_HEADLESS_WEBSOCKET_BUTTON_SIDE_CLICK = 6,
} eos_headless_websocket_input_action_t;

typedef void (*eos_headless_websocket_input_callback_t)(eos_headless_websocket_input_action_t action,
                                                        int32_t x,
                                                        int32_t y,
                                                        void *user_data);

/** Start a loopback WebSocket server. Port 0 requests an ephemeral port. */
bool eos_headless_websocket_init(uint16_t requested_port,
                                 uint32_t width,
                                 uint32_t height,
                                 eos_headless_websocket_input_callback_t input_callback,
                                 void *input_user_data);

/** Poll the non-blocking server from the Simulator/LVGL thread. */
void eos_headless_websocket_poll(void);

/** Publish the newest complete RGB565 framebuffer. */
bool eos_headless_websocket_submit_frame(const uint8_t *rgb565, size_t size);

/** Return the authenticated ws:// endpoint for the current server. */
const char *eos_headless_websocket_endpoint(void);

/** Stop the server and release its buffers. */
void eos_headless_websocket_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EOS_HEADLESS_WEBSOCKET_H */
