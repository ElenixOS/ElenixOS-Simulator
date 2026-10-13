/**
 * @file eos_headless_ipc.h
 * @brief Local IPC transport for the Native headless simulator.
 */

#ifndef EOS_HEADLESS_IPC_H
#define EOS_HEADLESS_IPC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Start the simulator IPC server.
 *
 * On macOS and Linux this creates a private Unix-domain socket and a matching
 * .ready file.  The ready file is the explicit startup handshake consumed by
 * the VS Code extension.
 */
bool eos_headless_ipc_init(const char *socket_path, uint32_t width, uint32_t height);

/** Add/update the direct WebSocket endpoint in the atomic ready marker. */
bool eos_headless_ipc_set_websocket_endpoint(const char *endpoint);

/**
 * Poll the non-blocking IPC server.  This must be called from the simulator's
 * main loop so input is injected on the same thread that runs LVGL.
 */
void eos_headless_ipc_poll(void);

/**
 * Publish the newest complete RGB565 framebuffer.
 *
 * The transport owns one latest-frame buffer.  A new frame replaces any frame
 * that is still being written to the socket, so frames never accumulate.
 */
bool eos_headless_ipc_submit_frame(const uint8_t *rgb565, size_t size);

/**
 * Return true after SIGINT/SIGTERM has requested simulator shutdown.
 */
bool eos_headless_ipc_should_exit(void);

/**
 * Stop the server and remove its socket/ready marker.
 */
void eos_headless_ipc_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EOS_HEADLESS_IPC_H */
