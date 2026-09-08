/**
 * @file eos_headless_ipc.c
 * @brief Local IPC transport for the Native headless simulator.
 */

#include "eos_headless_ipc.h"

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "eos_config.h"
#include "eos_touch.h"

#define EOS_HEADLESS_IPC_MAGIC "EOS1"
#define EOS_HEADLESS_IPC_VERSION 1U
#define EOS_HEADLESS_IPC_HEADER_SIZE 16U
#define EOS_HEADLESS_IPC_FRAME_META_SIZE 16U
#define EOS_HEADLESS_IPC_MAX_CONTROL_PAYLOAD 4096U
#define EOS_HEADLESS_IPC_RX_SIZE 8192U
#define EOS_HEADLESS_IPC_DEFAULT_SOCKET "/tmp/elenixos-simulator.sock"

enum
{
    EOS_HEADLESS_IPC_MESSAGE_HELLO = 1,
    EOS_HEADLESS_IPC_MESSAGE_FRAME = 2,
    EOS_HEADLESS_IPC_MESSAGE_INPUT = 3,
    EOS_HEADLESS_IPC_MESSAGE_HELLO_ACK = 4
};

enum
{
    EOS_HEADLESS_IPC_INPUT_DOWN = 1,
    EOS_HEADLESS_IPC_INPUT_MOVE = 2,
    EOS_HEADLESS_IPC_INPUT_UP = 3
};

static int s_listener_fd = -1;
static int s_client_fd = -1;
static bool s_client_ready;
static bool s_initialized;
static bool s_stop_requested;
static uint32_t s_width;
static uint32_t s_height;
static size_t s_frame_size;
static uint8_t *s_latest_frame;
static uint8_t *s_frame_wire;
static size_t s_frame_wire_size;
static size_t s_frame_wire_offset;
static bool s_frame_dirty;
static uint8_t s_control_wire[EOS_HEADLESS_IPC_HEADER_SIZE + EOS_HEADLESS_IPC_MAX_CONTROL_PAYLOAD];
static size_t s_control_wire_size;
static size_t s_control_wire_offset;
static uint8_t s_rx_buffer[EOS_HEADLESS_IPC_RX_SIZE];
static size_t s_rx_size;
static char s_socket_path[sizeof(((struct sockaddr_un *)0)->sun_path)];
static char s_ready_path[sizeof(s_socket_path) + 6U];
static char s_websocket_endpoint[256];

static void _signal_handler(int signal_number)
{
    (void)signal_number;
    s_stop_requested = true;
}

static bool _set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return false;
    }

    (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
    return true;
}

static void _write_u16_be(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value >> 8U);
    dst[1] = (uint8_t)value;
}

static void _write_u32_be(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value >> 24U);
    dst[1] = (uint8_t)(value >> 16U);
    dst[2] = (uint8_t)(value >> 8U);
    dst[3] = (uint8_t)value;
}

static uint16_t _read_u16_be(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[0] << 8U) | src[1]);
}

static uint32_t _read_u32_be(const uint8_t *src)
{
    return ((uint32_t)src[0] << 24U) | ((uint32_t)src[1] << 16U) | ((uint32_t)src[2] << 8U) | src[3];
}

static void _write_header(uint8_t *dst, uint16_t type, uint32_t payload_size, uint32_t sequence)
{
    memcpy(dst, EOS_HEADLESS_IPC_MAGIC, 4U);
    _write_u16_be(dst + 4U, EOS_HEADLESS_IPC_VERSION);
    _write_u16_be(dst + 6U, type);
    _write_u32_be(dst + 8U, payload_size);
    _write_u32_be(dst + 12U, sequence);
}

static void _close_client(void)
{
    if (s_client_fd >= 0)
    {
        close(s_client_fd);
        s_client_fd = -1;
    }

    s_client_ready = false;
    s_rx_size = 0U;
    s_control_wire_size = 0U;
    s_control_wire_offset = 0U;
    /* Drop only the unsent wire bytes.  Keep the latest framebuffer so a
     * replacement client can receive a complete current frame. */
    s_frame_wire_size = 0U;
    s_frame_wire_offset = 0U;
    s_frame_dirty = true;
}

static void _cleanup_files(void)
{
    if (s_socket_path[0] != '\0')
    {
        (void)unlink(s_socket_path);
    }
    if (s_ready_path[0] != '\0')
    {
        (void)unlink(s_ready_path);
    }
}

static void _cleanup_at_exit(void)
{
    eos_headless_ipc_shutdown();
}

static bool _write_ready_file(void)
{
    char temp_path[sizeof(s_ready_path) + 32U];
    FILE *file;
    int length;

    length = snprintf(temp_path, sizeof(temp_path), "%s.tmp.%ld", s_ready_path, (long)getpid());
    if (length <= 0 || (size_t)length >= sizeof(temp_path))
    {
        return false;
    }

    file = fopen(temp_path, "w");
    if (!file)
    {
        return false;
    }

    if (fprintf(file,
                "{\"protocol\":\"elenixos-simulator\",\"version\":1,\"socket\":\"%s\","
                "\"pid\":%ld,\"width\":%u,\"height\":%u,\"format\":\"rgb565-le\","
                "\"websocket\":\"%s\"}\n",
                s_socket_path,
                (long)getpid(),
                s_width,
                s_height,
                s_websocket_endpoint)
            < 0
        || fflush(file) != 0 || fclose(file) != 0)
    {
        (void)unlink(temp_path);
        return false;
    }

    if (rename(temp_path, s_ready_path) != 0)
    {
        (void)unlink(temp_path);
        return false;
    }

    return true;
}

static bool _send_pending_buffer(const uint8_t *buffer, size_t size, size_t *offset)
{
    while (*offset < size)
    {
        ssize_t written = send(s_client_fd, buffer + *offset, size - *offset, 0);
        if (written > 0)
        {
            *offset += (size_t)written;
            continue;
        }
        if (written < 0 && (errno == EINTR))
        {
            continue;
        }
        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            return true;
        }
        return false;
    }

    return true;
}

static void _queue_hello_ack(void)
{
    static const uint8_t payload[] = {'o', 'k'};

    _write_header(s_control_wire, EOS_HEADLESS_IPC_MESSAGE_HELLO_ACK, (uint32_t)sizeof(payload), 0U);
    memcpy(s_control_wire + EOS_HEADLESS_IPC_HEADER_SIZE, payload, sizeof(payload));
    s_control_wire_size = EOS_HEADLESS_IPC_HEADER_SIZE + sizeof(payload);
    s_control_wire_offset = 0U;
}

static void _handle_input(const uint8_t *payload, size_t payload_size)
{
    int32_t x;
    int32_t y;
    eos_touch_inject_result_t result;

    if (payload_size != 9U)
    {
        return;
    }

    x = (int32_t)_read_u32_be(payload + 1U);
    y = (int32_t)_read_u32_be(payload + 5U);
    switch (payload[0])
    {
        case EOS_HEADLESS_IPC_INPUT_DOWN:
            result = eos_touch_inject_down(x, y);
            break;
        case EOS_HEADLESS_IPC_INPUT_MOVE:
            result = eos_touch_inject_move(x, y);
            break;
        case EOS_HEADLESS_IPC_INPUT_UP:
            result = eos_touch_inject_up(x, y);
            break;
        default:
            return;
    }

    (void)result;
}

static bool _handle_message(uint16_t type, const uint8_t *payload, size_t payload_size)
{
    switch (type)
    {
        case EOS_HEADLESS_IPC_MESSAGE_HELLO:
            s_client_ready = true;
            /* A client may connect after several direct-WebSocket frames
             * have already been rendered. Reuse the latest retained frame
             * for the legacy compatibility stream, but do not copy frames
             * into that stream while no legacy client is subscribed. */
            s_frame_dirty = true;
            _queue_hello_ack();
            return true;
        case EOS_HEADLESS_IPC_MESSAGE_INPUT:
            if (s_client_ready)
            {
                _handle_input(payload, payload_size);
            }
            return true;
        default:
            return false;
    }
}

static bool _parse_client_messages(void)
{
    while (s_rx_size >= EOS_HEADLESS_IPC_HEADER_SIZE)
    {
        uint32_t payload_size;
        size_t message_size;

        if (memcmp(s_rx_buffer, EOS_HEADLESS_IPC_MAGIC, 4U) != 0
            || _read_u16_be(s_rx_buffer + 4U) != EOS_HEADLESS_IPC_VERSION)
        {
            return false;
        }

        payload_size = _read_u32_be(s_rx_buffer + 8U);
        if (payload_size > EOS_HEADLESS_IPC_MAX_CONTROL_PAYLOAD)
        {
            return false;
        }

        message_size = EOS_HEADLESS_IPC_HEADER_SIZE + (size_t)payload_size;
        if (s_rx_size < message_size)
        {
            return true;
        }

        if (!_handle_message(_read_u16_be(s_rx_buffer + 6U), s_rx_buffer + EOS_HEADLESS_IPC_HEADER_SIZE, payload_size))
        {
            return false;
        }

        s_rx_size -= message_size;
        if (s_rx_size > 0U)
        {
            memmove(s_rx_buffer, s_rx_buffer + message_size, s_rx_size);
        }
    }

    return true;
}

static bool _read_client(void)
{
    for (;;)
    {
        ssize_t received;

        if (s_rx_size >= sizeof(s_rx_buffer))
        {
            return false;
        }

        received = recv(s_client_fd, s_rx_buffer + s_rx_size, sizeof(s_rx_buffer) - s_rx_size, 0);
        if (received > 0)
        {
            s_rx_size += (size_t)received;
            if (!_parse_client_messages())
            {
                return false;
            }
            continue;
        }
        if (received == 0)
        {
            return false;
        }
        if (errno == EINTR)
        {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return true;
        }
        return false;
    }
}

static void _accept_clients(void)
{
    for (;;)
    {
        int client_fd = accept(s_listener_fd, NULL, NULL);
        if (client_fd < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return;
        }

        if (!_set_nonblocking(client_fd))
        {
            close(client_fd);
            continue;
        }

        _close_client();
        s_client_fd = client_fd;
    }
}

static void _queue_latest_frame(void)
{
    uint8_t *payload;

    if (!s_frame_dirty || s_frame_wire_size != 0U || !s_latest_frame)
    {
        return;
    }

    _write_header(s_frame_wire,
                  EOS_HEADLESS_IPC_MESSAGE_FRAME,
                  (uint32_t)(EOS_HEADLESS_IPC_FRAME_META_SIZE + s_frame_size),
                  0U);
    payload = s_frame_wire + EOS_HEADLESS_IPC_HEADER_SIZE;
    _write_u32_be(payload, s_width);
    _write_u32_be(payload + 4U, s_height);
    _write_u32_be(payload + 8U, s_width * 2U);
    _write_u32_be(payload + 12U, 1U); /* RGB565 little-endian */
    memcpy(payload + EOS_HEADLESS_IPC_FRAME_META_SIZE, s_latest_frame, s_frame_size);
    s_frame_wire_size = EOS_HEADLESS_IPC_HEADER_SIZE + EOS_HEADLESS_IPC_FRAME_META_SIZE + s_frame_size;
    s_frame_wire_offset = 0U;
    s_frame_dirty = false;
}

static void _flush_client(void)
{
    if (s_client_fd < 0 || !s_client_ready)
    {
        return;
    }

    if (s_control_wire_offset < s_control_wire_size
        && !_send_pending_buffer(s_control_wire, s_control_wire_size, &s_control_wire_offset))
    {
        _close_client();
        return;
    }

    if (s_control_wire_offset < s_control_wire_size)
    {
        return;
    }

    _queue_latest_frame();
    if (s_frame_wire_offset < s_frame_wire_size
        && !_send_pending_buffer(s_frame_wire, s_frame_wire_size, &s_frame_wire_offset))
    {
        _close_client();
        return;
    }

    if (s_frame_wire_size != 0U && s_frame_wire_offset >= s_frame_wire_size)
    {
        /* The complete wire frame is now safely out of our buffer.  Only
         * then may the latest replacement frame be copied into it. */
        s_frame_wire_size = 0U;
        s_frame_wire_offset = 0U;
        _queue_latest_frame();
    }
}

bool eos_headless_ipc_init(const char *socket_path, uint32_t width, uint32_t height)
{
    const char *requested_path = socket_path;
    struct sockaddr_un address;
    size_t socket_path_length;
    int length;

    if (s_initialized)
    {
        return true;
    }

    if (!requested_path || requested_path[0] == '\0')
    {
        requested_path = getenv("ELENIXOS_IPC_SOCKET");
    }
    if (!requested_path || requested_path[0] == '\0')
    {
        requested_path = EOS_HEADLESS_IPC_DEFAULT_SOCKET;
    }

    socket_path_length = strlen(requested_path);
    if (socket_path_length == 0U || socket_path_length >= sizeof(address.sun_path))
    {
        return false;
    }

    length = snprintf(s_socket_path, sizeof(s_socket_path), "%s", requested_path);
    if (length <= 0 || (size_t)length >= sizeof(s_socket_path))
    {
        return false;
    }
    length = snprintf(s_ready_path, sizeof(s_ready_path), "%s.ready", s_socket_path);
    if (length <= 0 || (size_t)length >= sizeof(s_ready_path))
    {
        s_socket_path[0] = '\0';
        return false;
    }

    s_width = width;
    s_height = height;
    s_frame_size = (size_t)width * (size_t)height * 2U;
    s_latest_frame = (uint8_t *)calloc(1U, s_frame_size);
    s_frame_wire =
        (uint8_t *)calloc(1U, EOS_HEADLESS_IPC_HEADER_SIZE + EOS_HEADLESS_IPC_FRAME_META_SIZE + s_frame_size);
    if (!s_latest_frame || !s_frame_wire)
    {
        free(s_latest_frame);
        free(s_frame_wire);
        s_latest_frame = NULL;
        s_frame_wire = NULL;
        return false;
    }

    (void)unlink(s_socket_path);
    (void)unlink(s_ready_path);

    s_listener_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s_listener_fd < 0 || !_set_nonblocking(s_listener_fd))
    {
        eos_headless_ipc_shutdown();
        return false;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
#ifdef __APPLE__
    address.sun_len = (uint8_t)(offsetof(struct sockaddr_un, sun_path) + socket_path_length + 1U);
#endif
    memcpy(address.sun_path, s_socket_path, socket_path_length + 1U);
    if (bind(s_listener_fd, (struct sockaddr *)&address, sizeof(address)) != 0 || listen(s_listener_fd, 1) != 0)
    {
        eos_headless_ipc_shutdown();
        return false;
    }

    (void)chmod(s_socket_path, 0600);
    s_initialized = true;
    s_stop_requested = false;
    (void)signal(SIGINT, _signal_handler);
    (void)signal(SIGTERM, _signal_handler);
    (void)signal(SIGPIPE, SIG_IGN);
    (void)atexit(_cleanup_at_exit);

    if (!_write_ready_file())
    {
        eos_headless_ipc_shutdown();
        return false;
    }

    return true;
}

bool eos_headless_ipc_set_websocket_endpoint(const char *endpoint)
{
    if (!s_initialized || !endpoint)
        return false;
    if (snprintf(s_websocket_endpoint, sizeof(s_websocket_endpoint), "%s", endpoint)
        >= (int)sizeof(s_websocket_endpoint))
        return false;
    return _write_ready_file();
}

void eos_headless_ipc_poll(void)
{
    if (!s_initialized || s_stop_requested)
    {
        return;
    }

    _accept_clients();
    if (s_client_fd >= 0 && !_read_client())
    {
        _close_client();
    }
    _flush_client();
}

bool eos_headless_ipc_submit_frame(const uint8_t *rgb565, size_t size)
{
    if (!s_initialized || !rgb565 || size != s_frame_size)
    {
        return false;
    }

    if (s_client_fd < 0 || !s_client_ready)
    {
        return true;
    }
    memcpy(s_latest_frame, rgb565, s_frame_size);
    s_frame_dirty = true;
    _queue_latest_frame();
    return true;
}

bool eos_headless_ipc_should_exit(void)
{
    return s_stop_requested;
}

void eos_headless_ipc_shutdown(void)
{
    if (s_client_fd >= 0)
    {
        close(s_client_fd);
        s_client_fd = -1;
    }
    if (s_listener_fd >= 0)
    {
        close(s_listener_fd);
        s_listener_fd = -1;
    }

    if (s_socket_path[0] != '\0' || s_ready_path[0] != '\0')
    {
        _cleanup_files();
    }

    free(s_latest_frame);
    free(s_frame_wire);
    s_latest_frame = NULL;
    s_frame_wire = NULL;
    s_frame_size = 0U;
    s_frame_wire_size = 0U;
    s_frame_wire_offset = 0U;
    s_initialized = false;
    s_client_ready = false;
    s_socket_path[0] = '\0';
    s_ready_path[0] = '\0';
    s_websocket_endpoint[0] = '\0';
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#include "eos_config.h"
#include "eos_touch.h"

#define EOS_HEADLESS_IPC_MAGIC "EOS1"
#define EOS_HEADLESS_IPC_VERSION 1U
#define EOS_HEADLESS_IPC_HEADER_SIZE 16U
#define EOS_HEADLESS_IPC_FRAME_META_SIZE 16U
#define EOS_HEADLESS_IPC_MAX_CONTROL_PAYLOAD 4096U
#define EOS_HEADLESS_IPC_RX_SIZE 8192U

enum
{
    EOS_HEADLESS_IPC_MESSAGE_HELLO = 1,
    EOS_HEADLESS_IPC_MESSAGE_FRAME = 2,
    EOS_HEADLESS_IPC_MESSAGE_INPUT = 3,
    EOS_HEADLESS_IPC_MESSAGE_HELLO_ACK = 4
};

enum
{
    EOS_HEADLESS_IPC_INPUT_DOWN = 1,
    EOS_HEADLESS_IPC_INPUT_MOVE = 2,
    EOS_HEADLESS_IPC_INPUT_UP = 3
};

static SOCKET s_listener_socket = INVALID_SOCKET;
static SOCKET s_client_socket = INVALID_SOCKET;
static bool s_client_ready;
static bool s_initialized;
static bool s_stop_requested;
static bool s_wsa_started;
static uint32_t s_width;
static uint32_t s_height;
static size_t s_frame_size;
static uint8_t *s_latest_frame;
static uint8_t *s_frame_wire;
static size_t s_frame_wire_size;
static size_t s_frame_wire_offset;
static bool s_frame_dirty;
static uint8_t s_control_wire[EOS_HEADLESS_IPC_HEADER_SIZE + EOS_HEADLESS_IPC_MAX_CONTROL_PAYLOAD];
static size_t s_control_wire_size;
static size_t s_control_wire_offset;
static uint8_t s_rx_buffer[EOS_HEADLESS_IPC_RX_SIZE];
static size_t s_rx_size;
static char s_ready_path[1024];
static char s_endpoint[128];
static char s_websocket_endpoint[256];

static void _signal_handler(int signal_number)
{
    (void)signal_number;
    s_stop_requested = true;
}

static bool _set_nonblocking(SOCKET socket_handle)
{
    u_long mode = 1;
    return ioctlsocket(socket_handle, FIONBIO, &mode) == 0;
}

static void _write_u16_be(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value >> 8U);
    dst[1] = (uint8_t)value;
}

static void _write_u32_be(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value >> 24U);
    dst[1] = (uint8_t)(value >> 16U);
    dst[2] = (uint8_t)(value >> 8U);
    dst[3] = (uint8_t)value;
}

static uint16_t _read_u16_be(const uint8_t *src)
{
    return (uint16_t)(((uint16_t)src[0] << 8U) | src[1]);
}

static uint32_t _read_u32_be(const uint8_t *src)
{
    return ((uint32_t)src[0] << 24U) | ((uint32_t)src[1] << 16U) | ((uint32_t)src[2] << 8U) | src[3];
}

static void _write_header(uint8_t *dst, uint16_t type, uint32_t payload_size)
{
    memcpy(dst, EOS_HEADLESS_IPC_MAGIC, 4U);
    _write_u16_be(dst + 4U, EOS_HEADLESS_IPC_VERSION);
    _write_u16_be(dst + 6U, type);
    _write_u32_be(dst + 8U, payload_size);
    _write_u32_be(dst + 12U, 0U);
}

static void _close_client(void)
{
    if (s_client_socket != INVALID_SOCKET)
    {
        closesocket(s_client_socket);
        s_client_socket = INVALID_SOCKET;
    }
    s_client_ready = false;
    s_rx_size = 0U;
    s_control_wire_size = 0U;
    s_control_wire_offset = 0U;
    s_frame_wire_size = 0U;
    s_frame_wire_offset = 0U;
    s_frame_dirty = true;
}

static bool _write_ready_file(void)
{
    char temp_path[sizeof(s_ready_path) + 32U];
    FILE *file;
    int length = snprintf(temp_path, sizeof(temp_path), "%s.tmp.%u", s_ready_path, (unsigned)_getpid());
    if (length <= 0 || (size_t)length >= sizeof(temp_path))
        return false;

    file = fopen(temp_path, "w");
    if (!file)
        return false;
    if (fprintf(file,
                "{\"protocol\":\"elenixos-simulator\",\"version\":1,\"socket\":\"%s\","
                "\"pid\":%u,\"width\":%u,\"height\":%u,\"format\":\"rgb565-le\","
                "\"websocket\":\"%s\"}\n",
                s_endpoint,
                (unsigned)_getpid(),
                s_width,
                s_height,
                s_websocket_endpoint)
            < 0
        || fflush(file) != 0 || fclose(file) != 0)
    {
        (void)remove(temp_path);
        return false;
    }
    (void)remove(s_ready_path);
    return rename(temp_path, s_ready_path) == 0;
}

static bool _send_pending_buffer(const uint8_t *buffer, size_t size, size_t *offset)
{
    while (*offset < size)
    {
        int written = send(s_client_socket, (const char *)buffer + *offset, (int)(size - *offset), 0);
        if (written > 0)
        {
            *offset += (size_t)written;
            continue;
        }
        if (written < 0 && WSAGetLastError() == WSAEINTR)
            continue;
        if (written < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
            return true;
        return false;
    }
    return true;
}

static void _queue_hello_ack(void)
{
    static const uint8_t payload[] = {'o', 'k'};
    _write_header(s_control_wire, EOS_HEADLESS_IPC_MESSAGE_HELLO_ACK, (uint32_t)sizeof(payload));
    memcpy(s_control_wire + EOS_HEADLESS_IPC_HEADER_SIZE, payload, sizeof(payload));
    s_control_wire_size = EOS_HEADLESS_IPC_HEADER_SIZE + sizeof(payload);
    s_control_wire_offset = 0U;
}

static void _handle_input(const uint8_t *payload, size_t payload_size)
{
    if (payload_size != 9U)
        return;
    int32_t x = (int32_t)_read_u32_be(payload + 1U);
    int32_t y = (int32_t)_read_u32_be(payload + 5U);
    switch (payload[0])
    {
        case EOS_HEADLESS_IPC_INPUT_DOWN:
            (void)eos_touch_inject_down(x, y);
            break;
        case EOS_HEADLESS_IPC_INPUT_MOVE:
            (void)eos_touch_inject_move(x, y);
            break;
        case EOS_HEADLESS_IPC_INPUT_UP:
            (void)eos_touch_inject_up(x, y);
            break;
        default:
            break;
    }
}

static bool _handle_message(uint16_t type, const uint8_t *payload, size_t payload_size)
{
    if (type == EOS_HEADLESS_IPC_MESSAGE_HELLO)
    {
        s_client_ready = true;
        s_frame_dirty = true;
        _queue_hello_ack();
        return true;
    }
    if (type == EOS_HEADLESS_IPC_MESSAGE_INPUT && s_client_ready)
    {
        _handle_input(payload, payload_size);
        return true;
    }
    return false;
}

static bool _parse_client_messages(void)
{
    while (s_rx_size >= EOS_HEADLESS_IPC_HEADER_SIZE)
    {
        uint32_t payload_size = _read_u32_be(s_rx_buffer + 8U);
        size_t message_size;
        if (memcmp(s_rx_buffer, EOS_HEADLESS_IPC_MAGIC, 4U) != 0
            || _read_u16_be(s_rx_buffer + 4U) != EOS_HEADLESS_IPC_VERSION
            || payload_size > EOS_HEADLESS_IPC_MAX_CONTROL_PAYLOAD)
            return false;
        message_size = EOS_HEADLESS_IPC_HEADER_SIZE + (size_t)payload_size;
        if (s_rx_size < message_size)
            return true;
        if (!_handle_message(_read_u16_be(s_rx_buffer + 6U), s_rx_buffer + EOS_HEADLESS_IPC_HEADER_SIZE, payload_size))
            return false;
        s_rx_size -= message_size;
        if (s_rx_size > 0U)
            memmove(s_rx_buffer, s_rx_buffer + message_size, s_rx_size);
    }
    return true;
}

static bool _read_client(void)
{
    for (;;)
    {
        int received;
        if (s_rx_size >= sizeof(s_rx_buffer))
            return false;
        received = recv(s_client_socket, (char *)s_rx_buffer + s_rx_size, (int)(sizeof(s_rx_buffer) - s_rx_size), 0);
        if (received > 0)
        {
            s_rx_size += (size_t)received;
            if (!_parse_client_messages())
                return false;
            continue;
        }
        if (received == 0)
            return false;
        if (WSAGetLastError() == WSAEINTR)
            continue;
        if (WSAGetLastError() == WSAEWOULDBLOCK)
            return true;
        return false;
    }
}

static void _accept_client(void)
{
    SOCKET client = accept(s_listener_socket, NULL, NULL);
    if (client == INVALID_SOCKET)
    {
        return;
    }
    if (!_set_nonblocking(client))
    {
        closesocket(client);
        return;
    }
    _close_client();
    s_client_socket = client;
}

static void _queue_latest_frame(void)
{
    uint8_t *payload;

    if (!s_frame_dirty || s_frame_wire_size != 0U || !s_latest_frame)
        return;
    _write_header(s_frame_wire,
                  EOS_HEADLESS_IPC_MESSAGE_FRAME,
                  (uint32_t)(EOS_HEADLESS_IPC_FRAME_META_SIZE + s_frame_size));
    payload = s_frame_wire + EOS_HEADLESS_IPC_HEADER_SIZE;
    _write_u32_be(payload, s_width);
    _write_u32_be(payload + 4U, s_height);
    _write_u32_be(payload + 8U, s_width * 2U);
    _write_u32_be(payload + 12U, 1U);
    memcpy(payload + EOS_HEADLESS_IPC_FRAME_META_SIZE, s_latest_frame, s_frame_size);
    s_frame_wire_size = EOS_HEADLESS_IPC_HEADER_SIZE + EOS_HEADLESS_IPC_FRAME_META_SIZE + s_frame_size;
    s_frame_wire_offset = 0U;
    s_frame_dirty = false;
}

static void _flush_client(void)
{
    if (s_client_socket == INVALID_SOCKET || !s_client_ready)
        return;
    if (s_control_wire_offset < s_control_wire_size
        && !_send_pending_buffer(s_control_wire, s_control_wire_size, &s_control_wire_offset))
    {
        _close_client();
        return;
    }
    if (s_control_wire_offset < s_control_wire_size)
        return;
    _queue_latest_frame();
    if (s_frame_wire_offset < s_frame_wire_size
        && !_send_pending_buffer(s_frame_wire, s_frame_wire_size, &s_frame_wire_offset))
        _close_client();
    if (s_frame_wire_size != 0U && s_frame_wire_offset >= s_frame_wire_size)
    {
        s_frame_wire_size = 0U;
        s_frame_wire_offset = 0U;
        _queue_latest_frame();
    }
}

bool eos_headless_ipc_init(const char *socket_path, uint32_t width, uint32_t height)
{
    WSADATA wsa_data;
    struct sockaddr_in address;
    int address_length = sizeof(address);
    const char *requested_path = socket_path;
    int length;

    if (s_initialized)
        return true;
    if (!requested_path || requested_path[0] == '\0')
        requested_path = "elenixos-simulator";
    length = snprintf(s_ready_path, sizeof(s_ready_path), "%s.ready", requested_path);
    if (length <= 0 || (size_t)length >= sizeof(s_ready_path))
        return false;
    (void)remove(s_ready_path);
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        return false;
    s_wsa_started = true;
    s_width = width;
    s_height = height;
    s_frame_size = (size_t)width * (size_t)height * 2U;
    s_latest_frame = (uint8_t *)calloc(1U, s_frame_size);
    s_frame_wire =
        (uint8_t *)calloc(1U, EOS_HEADLESS_IPC_HEADER_SIZE + EOS_HEADLESS_IPC_FRAME_META_SIZE + s_frame_size);
    if (!s_latest_frame || !s_frame_wire)
    {
        eos_headless_ipc_shutdown();
        return false;
    }

    s_listener_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_listener_socket == INVALID_SOCKET || !_set_nonblocking(s_listener_socket))
    {
        eos_headless_ipc_shutdown();
        return false;
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(0);
    if (bind(s_listener_socket, (struct sockaddr *)&address, sizeof(address)) != 0 || listen(s_listener_socket, 1) != 0
        || getsockname(s_listener_socket, (struct sockaddr *)&address, &address_length) != 0)
    {
        eos_headless_ipc_shutdown();
        return false;
    }
    (void)snprintf(s_endpoint, sizeof(s_endpoint), "tcp://127.0.0.1:%u", (unsigned)ntohs(address.sin_port));
    s_initialized = true;
    s_stop_requested = false;
    (void)signal(SIGINT, _signal_handler);
    (void)signal(SIGTERM, _signal_handler);
    (void)atexit(eos_headless_ipc_shutdown);
    if (!_write_ready_file())
    {
        eos_headless_ipc_shutdown();
        return false;
    }
    return true;
}

bool eos_headless_ipc_set_websocket_endpoint(const char *endpoint)
{
    if (!s_initialized || !endpoint)
        return false;
    if (snprintf(s_websocket_endpoint, sizeof(s_websocket_endpoint), "%s", endpoint)
        >= (int)sizeof(s_websocket_endpoint))
        return false;
    return _write_ready_file();
}

void eos_headless_ipc_poll(void)
{
    if (!s_initialized || s_stop_requested)
        return;
    _accept_client();
    if (s_client_socket != INVALID_SOCKET && !_read_client())
        _close_client();
    _flush_client();
}

bool eos_headless_ipc_submit_frame(const uint8_t *rgb565, size_t size)
{
    if (!s_initialized || !rgb565 || size != s_frame_size)
        return false;
    if (s_client_socket == INVALID_SOCKET || !s_client_ready)
        return true;
    memcpy(s_latest_frame, rgb565, s_frame_size);
    s_frame_dirty = true;
    _queue_latest_frame();
    return true;
}

bool eos_headless_ipc_should_exit(void)
{
    return s_stop_requested;
}

void eos_headless_ipc_shutdown(void)
{
    _close_client();
    if (s_listener_socket != INVALID_SOCKET)
    {
        closesocket(s_listener_socket);
        s_listener_socket = INVALID_SOCKET;
    }
    if (s_initialized)
        (void)remove(s_ready_path);
    free(s_latest_frame);
    free(s_frame_wire);
    s_latest_frame = NULL;
    s_frame_wire = NULL;
    s_frame_size = 0U;
    s_frame_wire_size = 0U;
    s_frame_wire_offset = 0U;
    s_initialized = false;
    s_websocket_endpoint[0] = '\0';
    if (s_wsa_started)
    {
        WSACleanup();
        s_wsa_started = false;
    }
}

#else

bool eos_headless_ipc_init(const char *socket_path, uint32_t width, uint32_t height)
{
    (void)socket_path;
    (void)width;
    (void)height;
    return false;
}

bool eos_headless_ipc_set_websocket_endpoint(const char *endpoint)
{
    (void)endpoint;
    return false;
}

void eos_headless_ipc_poll(void)
{
}

bool eos_headless_ipc_submit_frame(const uint8_t *rgb565, size_t size)
{
    (void)rgb565;
    (void)size;
    return false;
}

bool eos_headless_ipc_should_exit(void)
{
    return false;
}

void eos_headless_ipc_shutdown(void)
{
}

#endif
